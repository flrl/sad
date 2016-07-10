#include <SDL.h>

#include "mapedit/camera.h"

SDL_Point camera_offset = { 0, 0 };
SDL_Point camera_centre = { 0, 0 };

static SDL_Renderer *camera_renderer = NULL;

void camera_init(SDL_Renderer *renderer)
{
    SDL_Rect viewport;

    camera_centre.x = camera_centre.y = 0;
    camera_renderer = renderer;

    SDL_RenderGetViewport(camera_renderer, &viewport);

    camera_offset.x = camera_centre.x - (viewport.w / 2);
    camera_offset.y = camera_centre.y - (viewport.h / 2);
}

void camera_destroy(void)
{
    camera_offset.x = camera_offset.y = 0;
    camera_centre.x = camera_centre.y = 0;
    camera_renderer = NULL;
}

int camera_handle_event(const SDL_Event *e)
{
    return 0;
}

SDL_Point add(SDL_Point a, SDL_Point b)
{
    SDL_Point result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

SDL_Point subtract(const SDL_Point *a, const SDL_Point *b)
{
    SDL_Point tmp;
    tmp.x = a->x - b->x;
    tmp.y = a->y - b->y;
    return tmp;
}
