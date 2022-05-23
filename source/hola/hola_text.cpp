#include "hola_text.hpp"
#include "hola_util.hpp"

HolaDimensions HolaTextElement::getDynamicDimensions()
{
    if (this->font == NULL || this->content == NULL)
        return (HolaDimensions){0, 0};

    int fw, fh;
    TTF_SizeUTF8(this->font, this->content, &fw, &fh); // TODO: Error Handling
    return (HolaDimensions){fw, fh};
}

int HolaTextElement::render(SDL_Renderer* renderer)
{
    if (content == NULL || strlen(content) == 0 || font == NULL)
        return 0;

    HolaNormalCoord npos = hola_get_normal_coord(this->getFootprint());
    HolaDimensions ddim = this->getDynamicDimensions();

    SDL_Rect srcrect = { 0, 0, ddim.w, ddim.h };
    SDL_Rect dstrect = { npos.x, npos.y, ddim.w, ddim.h };

    if (has_max_size)
    {
        // If text would overflow horizontally, show rightmost of text in correct bounding box
        if (ddim.w > max_w)
        {
            dstrect.w = srcrect.w = max_w;
            srcrect.x += ddim.w - max_w; // show right instead of left
        }

        // If text would overflow vertically, show topmost of text in correct bounding box
        if (ddim.h > max_h)
        {
            dstrect.h = srcrect.h = max_h;
        }

        // TODO: Dynamic alignment inside max size
        if (ddim.h < max_h)
        {
            dstrect.y += max_h / 2 - ddim.h / 2;
        }
    }

    return hola_util_text_draw(
        renderer,
        font,
        content,
        color,
        &srcrect,
        &dstrect
    );
}