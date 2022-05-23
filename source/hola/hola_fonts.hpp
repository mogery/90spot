#pragma once
#include "hola.hpp"
#include <SDL2/SDL_ttf.h>

typedef struct {
    TTF_Font* sans_regular36;
    TTF_Font* sans_bold36;
    TTF_Font* sans_bold48;
} HolaFontPile;

HolaFontPile hola_get_font_pile();
HolaFontPile* hola_get_font_pile_ref();