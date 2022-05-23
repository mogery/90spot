#include "hola_fonts.hpp"

static HolaFontPile pile = {0};

HolaFontPile hola_get_font_pile()
{
    return pile;
}

HolaFontPile* hola_get_font_pile_ref()
{
    return &pile;
}