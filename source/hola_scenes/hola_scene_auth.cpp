#include "hola_scene_auth.hpp"
#include "../hola/hola_util.hpp"
#include "hola_scene_demo.hpp"
extern "C"
{
    #include "../spotifymgr.h"
    #include "log.h"
}

int HolaSceneAuth::onInteract(HolaInteraction e)
{
    HOLA_INTERACT_PT(this->title);
    HOLA_INTERACT_PT(this->username);
    HOLA_INTERACT_PT(this->password);
    HOLA_INTERACT_PT(this->button);

    return 0;
}

int HolaSceneAuth::onAuthenticated()
{
    log_info("onAuthenticated\n");
    hola_set_scene(new HolaSceneDemo(this->getFootprint()));
    return 0;
}

int HolaSceneAuth::onButtonPressed()
{
    if (this->username.getContent() == NULL || this->password.getContent() == NULL)
        return 0;

    log_info("Button was pressed!\n");
    this->button.setRealContent((char*)"...");
    spotifymgr_authenticate(this->username.getContent(), this->password.getContent(), onAuthenticated_forwarder, this);

    return 0;
}

int HolaSceneAuth::render(SDL_Renderer* renderer)
{
    HOLA_RENDER_CLEAR(14, 14, 14);
    
    HOLA_RENDER_PT(this->title);
    HOLA_RENDER_PT(this->username);
    HOLA_RENDER_PT(this->password);
    HOLA_RENDER_PT(this->button);
    
    return 0;
}