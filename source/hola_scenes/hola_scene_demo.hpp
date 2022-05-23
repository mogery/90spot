#pragma once
#include "../hola/hola.hpp"
#include "../hola/hola_text.hpp"
#include "../hola/hola_input.hpp"
#include "../hola/hola_button.hpp"
extern "C"
{
    #include "../spotifymgr.h"
}

class HolaSceneDemo : public HolaStaticElement {
    public:
        int onInteract(HolaInteraction e) {            
            HOLA_INTERACT_PT(button);

            return 0;
        }

        int render(SDL_Renderer* renderer) {
            HOLA_RENDER_CLEAR(14, 14, 14);
            
            HOLA_RENDER_PT(button);

            return 0;
        }

        HolaSceneDemo(HolaRect rect) : HolaStaticElement(rect) { }
    
    protected:
        int onButtonPressed()
        {
            return spotifymgr_demo();
        }

        static int onButtonPressed_forwarder(void* state)
        {
            return static_cast<HolaSceneDemo*>(state)->onButtonPressed();
        }

        HolaButtonElement button = HolaButtonElement(
            (HolaCoords){
                HOLA_ANCHOR_CENTER,
                getDimensions().w / 2,
                getDimensions().h / 2
            },
            hola_get_font_pile().sans_bold48,
            (char*)"Play Track",
            onButtonPressed_forwarder,
            (void*)this
        );
};