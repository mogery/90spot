#pragma once
#undef __APPLE__ // intellisense sucks
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct {
    TTF_Font* inter48bold;
    TTF_Font* inter36regular;
    TTF_Font* inter36bold;
} FontPile;

int gfx_init();
int gfx_update();
void gfx_clean();

// Utilities
int gfx_text_draw(SDL_Renderer* renderer, TTF_Font* font, char* text, SDL_Color color, SDL_Rect* srcrect, SDL_Rect* dstrect);