#pragma once
#include <stdlib.h>
#undef __APPLE__ // intellisense sucks
#include <SDL2/SDL.h>
#include "SDL2_gfx/SDL2_gfxPrimitives.h"
#include "../graphics.h"

int gsc_login_draw(SDL_Window* window, SDL_Renderer* renderer, FontPile fonts);
int gsc_login_handle_touch(SDL_Window* window, SDL_Renderer* renderer, FontPile fonts, int x, int y);