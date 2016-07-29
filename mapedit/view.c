#include <SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "mapedit/canvas.h"
#include "mapedit/geometry.h"
#include "mapedit/view.h"

fpoint camera_centre = { 0, 0 };
const float camera_unitpx = 128;

static const float zoom_levels[] = {
    1.0f/16, 1.0f/12, 1.0f/8, 1.0f/6, 1.0f/4, 1.0f/3, 1.0f/2,
    1.0f, /* index 7 */
    2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f,
};
static const unsigned view_default_zoom = 7;
static const unsigned view_min_zoom = 0;
static const unsigned view_max_zoom = 14;
static unsigned view_zoom = view_default_zoom;

static SDL_Point camera_offset = { 0, 0 };
static SDL_Renderer *view_renderer = NULL;
static SDL_Texture *texture = NULL;
static int is_view_dirty = 0;

extern struct vertex *verts;
extern size_t verts_alloc;
extern size_t verts_count;
extern struct node *nodes;
extern size_t nodes_alloc;
extern size_t nodes_count;
extern int is_data_dirty;

void view_init(SDL_Renderer *renderer)
{
    camera_offset.x = camera_offset.y = 0;
    camera_centre.x = camera_centre.y = 0;
    view_renderer = renderer;
    view_zoom = view_default_zoom;
    is_view_dirty = 0;
    texture = NULL;

    view_update();
}

void view_destroy(void)
{
    camera_offset.x = camera_offset.y = 0;
    camera_centre.x = camera_centre.y = 0;
    view_renderer = NULL;
    is_view_dirty = 0;

    if (texture) SDL_DestroyTexture(texture);
    texture = NULL;
}

void view_update(void)
{
    SDL_Rect viewport;

    if (!view_renderer) return;

    SDL_RenderGetViewport(view_renderer, &viewport);

    camera_offset.x = scalar_to_screen(camera_centre.x) - (viewport.w / 2);
    camera_offset.y = scalar_to_screen(camera_centre.y) - (viewport.h / 2);

    is_view_dirty = 1;
}

int scalar_to_screen(float f)
{
    return lroundf(f * zoom_levels[view_zoom] * camera_unitpx);
}

float scalar_from_screen(int i)
{
    return i / (zoom_levels[view_zoom] * camera_unitpx);
}

SDL_Point vectorxy_to_screen(float x, float y)
{
    SDL_Point screen = {
        scalar_to_screen(x),
        scalar_to_screen(y),
    };
    return screen;
}

fvector vector_from_screenxy(int x, int y)
{
    fvector v = {
        scalar_from_screen(x),
        scalar_from_screen(y),
    };
    return v;
}

SDL_Point pointxy_to_screen(float x, float y)
{
    return psubtractp(vectorxy_to_screen(x, y), camera_offset);
}

fpoint point_from_screenxy(int x, int y)
{
    float div = zoom_levels[view_zoom] * camera_unitpx;
    fpoint p = {
        (x + camera_offset.x) / div,
        (y + camera_offset.y) / div
    };
    return p;
}

void view_zoom_in(void)
{
    if (!view_renderer) return;

    if (view_zoom < view_max_zoom) {
        view_zoom ++;
        view_update();
    }
}

void view_zoom_out(void)
{
    if (!view_renderer) return;

    if (view_zoom > view_min_zoom) {
        view_zoom --;
        view_update();
    }
}

static void view_move(int x, int y, unsigned flipped)
{
    float x2, y2;

    if (!view_renderer) return;

    if (flipped) { x = -x; y = -y; }
    x2 = 0.5f * x * x;
    y2 = 0.5f * y * y;
    camera_centre.x -= scalar_from_screen(copysignf(x2, x));
    camera_centre.y += scalar_from_screen(copysignf(y2, y));

    view_update();
}

int view_handle_event(const SDL_Event *e)
{
    switch (e->type) {
        case SDL_WINDOWEVENT:
            if (e->window.event != SDL_WINDOWEVENT_SIZE_CHANGED) break;
            view_update();
            break;
        case SDL_MOUSEWHEEL:
            view_move(e->wheel.x, e->wheel.y, e->wheel.direction);
            break;
        case SDL_KEYDOWN:
            if (e->key.keysym.sym == SDLK_z) {
                if ((SDL_GetModState() & KMOD_SHIFT))
                    view_zoom_out();
                else
                    view_zoom_in();
                return 1;
            }
            break;
    }

    return 0;
}

void view_render(SDL_Renderer *renderer)
{
    if (is_view_dirty || !texture) {
        SDL_Rect viewport;
        node_id i;
        fpoint tl, br;
        float x, y;
        const float grid_subdiv = 0.25f;
        char buf[16];

        SDL_RenderGetViewport(renderer, &viewport);

        if (texture) SDL_DestroyTexture(texture);

        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    viewport.w, viewport.h);

        SDL_SetRenderTarget(renderer, texture);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (i = 0; i < nodes_count; i++) {
            const struct node *n = &nodes[i];
            if (n->id == ID_NONE) continue;

            SDL_Point points[3] = {
                point_to_screen(verts[n->v[0]].p),
                point_to_screen(verts[n->v[1]].p),
                point_to_screen(verts[n->v[2]].p),
            };

            filledTrigonRGBA(renderer,
                             points[0].x, points[0].y,
                             points[1].x, points[1].y,
                             points[2].x, points[2].y,
                             80, 80, 80, 255);
            trigonRGBA(renderer,
                       points[0].x, points[0].y,
                       points[1].x, points[1].y,
                       points[2].x, points[2].y,
                       120, 120, 120, 255);
        }

        tl = point_from_screenxy(viewport.x, viewport.y);
        br = point_from_screenxy(viewport.x + viewport.w, viewport.y + viewport.h);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        for (y = floorf(tl.y) - grid_subdiv;
             y <= ceilf(br.y) + grid_subdiv;
             y += grid_subdiv) {
            SDL_Point start, end;

            start = pointxy_to_screen(tl.x, y);
            end = pointxy_to_screen(br.x, y);

            if (truncf(y) == y) {
                snprintf(buf, sizeof(buf), "%.0f", y);
                stringRGBA(renderer, start.x + 2, start.y - 10, buf, 0, 255, 255, 64);

                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 48);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 32);
            }

            SDL_RenderDrawLine(renderer,
                               start.x, start.y,
                               end.x, end.y);

        }

        for (x = floorf(tl.x) - grid_subdiv;
             x <= ceilf(br.x) + grid_subdiv;
             x += grid_subdiv) {
            SDL_Point start, end;

            start = pointxy_to_screen(x, tl.y);
            end = pointxy_to_screen(x, br.y);

            if (truncf(x) == x) {
                snprintf(buf, sizeof(buf), "%.0f", x);
                stringRGBA(renderer, start.x + 2, start.y + 2, buf, 0, 255, 255, 64);
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 48);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 32);
            }

            SDL_RenderDrawLine(renderer,
                               start.x, start.y,
                               end.x, end.y);
        }

        SDL_SetRenderTarget(renderer, NULL);
        is_view_dirty = 0;
    }

    if (texture)
        SDL_RenderCopy(renderer, texture, NULL, NULL);
}
