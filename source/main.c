#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <switch/crypto/aes_ctr.h>
#include <switch/services/set.h>
#include <switch.h>
#include "protobuf-c.h"
#include "dh.h"
#include "conv.h"
#include "log.h"
#include "secrets.h"
#include "stb_vorbis.h"
#include "spotify/apresolve.h"
#include "spotify/handshake.h"
#include "spotify/session.h"
#include "spotify/mercury.h"
#include "spotify/idtool.h"
#include "spotify/audiokey.h"
#include "spotify/channelmgr.h"
#include "spotify/fetch.h"
#include "spotify/audiofetch.h"

#include "spotify/proto/metadata.pb-c.h"

session_ctx* session = NULL;
mercury_ctx* mercury = NULL;
audiokey_ctx* audiokey = NULL;
channelmgr_ctx* channelmgr = NULL;
fetch_ctx* fetch = NULL;
audiofetch_ctx* audiofetch = NULL;

void cleanup()
{
    if (audiofetch != NULL)
        audiofetch_destroy(audiofetch);

    if (fetch != NULL)
        fetch_destroy(fetch);

    if (channelmgr != NULL)
        channelmgr_destroy(channelmgr);
    
    if (audiokey != NULL)
        audiokey_destroy(audiokey);

    if (mercury != NULL)
        mercury_destroy(mercury);

    if (session != NULL)
        session_destroy(session);

    socketExit();
    setsysExit();
    consoleExit(NULL);
}

void NORETURN panic()
{
    log_info("\n\n[!] panic() called: press + button to exit gracefully\n");
    consoleUpdate(NULL);

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu
    }

    cleanup();

    appletUnlockExit();

    // https://github.com/switchbrew/libnx/blob/d14f931eab2fe19f8b5a7b80e1150b5e932342e1/nx/source/runtime/init.c#L190-L201
    void __attribute__((weak)) NORETURN __libnx_exit(int rc);
    __libnx_exit(0);
}

void authentication_handler(session_ctx* session, bool success)
{
    if (!success)
    {
        panic();
    }

    // Initialize Mercury
    if ((mercury = mercury_init(session)) == NULL)
        panic();

    // Initialize AudioKey
    if ((audiokey = audiokey_init(session)) == NULL)
        panic();

    // Initialize ChannelMgr
    if ((channelmgr = channelmgr_init(session)) == NULL)
        panic();
    
    // Initialize Fetch
    if ((fetch = fetch_init(channelmgr)) == NULL)
        panic();
    
    // Initialize AudioFetch
    if ((audiofetch = audiofetch_init(fetch, audiokey)) == NULL)
        panic();

    consoleUpdate(NULL);
}

// struct ak_passthrough {
//     spotify_id trackid;
//     spotify_file_id fileid;

//     Aes128CtrContext enc;
//     FILE *file;
// };

// int test_fetch_data_handler(
//     struct fetch_ctx* ctx, // The Fetch context the response originates from
//     struct fetch_pending_request* req,
//     uint8_t* buf, // Data buffer
//     size_t len, // Data buffer length
//     void* _pt // Optional state arg
// )
// {
//     struct ak_passthrough* pt = _pt;

//     uint8_t* dec = malloc(len);
//     aes128CtrCrypt(&pt->enc, dec, buf, len);

//     fwrite(dec, sizeof(uint8_t), len, pt->file);

//     free(dec);

//     return 0;
// }

// int test_fetch_end_handler(
//     struct fetch_ctx* ctx, // The Fetch context the response originates from
//     struct fetch_pending_request* req,
//     void* _pt // Optional state arg
// )
// {
//     struct ak_passthrough* pt = _pt;

//     fclose(pt->file);
//     log_info("closed file!\n");
//     consoleUpdate(NULL);

//     return 0;
// }

// int test_audiokey_response_handler(audiokey_ctx* audiokey, uint8_t* _key, size_t len, void* _pt)
// {
//     struct ak_passthrough* pt = _pt;
    
//     uint8_t* key = malloc(len);
//     memcpy(key, _key, len);

//     uint8_t ctr[0x10] = {0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb, 0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64, 0x3f, 0x63, 0x0d, 0x93};
//     aes128CtrContextCreate(&pt->enc, key, ctr);

//     pt->file = fopen("/dump.ogg", "wb");
//     log_info("opened file! %p\n", pt->file);
//     consoleUpdate(NULL);

//     fetch_stream(
//         fetch,
//         pt->fileid,
//         0, FETCH_MAX_BLOCKS,
//         test_fetch_data_handler,
//         test_fetch_end_handler,
//         pt
//     );

//     return 0;
// }

int test_mercury_request_handler(mercury_ctx* mercury, Header* header, mercury_message_part* parts, void* _)
{
    log_info("Request handler called!\n");
    consoleUpdate(NULL);

    if (header->status_code >= 300)
    {
        log_error("Bad status code! Halting.");
        consoleUpdate(NULL);
        return -1;
    }

    if (parts == NULL)
    {
        log_error("Received no parts! Halting.");
        consoleUpdate(NULL);
        return -1;
    }

    log_info("1st part: #%p, len %d\n", parts->buf, parts->len);
    consoleUpdate(NULL);

    Track* track = track__unpack(NULL, parts->len, parts->buf);
    if (track == NULL)
    {
        log_error("Failed to unpack Track!\n");
        consoleUpdate(NULL);
        return -1;
    }

    log_info("Track unpacked!\n");
    consoleUpdate(NULL);

    spotify_id trackid = spotify_id_from_raw(track->gid.data, SAT_Track);

    log_info("%s - %s\n", track->artist[0]->name, track->name);
    consoleUpdate(NULL);

    AudioFile* bestSupported = NULL;

    for (int i = 0; i < track->n_file; i++)
    {
        AudioFile* file = track->file[i];

        if (file->format < 3)
        {
            if (bestSupported == NULL)
                bestSupported = file;
            else if (bestSupported->format < file->format)
                bestSupported = file;
        }
    }

    char file_id[SPOTIFY_FILE_ID_B16_LENGTH] = "<no id>";
    spotify_file_id sfid = spotify_file_id_from_raw(bestSupported->file_id.data);
    spotify_file_id_to_b16(file_id, sfid);
    printf("[Best] File %s: format = %d\n", file_id, bestSupported->format);

    if (audiofetch_create(audiofetch, trackid, sfid, NULL, NULL) == NULL)
        return -1;

    return 0;
}

int main(int argc, char* argv[])
{
    appletLockExit();

    consoleInit(NULL);
    setsysInitialize();

    time_t unixTime = time(NULL);

    char logfile_name[257];
    snprintf(logfile_name, 256, "/switchspot_%ld.log", unixTime);

    // Create switchspot.log file on the root of the SD card and copy stdout to it
    int logfd = open(logfile_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logfd == -1)
    {
        log_warn("[LOG] WARNING: Failed to open '%s'! Not logging to file.\n", logfile_name);
    }
    else
    {
        log_set_fd(logfd);
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    
    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();

    _log("\x1b[32m", "SwitchSpot %s\n", SWITCHSPOT_VERSION);
    log_info("Bootstrapping.\n\n");

    struct tm* timeStruct = localtime((const time_t *)&unixTime);
    log_info("%i. %02i. %02i. %02i:%02i:%02i\n\n", timeStruct->tm_year + 1900, timeStruct->tm_mon + 1, timeStruct->tm_mday, timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);

    log_debug("[PROTOBUF] Version: %s\n", protobuf_c_version());
    consoleUpdate(NULL);
    
    dh_init();
    dh_keys keys = dh_keygen();
    consoleUpdate(NULL);

    // Resolve a Spotify Access Point
    struct sockaddr_in* ap = apresolve();
    if (ap == NULL)
    {
        panic();
    }
    consoleUpdate(NULL);

    // Handshake with AP
    hs_res* handshake = spotify_handshake(ap, keys);
    if (handshake == NULL)
    {
        panic();
    }
    consoleUpdate(NULL);

    // Initialize session from handshake data & socket
    session = session_init(handshake);
    if (session == NULL)
    {
        panic();
    }
    consoleUpdate(NULL);

    // Authenticate session
    if (session_authenticate(session, SPOTIFY_USERNAME, SPOTIFY_PASSWORD, authentication_handler) < 0)
    {
        panic();
    }
    consoleUpdate(NULL);

    bool sent = false;

    // Main loop
    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        if (kDown & HidNpadButton_A && !sent)
        {
            sent = true;
            log_info("Sending mercury GET\n");
            consoleUpdate(NULL);

            spotify_id track_id = spotify_id_from_b62("2V6BCyyQ7kSXhkXwAb13OR", SAT_Track);
            char track_id_b16[SPOTIFY_ID_B16_LENGTH + 1];
            spotify_id_to_b16(track_id_b16, track_id);

            char* mercury_url = malloc(strlen("hm://metadata/3/track/") + SPOTIFY_ID_B16_LENGTH + 1);
            sprintf(mercury_url, "hm://metadata/3/track/%s", track_id_b16);

            if (mercury_get_request(mercury, mercury_url, test_mercury_request_handler, NULL) < 0)
            {
                panic();
            }
            consoleUpdate(NULL);
        }

        if (!(kDown & HidNpadButton_A) && sent)
            sent = false;

        if (session_update(session) < 0)
        {
            panic();
        }

        consoleUpdate(NULL);
    }

    cleanup();
    appletUnlockExit();
    return 0;
}