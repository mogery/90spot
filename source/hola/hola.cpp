#include "hola.hpp"

HolaElement* current_scene = NULL;

void hola_set_scene(HolaElement* scene)
{
    if (current_scene != NULL)
        free(current_scene);

    current_scene = scene;
}

int hola_interact(HolaInteraction interaction)
{
    if (current_scene == NULL)
        return 0;
    
    return current_scene->onInteract(interaction);
}

int hola_update(SDL_Renderer* renderer)
{
    if (current_scene == NULL)
        return 0;
    
    return current_scene->render(renderer);
}

void hola_clear_scene()
{
    if (current_scene != NULL)
        free(current_scene);
    
    current_scene = NULL;
}