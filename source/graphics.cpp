#include "graphics.hpp"
extern "C"
{
    #include "spotifymgr.h"
}
#include <switch/display/native_window.h>
#include "log.h"
#include "hola_scenes/hola_scene_auth.hpp"

#define GFX_CHECK_FONT_LOAD(font) { \
    if (font == NULL) \
    { \
        log_error("[GRAPHICS] Failed to load font " #font ": %s\n", TTF_GetError()); \
        goto _fail_ttf; \
    } \
}

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int gfx_init()
{
    HolaFontPile* fonts = hola_get_font_pile_ref();
    
    log_info("[GFX] Initializing...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        log_error("[GRAPHICS] Failed to initialize SDL: %s\n", SDL_GetError());
        goto _fail;
    }

    window = SDL_CreateWindow("sdl2_gles2", 0, 0, 1920, 1080, 0);
    if (!window)
    {
        log_error("[GRAPHICS] Failed to create window: %s\n", SDL_GetError());
        goto _fail_sdl;
    }

    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        log_error("[GRAPHICS] Failed to create renderer: %s\n", SDL_GetError());
        goto _fail_sdl;
    }

    for (int i = 0; i < 2; i++)
    {
        if (SDL_JoystickOpen(i) == NULL)
        {
            log_error("[GRAPHICS] Failed to open joystick %d: %s\n", i, SDL_GetError());
            goto _fail_sdl;
        }
    }

    if (TTF_Init() < 0)
    {
        log_error("[GRAPHICS] Failed to initialize SDL_ttf: %s\n", TTF_GetError());
        goto _fail_sdl;
    }

    log_info("[GFX] Loading fonts...\n");

    fonts->sans_regular36 = TTF_OpenFontIndex("romfs:/fonts/Inter-Regular.ttf", 36, 0);
    GFX_CHECK_FONT_LOAD(fonts->sans_regular36);
    fonts->sans_bold36 = TTF_OpenFontIndex("romfs:/fonts/Inter-Bold.ttf", 36, 0);
    GFX_CHECK_FONT_LOAD(fonts->sans_bold36);
    fonts->sans_bold48 = TTF_OpenFontIndex("romfs:/fonts/Inter-Bold.ttf", 48, 0);
    GFX_CHECK_FONT_LOAD(fonts->sans_bold48);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    hola_set_scene(new HolaSceneAuth(
        (HolaRect){
            (HolaCoords){
                HOLA_ANCHOR_TOP_LEFT,
                0, 0
            },
            (HolaDimensions){
                w, h
            }
        }
    ));

    log_info("[GFX] Done!\n");
    return 0;

_fail_ttf:
    TTF_Quit();
_fail_sdl:
    SDL_Quit();
_fail:
    return -1;
}

int gfx_update()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_JOYAXISMOTION:
                log_info("Joystick %d axis %d value: %d\n",
                        event.jaxis.which,
                        event.jaxis.axis, event.jaxis.value);
                break;

            case SDL_JOYBUTTONDOWN:
                log_info("Joystick %d button %d down\n",
                        event.jbutton.which, event.jbutton.button);
                if (event.jbutton.which == 0 && event.jbutton.button == 10) {
                    // (+) button down
                    return 1;
                }
                break;

            case SDL_FINGERDOWN:
                {
                    int x = (int)(event.tfinger.x * 1920.0f), y = (int)(event.tfinger.y * 1080.0f);
                    log_info("Finger down at x: %d y: %d\n", x, y);

                    if (hola_interact((HolaInteraction){
                        HOLA_INTERACTION_TOUCH,
                        (HolaNormalCoord){ x, y }
                    }) < 0)
                        return -1;
                    break;
                }

            default:
                break;
        }
    }

    if (hola_update(renderer) < 0)
        return -1;

    SDL_RenderPresent(renderer);

    return 0;
}

void gfx_clean()
{
    hola_clear_scene();

    if (TTF_WasInit())
        TTF_Quit();

    if (renderer != NULL)
        SDL_DestroyRenderer(renderer);
    
    if (window != NULL)
        SDL_DestroyWindow(window);
    
    SDL_Quit();
}