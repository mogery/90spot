#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>

#include "main.h"

#include <switch/crypto/aes_ctr.h>
#include <switch/services/set.h>
#include <switch.h>
#include "protobuf-c.h"
#include "dh.h"
#include "conv.h"
#include "log.h"
#include "secrets.h"
#include "spotify/apresolve.h"
#include "spotify/handshake.h"
#include "spotify/session.h"
#include "spotify/mercury.h"
#include "spotify/idtool.h"
#include "spotify/audiokey.h"
#include "spotify/channelmgr.h"
#include "spotify/fetch.h"
#include "spotify/audiofetch.h"
#include "audioplay.h"
#include "graphics.h"

#include "spotify/proto/metadata.pb-c.h"

bool gfx_inited = false;
session_ctx* session = NULL;
mercury_ctx* mercury = NULL;
audiokey_ctx* audiokey = NULL;
channelmgr_ctx* channelmgr = NULL;
fetch_ctx* fetch = NULL;
audiofetch_ctx* audiofetch = NULL;
audioplay_ctx* audioplay = NULL;

void cleanup()
{
    if (audioplay != NULL)
        audioplay_destroy(audioplay);
    
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
    
    gfx_clean();

    audioplay_svc_cleanup();
    socketExit();
    setsysExit();
    romfsExit();
    // consoleExit(NULL);
}

void NORETURN panic()
{
    log_info("\n\n[!] panic() called: press + button to exit gracefully\n");
    // consoleUpdate(NULL);

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
    
    // Initialize AudioPlay
    if ((audioplay = audioplay_init(audiofetch)) == NULL)
        panic();
}

int test_mercury_request_handler(mercury_ctx* mercury, Header* header, mercury_message_part* parts, void* _)
{
    log_info("Request handler called!\n");
    // consoleUpdate(NULL);

    if (header->status_code >= 300)
    {
        log_error("Bad status code! Halting.");
        // consoleUpdate(NULL);
        return -1;
    }

    if (parts == NULL)
    {
        log_error("Received no parts! Halting.");
        // consoleUpdate(NULL);
        return -1;
    }

    log_info("1st part: #%p, len %d\n", parts->buf, parts->len);
    // consoleUpdate(NULL);

    Track* track = track__unpack(NULL, parts->len, parts->buf);
    if (track == NULL)
    {
        log_error("Failed to unpack Track!\n");
        // consoleUpdate(NULL);
        return -1;
    }

    log_info("Track unpacked!\n");
    // consoleUpdate(NULL);

    spotify_id trackid = spotify_id_from_raw(track->gid.data, SAT_Track);

    log_info("%s - %s\n", track->artist[0]->name, track->name);
    // consoleUpdate(NULL);

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

    if (audioplay_track_create(audioplay, trackid, sfid) != 0)
        return -1;

    return 0;
}

void authenticate(char* username, char* password)
{
    // Authenticate session
    if (session_authenticate(session, username, password, authentication_handler) < 0)
        panic();
}

int main(int argc, char* argv[])
{
    appletLockExit();

    // consoleInit(NULL);
    romfsInit();
    setsysInitialize();
    socketInitializeDefault();

    nxlinkStdio();

    if (gfx_init() < 0)
        panic();

    time_t unixTime = time(NULL);

    char logfile_name[257];
    snprintf(logfile_name, 256, "/90spot_%ld.log", unixTime);

    // Create 90spot.log file on the root of the SD card and copy stdout to it
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

    _log("\x1b[32m", "90spot %s\n", _90SPOT_VERSION);
    struct tm* timeStruct = localtime((const time_t *)&unixTime);
    log_info("%i. %02i. %02i. %02i:%02i:%02i\n", timeStruct->tm_year + 1900, timeStruct->tm_mon + 1, timeStruct->tm_mday, timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);
    log_info("Bootstrapping...\n\n");

    log_debug("[PROTOBUF] Version: %s\n", protobuf_c_version());
    // consoleUpdate(NULL);

    if (!audioplay_svc_init())
        panic();
    
    dh_init();
    dh_keys keys = dh_keygen();
    // consoleUpdate(NULL);

    // Resolve a Spotify Access Point
    struct sockaddr_in* ap = apresolve();
    if (ap == NULL)
        panic();
    // consoleUpdate(NULL);

    // Handshake with AP
    hs_res* handshake = spotify_handshake(ap, keys);
    if (handshake == NULL)
        panic();
    // consoleUpdate(NULL);

    // Initialize session from handshake data & socket
    session = session_init(handshake);
    if (session == NULL)
        panic();
    // consoleUpdate(NULL);

    bool sent = false;

    // Main loop
    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_A && !sent)
        {
            sent = true;
            log_info("Sending mercury GET\n");
            // consoleUpdate(NULL);

            spotify_id track_id = spotify_id_from_b62("08auB3LvJQJcasevC2nkPc", SAT_Track);
            char track_id_b16[SPOTIFY_ID_B16_LENGTH + 1];
            spotify_id_to_b16(track_id_b16, track_id);

            char* mercury_url = malloc(strlen("hm://metadata/3/track/") + SPOTIFY_ID_B16_LENGTH + 1);
            sprintf(mercury_url, "hm://metadata/3/track/%s", track_id_b16);

            if (mercury_get_request(mercury, mercury_url, test_mercury_request_handler, NULL) < 0)
            {
                panic();
            }
            // consoleUpdate(NULL);
        }

        if (session_update(session) < 0)
            panic();

        if (audioplay != NULL && audioplay_update(audioplay) < 0)
            panic();

        // consoleUpdate(NULL);
        int gfx_res = gfx_update();
        if (gfx_res < 0)
            panic();
        if (gfx_res == 1)
            break;
    }

    cleanup();
    appletUnlockExit();
    return 0;
}