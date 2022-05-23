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

#include <switch/crypto/aes_ctr.h>
#include <switch/services/set.h>
#include <switch.h>
#include "protobuf-c.h"
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
#include "graphics.hpp"
#include "spotifymgr.h"

#include "spotify/proto/metadata.pb-c.h"

bool gfx_inited = false;

int main(int argc, char* argv[])
{
    appletLockExit();

    // consoleInit(NULL);
    romfsInit();
    setsysInitialize();
    socketInitializeDefault();

    nxlinkStdio();

    time_t unixTime = time(NULL);

    char logfile_name[257];
    snprintf(logfile_name, 256, "/90spot_%ld.log", unixTime);

    // Create 90spot.log file on the root of the SD card and copy stdout to it
    FILE* logfd = fopen(logfile_name, "w");
    if (logfd == NULL)
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

    if (spotifymgr_init() < 0)
        panic();

    if (gfx_init() < 0)
        panic();

    // Main loop
    while (appletMainLoop())
    {
        if (spotifymgr_update() < 0)
            panic();

        int gfx_res = gfx_update();
        if (gfx_res < 0)
            panic();
        if (gfx_res == 1)
            break;
    }

    destroy();
    appletUnlockExit();
    return 0;
}