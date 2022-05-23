#pragma once
#undef __APPLE__ // intellisense sucks
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef __cplusplus
extern "C"
{
#endif
    int gfx_init();
    int gfx_update();
    void gfx_clean();
#ifdef __cplusplus
}
#endif