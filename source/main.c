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
#include <malloc.h>

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
#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "driver_internal.h"

#include "spotify/proto/metadata.pb-c.h"

session_ctx* session = NULL;
mercury_ctx* mercury = NULL;
audiokey_ctx* audiokey = NULL;
channelmgr_ctx* channelmgr = NULL;
fetch_ctx* fetch = NULL;
audiofetch_ctx* audiofetch = NULL;

static const AudioRendererConfig arConfig =
{
    .output_rate     = AudioRendererOutputRate_48kHz,
    .num_voices      = 24,
    .num_effects     = 0,
    .num_sinks       = 1,
    .num_mix_objs    = 1,
    .num_mix_buffers = 2,
};

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
    audrenExit(); // TODO: proper audren cleanup
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

struct af_passthrough {
    int mem_scnt;
    int mem_bytes;
    int mem_ptr;

    size_t mempool_size;
    int16_t* mempool_ptr;
    int mpid;

    AudioDriver audrv;
    AudioDriverWaveBuf wavebuf;
};

int test_audiofetch_end_handler(audiofetch_ctx* ctx, audiofetch_request* req, void* _pt)
{
    struct af_passthrough* pt = _pt;

    armDCacheFlush(pt->mempool_ptr, pt->mempool_size);

    log_info("af ended, starting\n");
    consoleUpdate(NULL);

    audrvVoiceStop(&pt->audrv, 0);
    audrvVoiceAddWaveBuf(&pt->audrv, 0, &pt->wavebuf);
    audrvVoiceStart(&pt->audrv, 0);
    audrvUpdate(&pt->audrv);

    return 0;
}

int test_audiofetch_header_handler(audiofetch_ctx* ctx, audiofetch_request* req, vorbis_info info, void* _pt)
{
    struct af_passthrough* pt = _pt;
    log_info("Channels: %d Rate: %ld\n", info.channels, info.rate);

    pt->mem_scnt = info.channels * info.rate;
    pt->mem_bytes = pt->mem_scnt * sizeof(uint16_t);
    pt->mem_ptr = 0;

    pt->mempool_size = (pt->mem_bytes + 0xFFF) &~ 0xFFF;
    pt->mempool_ptr = memalign(0x1000, pt->mempool_size);
    memset(pt->mempool_ptr, 0, pt->mempool_size);
    armDCacheFlush(pt->mempool_ptr, pt->mempool_size);

    log_info("[AUDIO] Allocated mempool of size %ld! (%d samples for 1s)\n", pt->mempool_size, pt->mem_scnt);

    AudioDriverWaveBuf wavebuf = {0};
    wavebuf.data_raw = pt->mempool_ptr;
    wavebuf.size = pt->mem_bytes;
    wavebuf.start_sample_offset = 0;
    wavebuf.end_sample_offset = pt->mem_scnt;
    wavebuf.is_looping = true;
    pt->wavebuf = wavebuf;

    if (!R_SUCCEEDED(audrvCreate(&pt->audrv, &arConfig, 2)))
    {
        log_error("[AUDIO] Failed to initialize audrv.\n");
        return -1;
    }

    pt->mpid = audrvMemPoolAdd(&pt->audrv, pt->mempool_ptr, pt->mempool_size);
    audrvMemPoolAttach(&pt->audrv, pt->mpid);

    static const uint8_t sink_channels[] = { 0, 1 };
    audrvDeviceSinkAdd(&pt->audrv, AUDREN_DEFAULT_DEVICE_NAME, info.channels, sink_channels);
    
    if (!R_SUCCEEDED(audrvUpdate(&pt->audrv)))
    {
        log_error("[AUDIO] failed to update audrv after adding mempool and sink\n");
        return -1;
    }

    if (!R_SUCCEEDED(audrenStartAudioRenderer()))
    {
        log_error("[AUDIO] failed to start audren\n");
        return -1;
    }

    if (!audrvVoiceInit(&pt->audrv, 0, info.channels, PcmFormat_Int16, info.rate))
    {
        log_error("[AUDIO] Failed to init voice\n");
        return -1;
    }
    log_info("fuv: %d\n", pt->audrv.etc->first_used_voice);
    audrvVoiceSetDestinationMix(&pt->audrv, 0, AUDREN_FINAL_MIX_ID);

    if (info.channels == 1)
    {
        audrvVoiceSetMixFactor(&pt->audrv, 0, 1.0f, 0, 0);
        audrvVoiceSetMixFactor(&pt->audrv, 0, 1.0f, 0, 1);
    }
    else
    {
        audrvVoiceSetMixFactor(&pt->audrv, 0, 1.0f, 0, 0);
        audrvVoiceSetMixFactor(&pt->audrv, 0, 0.0f, 0, 1);
        audrvVoiceSetMixFactor(&pt->audrv, 0, 0.0f, 1, 0);
        audrvVoiceSetMixFactor(&pt->audrv, 0, 1.0f, 1, 1);
    }

    log_info("[AUDIO] Voice running!\n");

    return 0;
}

int test_audiofetch_frame_handler(audiofetch_ctx* ctx, audiofetch_request* req, float** samples, int count, int channels, void* _pt)
{
    struct af_passthrough* pt = _pt;

    int remaining_samples = pt->mem_scnt - pt->mem_ptr;
    int commiting_samples = count * channels;
    if (commiting_samples > remaining_samples)
        commiting_samples = remaining_samples;

    for (int i = 0; i < commiting_samples; i++)
    {
        int si = i / channels;
        int channel = i % channels;

        float f = samples[channel][si];
        uint16_t x;
        f = f * 32768 ;
        if( f > 32767 ) f = 32767;
        if( f < -32768 ) f = -32768;
        x = (uint16_t) f;

        pt->mempool_ptr[pt->mem_ptr + i] = x;
    }

    pt->mem_ptr += commiting_samples;

    //printf("(%d) %d samples have been copied to mempool. %d %d\n", remaining_samples, commiting_samples, pt->wavebuf.state, audrvVoiceGetPlayedSampleCount(&pt->audrv, 0));
    return count;
}

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

    struct af_passthrough* pt = malloc(sizeof(struct af_passthrough));

    if (audiofetch_create(audiofetch, trackid, sfid, test_audiofetch_end_handler, test_audiofetch_header_handler, test_audiofetch_frame_handler, pt) == NULL)
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

    socketInitializeDefault();

    _log("\x1b[32m", "90spot %s\n", _90SPOT_VERSION);
    struct tm* timeStruct = localtime((const time_t *)&unixTime);
    log_info("%i. %02i. %02i. %02i:%02i:%02i\n", timeStruct->tm_year + 1900, timeStruct->tm_mon + 1, timeStruct->tm_mday, timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);
    log_info("Bootstrapping...\n\n");

    log_debug("[PROTOBUF] Version: %s\n", protobuf_c_version());
    consoleUpdate(NULL);

    if (!R_SUCCEEDED(audrenInitialize(&arConfig)))
    {
        log_error("Failed to initialize audren.\n");
        panic();
    }
    
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