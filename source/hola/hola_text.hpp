#pragma once
#include "hola.hpp"
#include <SDL2/SDL_ttf.h>

// Hola Element used for text rendering
class HolaTextElement : public HolaElement {
    public:
        HolaDimensions getDimensions()
        {
            if (has_max_size)
                return (HolaDimensions){max_w, max_h};
            else
                return getDynamicDimensions();
        }

        int onInteract(HolaInteraction e) { return 0; }

        int render(SDL_Renderer* renderer);

        HolaTextElement(HolaCoords pos, TTF_Font* font, char* content, SDL_Color color) : HolaElement(pos)
        {
            this->font = font;
            this->content = content;
            this->color = color;
        }

        void setContent(char* content)
        {
            this->content = content;
        }

        void setColor(SDL_Color color)
        {
            this->color = color;
        }

        void setMaxSize(HolaDimensions max)
        {
            this->has_max_size = true;
            this->max_h = max.h;
            this->max_w = max.w;
        }

        void clearMaxSize()
        {
            this->has_max_size = false;
        }

        void setFont(TTF_Font* font)
        {
            this->font = font;
        }

    protected:
        bool has_max_size = false;
        int max_w, max_h;

        TTF_Font* font;
        char* content;
        SDL_Color color;

        HolaDimensions getDynamicDimensions();
};