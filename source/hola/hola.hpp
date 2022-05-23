#pragma once
#include <stdlib.h>
#undef __APPLE__ // intellisense sucks
#include <SDL2/SDL.h>
extern "C"
{
    #include "log.h"
}

typedef enum {
    HOLA_ANCHOR_TOP_LEFT,
    HOLA_ANCHOR_TOP_CENTER,
    HOLA_ANCHOR_TOP_RIGHT,
    HOLA_ANCHOR_CENTER_LEFT,
    HOLA_ANCHOR_CENTER,
    HOLA_ANCHOR_CENTER_RIGHT,
    HOLA_ANCHOR_BOTTOM_LEFT,
    HOLA_ANCHOR_BOTTOM_CENTER,
    HOLA_ANCHOR_BOTTOM_RIGHT,
} HolaAnchor;

// A coordinate with an anchor
typedef struct {
    HolaAnchor anchor;
    int x, y;
} HolaCoords;

// A coordinate without an anchor
// without an anchor = anchored to TOP_LEFT
// To convert a HolaRect to a HolaNormalCoord, use hola_get_normal_coord(rect)
typedef struct {
    int x, y;
} HolaNormalCoord;

// A dimension (width and height of an element)
typedef struct {
    int w, h;
} HolaDimensions;

// A rectangle with an anchor
// To convert a HolaRect to a HolaNormalCoord, use hola_get_normal_coord(rect)
typedef struct {
    HolaCoords pos;
    HolaDimensions dim;
} HolaRect;

typedef enum {
    HOLA_INTERACTION_TOUCH,
    HOLA_INTERACTION_BUTTON
} HolaInteractionType;

// Describes a reported interaction
typedef struct {
    HolaInteractionType type;
    HolaNormalCoord pos;
} HolaInteraction;

// A basic Hola element.
class HolaElement {
    public:
        virtual HolaCoords getPosition() {
            return (HolaCoords){anchor, x, y};
        }
        virtual HolaDimensions getDimensions() {
            return (HolaDimensions){0, 0};
        }
        virtual HolaRect getFootprint() {
            return (HolaRect){getPosition(), getDimensions()};
        }
        
        virtual int onInteract(HolaInteraction e) = 0;
        virtual int render(SDL_Renderer* renderer) = 0;

        HolaElement(HolaCoords pos)
        {
            this->relocate(pos);
        }

        void relocate(HolaCoords pos)
        {
            this->anchor = pos.anchor;
            this->x = pos.x;
            this->y = pos.y;
        }

    protected:
        HolaAnchor anchor;
        int x, y;
};

// Static Hola elements are elements which have a predefined width and height.
class HolaStaticElement: public HolaElement {
    public:
        virtual HolaDimensions getDimensions() {
            return (HolaDimensions){w, h};
        }

        HolaStaticElement(HolaRect footprint) : HolaElement(footprint.pos)
        {
            resize(footprint.dim);
        }

        void resize(HolaDimensions dim)
        {
            this->w = dim.w;
            this->h = dim.h;
        }

    protected:
        int w, h;
};

void hola_set_scene(HolaElement* scene);
int hola_interact(HolaInteraction interaction);
int hola_update(SDL_Renderer* renderer);
void hola_clear_scene();