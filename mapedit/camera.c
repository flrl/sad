#include <SDL.h>

#include "mapedit/camera.h"
#include "mapedit/geometry.h"

fpoint camera_centre = { 0, 0 };
const float camera_unitpx = 128;

static SDL_Point camera_offset = { 0, 0 };
static SDL_Renderer *camera_renderer = NULL;

static void camera_update_offset(void);

void camera_init(SDL_Renderer *renderer)
{
    camera_centre.x = camera_centre.y = 0;
    camera_renderer = renderer;

    camera_update_offset();
}

void camera_destroy(void)
{
    camera_offset.x = camera_offset.y = 0;
    camera_centre.x = camera_centre.y = 0;
    camera_renderer = NULL;
}

static void camera_update_offset(void)
{
    SDL_Rect viewport;

    if (!camera_renderer) return;

    SDL_RenderGetViewport(camera_renderer, &viewport);

    camera_offset.x = lroundf(camera_unitpx * camera_centre.x) - (viewport.w / 2);
    camera_offset.y = lroundf(camera_unitpx * camera_centre.y) - (viewport.h / 2);
}

SDL_Point to_screen(fpoint p)
{
    SDL_Point screen;
    screen.x = lroundf(camera_unitpx * p.x) - camera_offset.x;
    screen.y = lroundf(camera_unitpx * p.y) - camera_offset.y;
    return screen;
}

fpoint from_screen(SDL_Point screen)
{
    fpoint p;
    p.x = (screen.x + camera_offset.x) / camera_unitpx;
    p.y = (screen.y + camera_offset.y) / camera_unitpx;
    return p;
}

static void camera_move(int x, int y, unsigned flipped)
{
    if (!camera_renderer) return;

    if (flipped) { x = -x; y = -y; }
    camera_centre.x -= (x / camera_unitpx);
    camera_centre.y += (y / camera_unitpx);

    camera_update_offset();
}

int camera_handle_event(const SDL_Event *e)
{
    switch (e->type) {
        case SDL_WINDOWEVENT:
            if (e->window.event != SDL_WINDOWEVENT_SIZE_CHANGED) break;
            camera_update_offset();
            break;
        case SDL_MOUSEWHEEL:
            camera_move(e->wheel.x, e->wheel.y, e->wheel.direction);
            break;
    }

    return 0;
}
