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

int main(int argc, char* argv[])
{
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    
    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();

    printf("Bootstrapping spottie...\n\n");

    printf("[PROTOBUF] Version: %s\n", protobuf_c_version());
    
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

    socketExit();
    consoleExit(NULL);
    return 0;
}