#include <assert.h>

#include <SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "mapedit/canvas.h"
#include "mapedit/colour.h"
#include "mapedit/geometry.h"
#include "mapedit/view.h"

const double camera_unitpx = 128;

static const double zoom_levels[] = {
    1.0/16, 1.0/8, 3.0/16, 1.0/4, 1.0/2, 3.0/4,
    1.0, /* index 6 */
    5.0/4, 3.0/2, 7.0/4, 2.0, 5.0/2, 3.0, 7.0/2, 4.0, 6.0, 8.0, 12.0, 16.0,
};
static const unsigned view_default_zoom = 6;
static const unsigned view_min_zoom = 0;
static const unsigned view_max_zoom = sizeof(zoom_levels) / sizeof(zoom_levels[0]) - 1;

static struct {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Point camera_offset;
    fpoint camera_centre;
    double grid_step;
    int show_grid;
    int show_mouse_pos;
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
    view.grid_step = 1.0;
    view.show_mouse_pos = 1;

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

    while (subdiv <= 16 && scalar_to_screen(1.0 / (subdiv * 2)) >= 24)
        subdiv = subdiv * 2;
    view.grid_step = 1.0 / subdiv;

    view.is_dirty = 1;
}

int scalar_to_screen(double d)
{
    return lround(d * zoom_levels[view.zoom] * camera_unitpx);
}

double scalar_from_screen(double d)
{
    return d / (zoom_levels[view.zoom] * camera_unitpx);
}

SDL_Point vectorxy_to_screen(double x, double y)
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

SDL_Point pointxy_to_screen(double x, double y)
{
    return psubtractp(vectorxy_to_screen(x, y), view.camera_offset);
}

fpoint point_from_screenxy(int x, int y)
{
    double div = zoom_levels[view.zoom] * camera_unitpx;
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

void view_toggle_mouse_pos(void)
{
    view.show_mouse_pos = !view.show_mouse_pos;
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

fpoint view_find_gridpoint_near(fpoint p, double snap)
{
    double x, y;
    if (!view.show_grid) return p;

    assert(snap >= 0);

    for (y = floor(p.y); y <= ceil(p.y); y += view.grid_step) {
        for (x = floor(p.x); x <= ceil(p.x); x += view.grid_step) {
            fpoint tmp = { x, y };

            if (lengthfv(subtractfp(p, tmp)) <= snap)
                return tmp;
        }
    }

    return p;
}

static void view_move(int x, int y, unsigned flipped)
{
    double x2, y2;

    if (!view.renderer) return;

    if (flipped) { x = -x; y = -y; }
    x2 = 0.5 * x * x;
    y2 = 0.5 * y * y;
    view.camera_centre.x -= scalar_from_screen(copysign(x2, x));
    view.camera_centre.y += scalar_from_screen(copysign(y2, y));

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
                if ((SDL_GetModState() & KMOD_SHIFT))
                    view_toggle_mouse_pos();
                else
                    view_toggle_grid();
                return 1;
            }
            break;
    }

    return 0;
}

void view_render(SDL_Renderer *renderer)
{
    SDL_Rect viewport;
    SDL_RenderGetViewport(renderer, &viewport);

    if (view.is_dirty || !view.texture) {
        node_id i;

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
            double x, y;
            fpoint tl, br;
            char buf[16];

            tl = point_from_screenxy(viewport.x, viewport.y);
            br = point_from_screenxy(viewport.x + viewport.w, viewport.y + viewport.h);

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

            for (y = floor(tl.y) - view.grid_step;
                 y <= ceil(br.y) + view.grid_step;
                 y += view.grid_step) {
                SDL_Point start, end;

                start = pointxy_to_screen(tl.x, y);
                end = pointxy_to_screen(br.x, y);

                if (trunc(y) == y) {
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

            for (x = floor(tl.x) - view.grid_step;
                 x <= ceil(br.x) + view.grid_step;
                 x += view.grid_step) {
                SDL_Point start, end;

                start = pointxy_to_screen(x, tl.y);
                end = pointxy_to_screen(x, br.y);

                if (trunc(x) == x) {
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

    if (view.show_mouse_pos) {
        int x, y, n;
        fpoint mouse;
        char buf[16];

        SDL_GetMouseState(&x, &y);
        mouse = point_from_screenxy(x, y);

        snprintf(buf, sizeof(buf), "%.6f%n", mouse.x, &n);
        stringRGBA(renderer, viewport.w - (n * 8), viewport.h - 16, buf, C(view_mouse_pos));
        snprintf(buf, sizeof(buf), "%.6f%n", mouse.y, &n);
        stringRGBA(renderer, viewport.w - (n * 8), viewport.h - 8, buf, C(view_mouse_pos));
    }
}
