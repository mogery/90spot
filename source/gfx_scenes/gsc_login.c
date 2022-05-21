#include "gsc_login.h"
#include <switch/applets/swkbd.h>
#include "log.h"

#include "../main.h"

#define GSC_LOGIN_INPUT_FIELD_WIDTH 600 // width of a single input field
#define GSC_LOGIN_INPUT_FIELD_HEIGHT 100 // height of a single input field
#define GSC_LOGIN_INPUT_FIELD_GAP 75 // gap between input fields

#define GSC_LOGIN_INPUT_TEXT_HPAD 10 // vertical padding of text inside input field
#define GSC_LOGIN_INPUT_TEXT_WPAD 20 // horizontal padding of text inside input field
#define GSC_LOGIN_INPUT_TEXT_HEIGHT \
    GSC_LOGIN_INPUT_FIELD_HEIGHT - (2 * GSC_LOGIN_INPUT_TEXT_HPAD)
#define GSC_LOGIN_INPUT_TEXT_WIDTH \
    GSC_LOGIN_INPUT_FIELD_WIDTH  - (2 * GSC_LOGIN_INPUT_TEXT_WPAD)

#define GSC_LOGIN_INPUT_FIELD_COUNT 2 // number of input fields
#define GSC_LOGIN_INPUT_FULL_HEIGHT \
    GSC_LOGIN_INPUT_FIELD_HEIGHT * GSC_LOGIN_INPUT_FIELD_COUNT \
        + GSC_LOGIN_INPUT_FIELD_GAP * (GSC_LOGIN_INPUT_FIELD_COUNT - 1)

static char* input_contents[2] = { NULL };
static const char* input_placeholders[2] =
{
    "Username / E-mail",
    "Password"
};
bool login_busy = false;

struct gsc_login_layout {
    
};

int gsc_login_draw(SDL_Window* window, SDL_Renderer* renderer, FontPile fonts)
{
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderClear(renderer);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    { // draw input boxes
        int x_left = w/2 - GSC_LOGIN_INPUT_FIELD_WIDTH/2;
        int y_top = h/2 - GSC_LOGIN_INPUT_FULL_HEIGHT/2;

        for (int i = 0; i < GSC_LOGIN_INPUT_FIELD_COUNT; i++)
        {
            roundedBoxRGBA(
                renderer,
                x_left, y_top, // x1, y1
                x_left + GSC_LOGIN_INPUT_FIELD_WIDTH, // x2
                y_top + GSC_LOGIN_INPUT_FIELD_HEIGHT, // y2
                45, // degree
                41, 41, 41, 255 // color
            );

            { // draw text inside input
                char* input_content = input_contents[i];
                SDL_Color content_color = (SDL_Color){255, 255, 255, 255};

                // if content is empty, use placeholder
                if (input_content == NULL || strlen(input_content) == 0)
                {
                    input_content = (char*)input_placeholders[i];
                    content_color = (SDL_Color){255, 255, 255, 127};
                }
                
                // calculate text dimensions
                int fw, fh;
                if (TTF_SizeText(fonts.inter36regular, input_content, &fw, &fh) < 0)
                {
                    log_error("[GRAPHICS] Couldn't calculate size of text \"%s\": %s\n", input_content, TTF_GetError());
                    return -1;
                }

                // calculate the max height of text that fits into the input
                const int text_max_height = GSC_LOGIN_INPUT_FIELD_HEIGHT - 2 * GSC_LOGIN_INPUT_TEXT_HPAD;
                if (fh > text_max_height)
                    log_warn("[GRAPHICS] Input %d's text has a bigger height than it's input! This will cause clipping.\n", i);

                // calculate the vertical offset the text needs in order to be vertically centered
                int text_y_center_offset = text_max_height/2 - fh/2;
                if (text_y_center_offset < 0)
                    text_y_center_offset = 0;

                // mask rectangle put onto window
                SDL_Rect dstrect =
                {
                    // x: input's top + text padding
                    x_left + GSC_LOGIN_INPUT_TEXT_WPAD,
                    // y: input's top + text padding + centering offset
                    y_top + GSC_LOGIN_INPUT_TEXT_HPAD + text_y_center_offset,
                    // w: text's natural width (will be adjusted to handle overflow)
                    fw,
                    // h: text's natural height
                    fh
                };

                // mask rectangle taken from text render
                SDL_Rect srcrect =
                {
                    // x: left of text (will be adjusted to handle overflow)
                    0,
                    // y: top of text
                    0,
                    // w: text's natural width (will be adjusted to handle overflow)
                    fw,
                    // h: text's natural height
                    fh
                };

                const int text_max_width = GSC_LOGIN_INPUT_FIELD_WIDTH - 2 * GSC_LOGIN_INPUT_TEXT_WPAD;

                // if text would horizontally overflow, only show end of text
                if (fw > text_max_width)
                {
                    // clamp rects' width to max width
                    dstrect.w = text_max_width;
                    srcrect.w = text_max_width;

                    // shift text rect's left to only show end and retain aspect ratio
                    srcrect.x += fw - srcrect.w;
                }
                
                if (gfx_text_draw(
                    renderer,
                    fonts.inter36regular,
                    input_content,
                    content_color,
                    &srcrect,
                    &dstrect
                ) < 0)
                    return -1;
            }

            y_top += GSC_LOGIN_INPUT_FIELD_HEIGHT + GSC_LOGIN_INPUT_FIELD_GAP;
        }
    }

    {
        const char* str = "Sign In";
        int fw, fh;
        if (TTF_SizeText(fonts.inter48bold, str, &fw, &fh) < 0)
        {
            log_error("[GRAPHICS] Couldn't calculate size of text \"%s\": %s\n", str, TTF_GetError());
            return -1;
        }

        const int inputs_top = h/2 - GSC_LOGIN_INPUT_FULL_HEIGHT/2;
        int text_top = inputs_top - 75 - fh;
        int text_left = w/2 - fw/2;
        if (gfx_text_draw(
            renderer,
            fonts.inter48bold,
            (char*)str,
            (SDL_Color){ 255, 255, 255, 255 },
            NULL,
            &(SDL_Rect){
                text_left, text_top,
                fw, fh
            }
        ) < 0)
            return -1;
    }

    {
        const char* str = "Sign In";
        int fw, fh;
        if (TTF_SizeText(fonts.inter36bold, str, &fw, &fh) < 0)
        {
            log_error("[GRAPHICS] Couldn't calculate size of text \"%s\": %s\n", str, TTF_GetError());
            return -1;
        }

        const int inputs_top = h/2 - GSC_LOGIN_INPUT_FULL_HEIGHT/2;
        const int inputs_bottom = inputs_top + GSC_LOGIN_INPUT_FULL_HEIGHT;
        int button_top = inputs_bottom + 75 + fh;
        int button_left = w/2 - fw/2 - 40/2;

        char* rstr = (char*)str;
        if (login_busy)
            rstr = "...";
        int rfw, rfh;
        if (TTF_SizeText(fonts.inter36bold, rstr, &rfw, &rfh) < 0)
        {
            log_error("[GRAPHICS] Couldn't calculate size of text \"%s\": %s\n", rstr, TTF_GetError());
            return -1;
        }

        int text_center_offset = (fw + 40) / 2 - rfw / 2;
        
        roundedBoxRGBA(
            renderer,
            button_left, button_top, // x1, y1
            button_left + fw + 40, // x2
            button_top + fh + 40, // y2
            45, // degree
            41, 41, 41, 255 // color
        );

        if (gfx_text_draw(
            renderer,
            fonts.inter48bold,
            rstr,
            (SDL_Color){ 255, 255, 255, 255 },
            NULL,
            &(SDL_Rect){
                button_left + text_center_offset, button_top + 20,
                rfw, rfh
            }
        ) < 0)
            return -1;
    }

    return 0;
}

int gsc_login_handle_touch(SDL_Window* window, SDL_Renderer* renderer, FontPile fonts, int x, int y)
{
    if (login_busy)
        return 0;

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    int inputs_top = h/2 - GSC_LOGIN_INPUT_FULL_HEIGHT/2;
    int inputs_left = w/2 - GSC_LOGIN_INPUT_FIELD_WIDTH/2;
    for (int i = 0; i < GSC_LOGIN_INPUT_FIELD_COUNT; i++)
    {
        if (x < inputs_left || x > inputs_left + GSC_LOGIN_INPUT_FIELD_WIDTH || y < inputs_top || y > inputs_top + GSC_LOGIN_INPUT_FIELD_HEIGHT)
        {
            inputs_top += GSC_LOGIN_INPUT_FIELD_HEIGHT + GSC_LOGIN_INPUT_FIELD_GAP;
            continue;
        }

        Result res;
        SwkbdConfig kbd;

        if (R_FAILED((res = swkbdCreate(&kbd, 0))))
        {
            log_error("[GSC_LOGIN] Failed to create swkbd: %04X\n", res);
            return -1;
        }

        if (i == 0) // username / e-mail field
            swkbdConfigMakePresetDefault(&kbd);
        else // password field
            swkbdConfigMakePresetPassword(&kbd);
        
        char tmp_text[1025] = {0};

        if (input_contents[i] != NULL)
            swkbdConfigSetInitialText(&kbd, input_contents[i]);
        
        swkbdConfigSetGuideText(&kbd, input_placeholders[i]);
        swkbdConfigSetStringLenMax(&kbd, 1024);

        if (R_FAILED((res = swkbdShow(&kbd, tmp_text, 1025))))
        {
            // cancelled by user, probably
            return 0;
        }

        int out_len = strlen(tmp_text);

        if (out_len == 0)
            input_contents[i] = NULL;
        else
        {
            if (input_contents[i] != NULL)
                free(input_contents[i]);
            
            input_contents[i] = malloc(out_len + 1);
            memcpy(input_contents[i], tmp_text, out_len);
            input_contents[i][out_len] = 0;
        }

        return 0;
    }

    {
        const char* str = "Sign In";
        int fw, fh;
        if (TTF_SizeText(fonts.inter36bold, str, &fw, &fh) < 0)
        {
            log_error("[GRAPHICS] Couldn't calculate size of text \"%s\": %s\n", str, TTF_GetError());
            return -1;
        }

        const int inputs_top = h/2 - GSC_LOGIN_INPUT_FULL_HEIGHT/2;
        const int inputs_bottom = inputs_top + GSC_LOGIN_INPUT_FULL_HEIGHT;
        int button_top = inputs_bottom + 75 + fh;
        int button_left = w/2 - fw/2 - 40/2;

        int button_bottom = button_top + fh + 40;
        int button_right = button_left + fw + 40;

        if (x >= button_left && x <= button_right && y >= button_top && y <= button_bottom)
        {
            if (input_contents[0] == NULL || input_contents[1] == NULL)
            {
                return 0;
            }

            log_info("[GRAPHICS] login button pressed!!!!\n");

            login_busy = true;

            authenticate(input_contents[0], input_contents[1]);
        }
    }
    return 0;
}