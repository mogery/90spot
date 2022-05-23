#pragma once
#include "hola.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "SDL2_gfx/SDL2_gfxPrimitives.h"

#define HOLA_RENDER_CLEAR(r, g, b) \
    SDL_SetRenderDrawColor(renderer, r, g, b, 255); \
    SDL_RenderClear(renderer);

#define HOLA_INTERACT_PT(elem) \
    if (hola_coords_inside_rect(e.pos, elem.getFootprint())) \
        return elem.onInteract(e);

#define HOLA_RENDER_PT(elem) \
    if (elem.render(renderer) < 0) \
        return -1;

HolaNormalCoord hola_get_normal_coord(HolaRect rect);
bool hola_coords_inside_rect(HolaNormalCoord pos, HolaRect rect);
int hola_util_text_draw(SDL_Renderer* renderer, TTF_Font* font, char* text, SDL_Color color, SDL_Rect* srcrect, SDL_Rect* dstrect);
