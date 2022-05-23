#ifndef _glob_H
#define _glob_H

#include <switch.h>
#include "spotifymgr.h"
#include "graphics.hpp"

void destroy()
{
    spotifymgr_destroy();
    gfx_clean();

    socketExit();
    setsysExit();
    romfsExit();
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

    destroy();

    appletUnlockExit();

    // https://github.com/switchbrew/libnx/blob/d14f931eab2fe19f8b5a7b80e1150b5e932342e1/nx/source/runtime/init.c#L190-L201
    void __attribute__((weak)) NORETURN __libnx_exit(int rc);
    __libnx_exit(0);
}

#endif