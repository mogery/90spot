#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <switch.h>
#include "apresolve.h"
#include "protobuf-c.h"
#include "dh.h"
#include "handshake.h"
#include "session.h"
#include "log.h"

void cleanup(session_ctx* session)
{
    if (session != NULL)
    {
        session_destroy(session);
    }

    socketExit();
    consoleExit(NULL);
}

void NORETURN panic(session_ctx* session)
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

        consoleUpdate(NULL);
    }

    cleanup(NULL);

    appletUnlockExit();

    // https://github.com/switchbrew/libnx/blob/d14f931eab2fe19f8b5a7b80e1150b5e932342e1/nx/source/runtime/init.c#L190-L201
    void __attribute__((weak)) NORETURN __libnx_exit(int rc);
    __libnx_exit(0);
}

int main(int argc, char* argv[])
{
    session_ctx* session = NULL;

    appletLockExit();

    consoleInit(NULL);

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

    log("Bootstrapping spottie...\n");

    struct tm* timeStruct = localtime((const time_t *)&unixTime);
    log("%i. %02i. %02i. %02i:%02i:%02i\n\n", timeStruct->tm_year + 1900, timeStruct->tm_mon + 1, timeStruct->tm_mday, timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);

    log_debug("[PROTOBUF] Version: %s\n", protobuf_c_version());
    consoleUpdate(NULL);
    
    dh_init();
    dh_keys keys = dh_keygen();

    struct sockaddr_in* ap = apresolve();

    spotify_handshake(ap, keys);

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

        consoleUpdate(NULL);
    }

    cleanup(session);
    appletUnlockExit();
    return 0;
}