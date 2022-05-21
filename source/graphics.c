#include "graphics.h"
#include "SDL2_gfx/SDL2_gfxPrimitives.h"
#include <switch/display/native_window.h>
#include "log.h"
#include "gfx_scenes/gfx_scenes.h"

#define GFX_CHECK_FONT_LOAD(font) { \
    if (font == NULL) \
    { \
        log_error("[GRAPHICS] Failed to load font " #font ": %s\n", TTF_GetError()); \
        goto _fail_ttf; \
    } \
}

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

FontPile fonts;
GSC_Screen current_screen = GSC_LOGIN;

int gfx_init()
{
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

    fonts.inter36regular = TTF_OpenFontIndex("romfs:/fonts/Inter-Regular.ttf", 36, 0);
    GFX_CHECK_FONT_LOAD(fonts.inter36regular);
    fonts.inter36bold = TTF_OpenFontIndex("romfs:/fonts/Inter-Bold.ttf", 36, 0);
    GFX_CHECK_FONT_LOAD(fonts.inter36bold);
    fonts.inter48bold = TTF_OpenFontIndex("romfs:/fonts/Inter-Bold.ttf", 48, 0);
    GFX_CHECK_FONT_LOAD(fonts.inter48bold);

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
                int x = (int)(event.tfinger.x * 1920.0f), y = (int)(event.tfinger.y * 1080.0f);
                log_info("Finger down at x: %d y: %d\n", x, y);

                int touch_res = 0;
                if (current_screen == GSC_LOGIN)
                    touch_res = gsc_login_handle_touch(window, renderer, fonts, x, y);

                if (touch_res < 0)
                    return touch_res;
                break;

            default:
                break;
        }
    }

    int draw_res = 0;

    if (current_screen == GSC_LOGIN)
        draw_res = gsc_login_draw(window, renderer, fonts);

    if (draw_res < 0)
        return draw_res;

    SDL_RenderPresent(renderer);

    return 0;
}

void gfx_clean()
{
    if (TTF_WasInit())
        TTF_Quit();

    if (renderer != NULL)
        SDL_DestroyRenderer(renderer);
    
    if (window != NULL)
        SDL_DestroyWindow(window);
    
    SDL_Quit();
}

// Utilities

int gfx_text_draw(SDL_Renderer* renderer, TTF_Font* font, char* text, SDL_Color color, SDL_Rect* srcrect, SDL_Rect* dstrect)
{
    SDL_Surface* text_surface;
    SDL_Texture* text_texture;

    if ((text_surface = TTF_RenderText_Blended(font, text, color)) == NULL)
    {
        log_error("[GRAPHICS] Couldn't render text \"%s\": %s\n", text, TTF_GetError());
        return -1;
    }

    if ((text_texture = SDL_CreateTextureFromSurface(renderer, text_surface)) == NULL)
    {
        log_error("[GRAPHICS] Failed to convert text surface to texture: %s\n", SDL_GetError());
        return -1;
    }

    if (SDL_RenderCopy(renderer, text_texture, srcrect, dstrect) < 0)
    {
        log_error("[GRAPHICS] Failed to render text texture: %s\n", SDL_GetError());
        return -1;
    }

    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);

    return 0;
}