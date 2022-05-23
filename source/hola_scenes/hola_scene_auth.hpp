#pragma once
#include "../hola/hola.hpp"
#include "../hola/hola_text.hpp"
#include "../hola/hola_input.hpp"
#include "../hola/hola_button.hpp"
#include "../hola/hola_fonts.hpp"
#include "secrets.h"

class HolaSceneAuth : public HolaStaticElement {
    public:
        int onInteract(HolaInteraction e);
        int render(SDL_Renderer* renderer);

        HolaSceneAuth(HolaRect rect) : HolaStaticElement(rect)
        {
            username.setContent(SPOTIFY_USERNAME);
            password.setContent(SPOTIFY_PASSWORD);
        }

    protected:
        TTF_Font* font_title = hola_get_font_pile().sans_bold48;
        TTF_Font* font_regular = hola_get_font_pile().sans_regular36;
        TTF_Font* font_bold = hola_get_font_pile().sans_bold36;

        HolaTextElement title = HolaTextElement(
            (HolaCoords){
                HOLA_ANCHOR_BOTTOM_CENTER,
                getDimensions().w / 2,
                getDimensions().h / 2 - 325
            },
            font_title,
            (char*)"Sign In",
            (SDL_Color){ 255, 255, 255, 255 }
        );

        HolaInputElement username = HolaInputElement(
            (HolaRect){
                (HolaCoords){
                    HOLA_ANCHOR_BOTTOM_CENTER,
                    getDimensions().w / 2,
                    getDimensions().h / 2 - 37
                },
                (HolaDimensions){ 600, 100 }
            },
            font_regular,
            HOLA_INPUT_DEFAULT,
            (char*)"Username / E-mail"
        );

        HolaInputElement password = HolaInputElement(
            (HolaRect){
                (HolaCoords){
                    HOLA_ANCHOR_TOP_CENTER,
                    getDimensions().w / 2,
                    getDimensions().h / 2 + 38
                },
                (HolaDimensions){ 600, 100 }
            },
            font_regular,
            HOLA_INPUT_PASSWORD,
            (char*)"Password"
        );

        int onButtonPressed();
        static int onButtonPressed_forwarder(void* state)
        {
            return static_cast<HolaSceneAuth*>(state)->onButtonPressed();
        }

        int onAuthenticated();
        static int onAuthenticated_forwarder(void* state)
        {
            return static_cast<HolaSceneAuth*>(state)->onAuthenticated();
        }

        HolaButtonElement button = HolaButtonElement(
            (HolaCoords){
                HOLA_ANCHOR_TOP_CENTER,
                getDimensions().w / 2,
                getDimensions().h / 2 + 325
            },
            font_bold,
            (char*)"Sign In",
            onButtonPressed_forwarder,
            (void*)this
        );
};