#pragma once
#include "hola.hpp"
#include "hola_text.hpp"
#include <SDL2/SDL_ttf.h>

class HolaButtonElement : public HolaElement {
    public:
        HolaDimensions getDimensions()
        {
            HolaDimensions tdim = this->text.getDimensions();
            tdim.w += 40;
            tdim.h += 40;
            return tdim;
        }

        int render(SDL_Renderer* renderer);

        HolaButtonElement(HolaCoords pos, TTF_Font* font, char* content, int (*onPress)(void*), void* state) : HolaElement(pos)
        {
            this->content = content;
            this->text.setContent(content);
            this->text.setFont(font);
            this->onPress = onPress;
            this->onPress_state = state;
        }

        int onInteract(HolaInteraction e);

        char* getContent()
        {
            return this->content;
        }

        void setRealContent(char* real_content)
        {
            this->real_content = real_content;
        }

        void clearRealContent()
        {
            this->real_content = NULL;
        }
    
    protected:
        HolaTextElement text = HolaTextElement(
            (HolaCoords){
                HOLA_ANCHOR_TOP_LEFT,
                getPosition().x + 20,
                getPosition().y + 20
            },
            NULL,
            content,
            (SDL_Color){ 255, 255, 255, 255 }
        );

        char* content = NULL;
        char* real_content = NULL;

        int (*onPress)(void*) = NULL;
        void* onPress_state = NULL;
};