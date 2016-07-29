#include <SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "mapedit/canvas.h"
#include "mapedit/colour.h"
#include "mapedit/geometry.h"
#include "mapedit/view.h"

const float camera_unitpx = 128;

static const float zoom_levels[] = {
    1.0f/16, 1.0f/12, 1.0f/8, 1.0f/6, 1.0f/4, 1.0f/3, 1.0f/2, 0.75f,
    1.0f, /* index 8 */
    1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f,
};
static const unsigned view_default_zoom = 8;
static const unsigned view_min_zoom = 0;
static const unsigned view_max_zoom = 18;

static struct {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Point camera_offset;
    fpoint camera_centre;
    float grid_step;
    int show_grid;
    unsigned zoom;
    int is_dirty;
} view;

extern struct vertex *verts;
extern size_t verts_alloc;
extern size_t verts_count;
extern struct node *nodes;
extern size_t nodes_alloc;
extern size_t nodes_count;
extern int is_data_dirty;

void view_init(SDL_Renderer *renderer)
{
    memset(&view, 0, sizeof(view));
    view.renderer = renderer;
    view.zoom = view_default_zoom;
    view.show_grid = 1;
    view.grid_step = 1.0f;

    view_update();
}

void view_destroy(void)
{
    if (view.texture) SDL_DestroyTexture(view.texture);

    memset(&view, 0, sizeof(view));
}

void view_update(void)
{
    SDL_Rect viewport;
    int subdiv = 1;

    if (!view.renderer) return;

    SDL_RenderGetViewport(view.renderer, &viewport);

    view.camera_offset.x = scalar_to_screen(view.camera_centre.x) - (viewport.w / 2);
    view.camera_offset.y = scalar_to_screen(view.camera_centre.y) - (viewport.h / 2);

    while (subdiv <= 16 && scalar_to_screen(1.0f / (subdiv * 2)) >= 24)
        subdiv = subdiv * 2;
    view.grid_step = 1.0f / subdiv;

    view.is_dirty = 1;
}

int scalar_to_screen(float f)
{
    return lroundf(f * zoom_levels[view.zoom] * camera_unitpx);
}

float scalar_from_screen(int i)
{
    return i / (zoom_levels[view.zoom] * camera_unitpx);
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
    return psubtractp(vectorxy_to_screen(x, y), view.camera_offset);
}

fpoint point_from_screenxy(int x, int y)
{
    float div = zoom_levels[view.zoom] * camera_unitpx;
    fpoint p = {
        (x + view.camera_offset.x) / div,
        (y + view.camera_offset.y) / div
    };
    return p;
}

void view_toggle_grid(void)
{
    view.show_grid = !view.show_grid;
    view_update();
}

void view_zoom_in(void)
{
    if (!view.renderer) return;

    if (view.zoom < view_max_zoom) {
        view.zoom ++;
        view_update();
    }
}

void view_zoom_out(void)
{
    if (!view.renderer) return;

    if (view.zoom > view_min_zoom) {
        view.zoom --;
        view_update();
    }
}

static void view_move(int x, int y, unsigned flipped)
{
    float x2, y2;

    if (!view.renderer) return;

    if (flipped) { x = -x; y = -y; }
    x2 = 0.5f * x * x;
    y2 = 0.5f * y * y;
    view.camera_centre.x -= scalar_from_screen(copysignf(x2, x));
    view.camera_centre.y += scalar_from_screen(copysignf(y2, y));

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
                view_zoom_in();
                return 1;
            }
            else if (e->key.keysym.sym == SDLK_x) {
                view_zoom_out();
                return 1;
            }
            else if (e->key.keysym.sym == SDLK_g) {
                view_toggle_grid();
                return 1;
            }
            break;
    }

    return 0;
}

void view_render(SDL_Renderer *renderer)
{
    if (view.is_dirty || !view.texture) {
        SDL_Rect viewport;
        node_id i;
        fpoint tl, br;
        float x, y;
        char buf[16];

        SDL_RenderGetViewport(renderer, &viewport);

        if (view.texture) SDL_DestroyTexture(view.texture);

        view.texture = SDL_CreateTexture(renderer,
                                               SDL_PIXELFORMAT_RGBA8888,
                                               SDL_TEXTUREACCESS_TARGET,
                                               viewport.w, viewport.h);

        SDL_SetRenderTarget(renderer, view.texture);

        SDL_SetRenderDrawColor(renderer, C(view_background));
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
                             C(view_node));
            trigonRGBA(renderer,
                       points[0].x, points[0].y,
                       points[1].x, points[1].y,
                       points[2].x, points[2].y,
                       C(view_edge));
        }

        if (view.show_grid) {
            tl = point_from_screenxy(viewport.x, viewport.y);
            br = point_from_screenxy(viewport.x + viewport.w, viewport.y + viewport.h);

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

            for (y = floorf(tl.y) - view.grid_step;
                 y <= ceilf(br.y) + view.grid_step;
                 y += view.grid_step) {
                SDL_Point start, end;

                start = pointxy_to_screen(tl.x, y);
                end = pointxy_to_screen(br.x, y);

                if (truncf(y) == y) {
                    snprintf(buf, sizeof(buf), "%.0f", y);
                    stringRGBA(renderer, start.x + 2, start.y - 10, buf, C(view_grid_coord));

                    SDL_SetRenderDrawColor(renderer, C(view_grid_major));
                }
                else {
                    SDL_SetRenderDrawColor(renderer, C(view_grid_minor));
                }

                SDL_RenderDrawLine(renderer,
                                start.x, start.y,
                                end.x, end.y);

            }

            for (x = floorf(tl.x) - view.grid_step;
                 x <= ceilf(br.x) + view.grid_step;
                 x += view.grid_step) {
                SDL_Point start, end;

                start = pointxy_to_screen(x, tl.y);
                end = pointxy_to_screen(x, br.y);

                if (truncf(x) == x) {
                    snprintf(buf, sizeof(buf), "%.0f", x);
                    stringRGBA(renderer, start.x + 2, start.y + 2, buf, C(view_grid_coord));
                    SDL_SetRenderDrawColor(renderer, C(view_grid_major));
                }
                else {
                    SDL_SetRenderDrawColor(renderer, C(view_grid_minor));
                }

                SDL_RenderDrawLine(renderer,
                                start.x, start.y,
                                end.x, end.y);
            }
        }

        SDL_SetRenderTarget(renderer, NULL);
        view.is_dirty = 0;
    }

    if (view.texture)
        SDL_RenderCopy(renderer, view.texture, NULL, NULL);
}
