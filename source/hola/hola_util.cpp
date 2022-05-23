#include "hola_util.hpp"
#include "log.h"

HolaNormalCoord hola_get_normal_coord(HolaRect rect)
{
    HolaCoords pos = rect.pos;
    HolaDimensions dim = rect.dim;

    if (pos.anchor == HOLA_ANCHOR_TOP_LEFT)
        return (HolaNormalCoord){rect.pos.x, rect.pos.y};
    if (pos.anchor == HOLA_ANCHOR_TOP_CENTER)
        return (HolaNormalCoord){rect.pos.x - dim.w / 2, rect.pos.y};
    if (pos.anchor == HOLA_ANCHOR_TOP_RIGHT)
        return (HolaNormalCoord){rect.pos.x - dim.w, rect.pos.y};
    if (pos.anchor == HOLA_ANCHOR_CENTER_LEFT)
        return (HolaNormalCoord){rect.pos.x, rect.pos.y - dim.h / 2};
    if (pos.anchor == HOLA_ANCHOR_CENTER)
        return (HolaNormalCoord){rect.pos.x - dim.w / 2, rect.pos.y - dim.h / 2};
    if (pos.anchor == HOLA_ANCHOR_CENTER_RIGHT)
        return (HolaNormalCoord){rect.pos.x - dim.w, rect.pos.y - dim.h / 2};
    if (pos.anchor == HOLA_ANCHOR_BOTTOM_LEFT)
        return (HolaNormalCoord){rect.pos.x, rect.pos.y - dim.h};
    if (pos.anchor == HOLA_ANCHOR_BOTTOM_CENTER)
        return (HolaNormalCoord){rect.pos.x - dim.w / 2, rect.pos.y - dim.h};
    if (pos.anchor == HOLA_ANCHOR_BOTTOM_RIGHT)
        return (HolaNormalCoord){rect.pos.x - dim.w, rect.pos.y - dim.h};
    
    return (HolaNormalCoord){rect.pos.x, rect.pos.y};
}

bool hola_coords_inside_rect(HolaNormalCoord pos, HolaRect rect)
{
    HolaNormalCoord rect_pos = hola_get_normal_coord(rect);
    HolaDimensions rect_dim = rect.dim;

    return pos.x >= rect_pos.x
        && pos.x <= rect_pos.x + rect_dim.w
        && pos.y >= rect_pos.y
        && pos.y <= rect_pos.y + rect_dim.h;
}

int hola_util_text_draw(SDL_Renderer* renderer, TTF_Font* font, char* text, SDL_Color color, SDL_Rect* srcrect, SDL_Rect* dstrect)
{
    SDL_Surface* text_surface;
    SDL_Texture* text_texture;

    if ((text_surface = TTF_RenderText_Blended(font, text, color)) == NULL)
    {
        log_error("[HOLA] Couldn't render text \"%s\": %s\n", text, TTF_GetError());
        return -1;
    }

    if ((text_texture = SDL_CreateTextureFromSurface(renderer, text_surface)) == NULL)
    {
        log_error("[HOLA] Failed to convert text surface to texture: %s\n", SDL_GetError());
        return -1;
    }

    if (SDL_RenderCopy(renderer, text_texture, srcrect, dstrect) < 0)
    {
        log_error("[HOLA] Failed to render text texture: %s\n", SDL_GetError());
        return -1;
    }

    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);

    return 0;
}