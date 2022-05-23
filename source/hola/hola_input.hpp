#pragma once
#include "hola.hpp"
#include "hola_text.hpp"
#include <SDL2/SDL_ttf.h>

typedef enum {
    HOLA_INPUT_DEFAULT,
    HOLA_INPUT_PASSWORD
} HolaInputType;

class HolaInputElement : public HolaStaticElement {
    public:
        int render(SDL_Renderer* renderer);

        HolaInputElement(HolaRect footprint, TTF_Font* font, HolaInputType type, char* placeholder) : HolaStaticElement(footprint)
        {
            this->type = type;
            this->placeholder = placeholder;
            this->text.setFont(font);
        }

        int onInteract(HolaInteraction e);

        void setContent(const char* content)
        {
            int len = strlen(content);
            this->content = (char*)malloc(len + 1);
            memcpy(this->content, content, len);
            this->content[len] = 0;
        }

        char* getContent()
        {
            return this->content;
        }

        void setPlaceholder(char* placeholder)
        {
            this->placeholder = placeholder;
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

        HolaInputType type;
        char* content = NULL;
        char* placeholder = NULL;

        char* password_buf = NULL;
};