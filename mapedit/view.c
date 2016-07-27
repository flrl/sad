#include <SDL.h>

#include "mapedit/geometry.h"
#include "mapedit/view.h"

fpoint camera_centre = { 0, 0 };
const float camera_unitpx = 128;
float camera_zoom = 1;

static SDL_Point camera_offset = { 0, 0 };
static SDL_Renderer *view_renderer = NULL;
static const float camera_max_zoom = 16.0;
static const float camera_min_zoom = 1.0 / 16.0;

static void view_update_offset(void);

void view_init(SDL_Renderer *renderer)
{
    camera_centre.x = camera_centre.y = 0;
    view_renderer = renderer;

    view_update_offset();
}

void view_destroy(void)
{
    camera_offset.x = camera_offset.y = 0;
    camera_centre.x = camera_centre.y = 0;
    view_renderer = NULL;
}

static void view_update_offset(void)
{
    SDL_Rect viewport;
    float mult = camera_zoom * camera_unitpx;

    if (!view_renderer) return;

    SDL_RenderGetViewport(view_renderer, &viewport);

    camera_offset.x = lroundf(mult * camera_centre.x) - (viewport.w / 2);
    camera_offset.y = lroundf(mult * camera_centre.y) - (viewport.h / 2);
}

SDL_Point to_screen(fpoint p)
{
    SDL_Point screen;
    float mult = camera_zoom * camera_unitpx;
    screen.x = lroundf(mult * p.x) - camera_offset.x;
    screen.y = lroundf(mult * p.y) - camera_offset.y;
    return screen;
}

fpoint from_screen(SDL_Point screen)
{
    fpoint p;
    float div = camera_zoom * camera_unitpx;
    p.x = (screen.x + camera_offset.x) / div;
    p.y = (screen.y + camera_offset.y) / div;
    return p;
}

void view_zoom_in(void)
{
    if (!view_renderer) return;

    camera_zoom = 2.0 * camera_zoom;

    if (camera_zoom > camera_max_zoom)
        camera_zoom = camera_max_zoom;

    view_update_offset();
}

void view_zoom_out(void)
{
    if (!view_renderer) return;

    camera_zoom = 0.5 * camera_zoom;

    if (camera_zoom < camera_min_zoom)
        camera_zoom = camera_min_zoom;

    view_update_offset();
}

static void view_move(float x, float y, unsigned flipped)
{
    float div = camera_zoom * camera_unitpx;
    float x2, y2;

    if (!view_renderer) return;

    if (flipped) { x = -x; y = -y; }
    x2 = 0.5f * x * x;
    y2 = 0.5f * y * y;
    camera_centre.x -= (copysignf(x2, x) / div);
    camera_centre.y += (copysignf(y2, y) / div);

    view_update_offset();
}

int view_handle_event(const SDL_Event *e)
{
    switch (e->type) {
        case SDL_WINDOWEVENT:
            if (e->window.event != SDL_WINDOWEVENT_SIZE_CHANGED) break;
            view_update_offset();
            break;
        case SDL_MOUSEWHEEL:
            view_move(e->wheel.x, e->wheel.y, e->wheel.direction);
            break;
    }

    return 0;
}
