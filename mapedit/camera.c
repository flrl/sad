#include <SDL.h>

#include "mapedit/camera.h"
#include "mapedit/geometry.h"

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

SDL_Point to_screen(fpoint p)
{
    SDL_Point screen;
    screen.x = lroundf(p.x) - camera_offset.x;
    screen.y = lroundf(p.y) - camera_offset.y;
    return screen;
}

fpoint from_screen(SDL_Point screen)
{
    fpoint p;
    p.x = screen.x + camera_offset.x;
    p.y = screen.y + camera_offset.y;
    return p;
}

static void camera_move(int x, int y, unsigned flipped)
{
    SDL_Rect viewport;

    if (!camera_renderer) return;

    if (flipped) { x = -x; y = -y; }
    camera_centre.x -= x;
    camera_centre.y += y;

    SDL_RenderGetViewport(camera_renderer, &viewport);

    camera_offset.x = camera_centre.x - (viewport.w / 2);
    camera_offset.y = camera_centre.y - (viewport.h / 2);
}

int camera_handle_event(const SDL_Event *e)
{
    switch (e->type) {
        case SDL_WINDOWEVENT:
            if (e->window.event != SDL_WINDOWEVENT_SIZE_CHANGED) break;
            camera_offset.x = camera_centre.x - (e->window.data1 / 2);
            camera_offset.y = camera_centre.y - (e->window.data2 / 2);
            break;
        case SDL_MOUSEWHEEL:
            camera_move(e->wheel.x, e->wheel.y, e->wheel.direction);
            break;
    }

    return 0;
}
