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

#include <switch/services/set.h>
#include <switch.h>
#include "spotify/apresolve.h"
#include "protobuf-c.h"
#include "dh.h"
#include "spotify/handshake.h"
#include "spotify/session.h"
#include "log.h"
#include "secrets.h"
#include "spotify/mercury.h"

#include "spotify/proto/metadata.pb-c.h"

session_ctx* session = NULL;
mercury_ctx* mercury = NULL;

void cleanup()
{
    if (mercury != NULL)
    {
        mercury_destroy(mercury);
    }

    if (session != NULL)
    {
        session_destroy(session);
    }

    socketExit();
    setsysExit();
    consoleExit(NULL);
}

void NORETURN panic()
{
    log("\n\n[!] panic() called: press + button to exit gracefully\n");
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
    mercury = mercury_init(session);
    if (mercury == NULL)
    {
        panic();
    }
    consoleUpdate(NULL);
}

int test_mercury_request_handler(mercury_ctx* mercury, Header* header, mercury_message_part* parts, void* _)
{
    log("Request handler called!\n");
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

    log("1st part: #%p, len %d\n", parts->buf, parts->len);
    consoleUpdate(NULL);

    Track* track = track__unpack(NULL, parts->len, parts->buf);
    if (track == NULL)
    {
        log_error("Failed to unpack Track!\n");
        consoleUpdate(NULL);
        return -1;
    }

    log("Track unpacked!\n");
    consoleUpdate(NULL);

    log("%s - %s\n", track->artist[0]->name, track->name);
    consoleUpdate(NULL);

    return 0;
}

int main(int argc, char* argv[])
{
    appletLockExit();

    consoleInit(NULL);
    setsysInitialize();

    time_t unixTime = time(NULL);

    char logfile_name[257];
    snprintf(logfile_name, 256, "/spottie_%ld.log", unixTime);

    // Create spottie.log file on the root of the SD card and copy stdout to it
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

    _log("\x1b[32m", "spottie %s\n", SPOTTIE_VERSION);
    log("Bootstrapping.\n\n");

    struct tm* timeStruct = localtime((const time_t *)&unixTime);
    log("%i. %02i. %02i. %02i:%02i:%02i\n\n", timeStruct->tm_year + 1900, timeStruct->tm_mon + 1, timeStruct->tm_mday, timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);

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
            log("Sending mercury GET\n");
            consoleUpdate(NULL);
            if (mercury_get_request(mercury, "hm://metadata/3/track/a7447d8aef4e4a2f94c074d833c37265", test_mercury_request_handler, NULL) < 0)
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