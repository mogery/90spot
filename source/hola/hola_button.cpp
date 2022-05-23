#include "hola_button.hpp"
#include "hola_util.hpp"

int HolaButtonElement::onInteract(HolaInteraction e)
{
    if (this->onPress != NULL)
        return this->onPress(this->onPress_state);

    return 0;
}

int HolaButtonElement::render(SDL_Renderer* renderer)
{
    if (this->content == NULL)
        this->text.setContent((char*)"");
    else
        this->text.setContent(this->content);

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

    this->text.relocate((HolaCoords){
        HOLA_ANCHOR_CENTER,
        npos.x + dim.w / 2,
        npos.y + dim.h / 2
    });

    if (this->real_content != NULL && strlen(this->real_content) != 0)
        this->text.setContent(this->real_content);

    return this->text.render(renderer);
}