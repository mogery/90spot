#include "hola_input.hpp"
#include "hola_util.hpp"

extern "C"
{
    #include <switch/applets/swkbd.h>
}

int HolaInputElement::onInteract(HolaInteraction e)
{
    SwkbdConfig kbd;
    char tmpoutstr[1025] = {0};
    swkbdCreate(&kbd, 0);

    if (this->type == HOLA_INPUT_PASSWORD)
        swkbdConfigMakePresetPassword(&kbd);
    else
        swkbdConfigMakePresetDefault(&kbd);
    
    if (this->placeholder != NULL)
        swkbdConfigSetGuideText(&kbd, this->placeholder);

    if (R_FAILED(swkbdShow(&kbd, tmpoutstr, 1025))) // cancelled by user
        return 0;

    size_t out_len = strlen(tmpoutstr);

    if (this->content != NULL)
        free(this->content);

    if (out_len == 0)
        this->content = NULL;
    else
    {
        this->content = (char*)malloc(out_len + 1);
        memcpy(this->content, tmpoutstr, out_len);
        this->content[out_len] = 0;
    }

    return 0;
}

int HolaInputElement::render(SDL_Renderer* renderer)
{
    if (this->password_buf != NULL)
        free(this->password_buf);

    HolaNormalCoord npos = hola_get_normal_coord(getFootprint());
    HolaDimensions dim = getDimensions();

    if (roundedBoxRGBA(
        renderer,
        npos.x, npos.y,
        npos.x + dim.w,
        npos.y + dim.h,
        45,
        41, 41, 41, 255
    ) < 0)
        return -1;

    if ((this->content == NULL || strlen(this->content) == 0) && (this->placeholder != NULL && strlen(this->placeholder) != 0))
    {
        this->text.setContent(this->placeholder);
        this->text.setColor((SDL_Color){255, 255, 255, 127});
    }
    else
    {
        if (this->content == NULL)
            this->text.setContent((char*)"");
        else
        {
            if (this->type == HOLA_INPUT_PASSWORD)
            {
                // Conceal password, show asterisks instead.
                int content_len = strlen(this->content);
                this->password_buf = (char*)malloc(content_len + 1);
                for (int i = 0; i < content_len; i++)
                    this->password_buf[i] = '*';
                
                this->password_buf[content_len] = 0;
                this->text.setContent(this->password_buf);
            }
            else
                this->text.setContent(this->content);
        }

        this->text.setColor((SDL_Color){255, 255, 255, 255});
    }

    this->text.setMaxSize((HolaDimensions){ dim.w - 40, dim.h - 40 });
    this->text.relocate((HolaCoords){
        HOLA_ANCHOR_TOP_LEFT,
        npos.x + 20,
        npos.y + 20
    });

    return this->text.render(renderer);
}