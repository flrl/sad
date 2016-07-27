#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <SDL2_gfxPrimitives.h>

#include "mapedit/canvas.h"
#include "mapedit/geometry.h"
#include "mapedit/prompt.h"
#include "mapedit/tools.h"
#include "mapedit/view.h"

#define TOOL_SNAP (5 / camera_unitpx)
#define TOOL_SNAP2 (TOOL_SNAP * TOOL_SNAP)

/*** nodedraw ***/
static struct nodedraw_state {
    fpoint points[3];
    unsigned n_points;
} nodedraw_state;

static void nodedraw_reset(void)
{
    struct nodedraw_state *state = &nodedraw_state;

    memset(state, 0, sizeof *state);
}

static void nodedraw_start(fpoint p)
{
    struct nodedraw_state *state = &nodedraw_state;

    assert(state->n_points == 0);

    state->points[state->n_points++] = p;
}

static void nodedraw_next_point(fpoint p)
{
    struct nodedraw_state *state = &nodedraw_state;
    vertex_id vid[3];
    size_t i;

    assert(state->n_points > 0);

    if (state->n_points >= 3) {
        for (i = 0; i < 3; i++) {
            vid[i] = canvas_find_vertex_near(state->points[i], 0, NULL);
            if (vid[i] == ID_NONE)
                vid[i] = canvas_add_vertex(state->points[i]);
        }
        canvas_add_node(vid);
        nodedraw_reset();
    }
    else {
        state->points[state->n_points++] = p;
    }
}

static void nodedraw_edit_point(const fpoint *p_abs, const SDL_Point *p_rel)
{
    struct nodedraw_state *state = &nodedraw_state;

    assert(state->n_points > 0);

    if (state->n_points > 3)
        return;

    if (p_abs) {
        state->points[state->n_points - 1] = *p_abs;
    }
    else if (p_rel) {
        SDL_Point p = to_screen(state->points[state->n_points -1]);
        p.x += p_rel->x;
        p.y += p_rel->y;
        state->points[state->n_points - 1] = from_screen(p);
    }
}

static void mouse_rotsnap(fpoint a, fpoint b, fpoint *out)
{
    const static fvector directions[8] = {
        {        1.0f,  0.0f       },   // east
        {  INV_SQRT2f,  INV_SQRT2f },   // northeast
        {        0.0f,  1.0f       },   // north
        { -INV_SQRT2f,  INV_SQRT2f },   // northwest
        {       -1.0f,  0.0f       },   // west
        { -INV_SQRT2f, -INV_SQRT2f },   // southwest
        {        0.0f, -1.0f       },   // south
        {  INV_SQRT2f, -INV_SQRT2f },   // southeast
    };
    const static size_t n_directions = 8;
    fvector vab = subtractfp(a, b); // FIXME ?
    const float vab_len = lengthfv(vab);
    size_t best_direction = 0;
    float best_result = 0.0f;
    size_t i;

    assert(out != NULL);

    for (i = 0; i < n_directions; i++) {
        float quality = dotfv(directions[i], vab);
        if (quality < best_result) {
            best_result = quality;
            best_direction = i;
        }
    }

    *out = addfp(a, scalefv(directions[best_direction], vab_len));
}

static int nodedraw_handle_event(const SDL_Event *e)
{
    struct nodedraw_state *state = &nodedraw_state;
    SDL_Point tmp;
    SDL_Point arrows;
    SDL_Keymod keymod;
    fpoint mouse;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = from_screen(tmp);

    keymod = SDL_GetModState();
    if ((keymod & KMOD_SHIFT) && state->n_points > 1)
        mouse_rotsnap(state->points[state->n_points - 2],
                      mouse, &mouse);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->n_points == 0) break;
            if (e->key.keysym.sym == SDLK_ESCAPE)
                nodedraw_reset();
            else if (e->key.keysym.sym == SDLK_RETURN)
                nodedraw_next_point(mouse);
            break;
        case SDL_KEYDOWN:
            if (state->n_points == 0) break;
            arrows.x = arrows.y = 0;
            if (e->key.keysym.sym == SDLK_DOWN) arrows.y ++;
            else if (e->key.keysym.sym == SDLK_UP) arrows.y --;
            else if (e->key.keysym.sym == SDLK_LEFT) arrows.x --;
            else if (e->key.keysym.sym == SDLK_RIGHT) arrows.x ++;
            else break;
            nodedraw_edit_point(NULL, &arrows);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points > 0) break;
            canvas_find_vertex_near(mouse, TOOL_SNAP2, &mouse);
            nodedraw_start(mouse);
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0) break;
            canvas_find_vertex_near(mouse, TOOL_SNAP2, &mouse);
            nodedraw_edit_point(&mouse, NULL);
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button == SDL_BUTTON_RIGHT)
                nodedraw_reset();
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points == 0) break;
            canvas_find_vertex_near(mouse, TOOL_SNAP2, &mouse);
            nodedraw_next_point(mouse);
            break;
    }

    return 0;
}

static void nodedraw_render(SDL_Renderer *renderer)
{
    struct nodedraw_state *state = &nodedraw_state;
    SDL_Point points[4];
    size_t n;

    if (!state->n_points) return;

    for (n = 0; n < state->n_points; n++) {
        points[n] = to_screen(state->points[n]);
    }

    switch (state->n_points) {
        case 1:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            /* complete the "line" */
            points[n++] = to_screen(state->points[0]);
            break;
        case 2:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        case 3:
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            /* close the triangle */
            points[n++] = to_screen(state->points[0]);
            break;

        default:
            break;
    }

    SDL_RenderDrawLines(renderer, points, n);
}

/*** vertmove ***/

static struct vertmove_state {
    vertex_id selected;
    vertex_id hovered;
    fpoint orig_point;
} vertmove_state;

static void vertmove_select(void)
{
    struct vertmove_state *state = &vertmove_state;
    SDL_Point tmp;
    fpoint mouse;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = from_screen(tmp);

    memset(state, 0, sizeof *state);
    state->selected = ID_NONE;
    state->hovered = canvas_find_vertex_near(mouse, TOOL_SNAP2, NULL);
}

static void vertmove_reset(void)
{
    struct vertmove_state *state = &vertmove_state;

    memset(state, 0, sizeof *state);
    state->selected = ID_NONE;
    state->hovered = ID_NONE;
}

static int vertmove_handle_event(const SDL_Event *e)
{
    struct vertmove_state *state = &vertmove_state;
    SDL_Point tmp;
    fpoint arrows, mouse;

    SDL_Keymod keymod;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = from_screen(tmp);

    keymod = SDL_GetModState();
    if ((keymod & KMOD_SHIFT) && state->selected != ID_NONE)
        mouse_rotsnap(state->orig_point, mouse, &mouse);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->selected == ID_NONE) break;
            if (e->key.keysym.sym == SDLK_ESCAPE) {
                canvas_edit_vertex(state->selected, &state->orig_point, NULL);
                state->hovered = canvas_find_vertex_near(mouse, TOOL_SNAP2, NULL);
                state->selected = ID_NONE;
                break;
            }
            if (e->key.keysym.sym == SDLK_RETURN) {
                state->hovered = canvas_find_vertex_near(mouse, TOOL_SNAP2, NULL);
                state->selected = ID_NONE;
                break;
            }
            break;
        case SDL_KEYDOWN:
            if (state->selected == ID_NONE) break;
            arrows.x = arrows.y = 0;
            if (e->key.keysym.sym == SDLK_DOWN) arrows.y ++;
            if (e->key.keysym.sym == SDLK_UP) arrows.y --;
            if (e->key.keysym.sym == SDLK_LEFT) arrows.x --;
            if (e->key.keysym.sym == SDLK_RIGHT) arrows.x ++;
            if (arrows.x == 0 && arrows.y == 0) break;
            /* FIXME arrows is in screen space! */
            canvas_edit_vertex(state->selected, NULL, &arrows);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->hovered == ID_NONE) break;
            state->selected = state->hovered;
            state->hovered = ID_NONE;
            state->orig_point = canvas_vertex(state->selected)->p;
            break;
        case SDL_MOUSEMOTION:
            if (state->selected == ID_NONE)
                state->hovered = canvas_find_vertex_near(mouse, TOOL_SNAP2, NULL);
            else
                canvas_edit_vertex(state->selected, &mouse, NULL);
            break;
        case SDL_MOUSEBUTTONUP:
            if (state->selected == ID_NONE) break;
            if (e->button.button != SDL_BUTTON_LEFT) break;
            canvas_edit_vertex(state->selected, &mouse, NULL);
            state->hovered = state->selected;
            state->selected = ID_NONE;
            break;
    }

    return 0;
}

static void vertmove_render(SDL_Renderer *renderer)
{
    struct vertmove_state *state = &vertmove_state;
    SDL_Point a, b;

    if (state->selected != ID_NONE) {
        const struct vertex *vertex = canvas_vertex(state->selected);

        a = to_screen(state->orig_point);
        b = to_screen(vertex->p);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawLine(renderer, a.x, a.y, b.x, b.y);
    }
    else if (state->hovered != ID_NONE) {
        const struct vertex *vertex = canvas_vertex(state->hovered);
        SDL_Point p = to_screen(vertex->p);

        filledCircleRGBA(renderer, p.x, p.y, TOOL_SNAP * camera_unitpx,
                         120, 120, 120, 255);
    }
}

/*** nodedel ***/

static struct nodedel_state {
    node_id over;
} nodedel_state;

static void nodedel_select(void)
{
    struct nodedel_state *state = &nodedel_state;
    SDL_Point tmp;
    fpoint mouse;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = from_screen(tmp);

    memset(state, 0, sizeof *state);
    state->over = canvas_find_node_at(mouse);
}

static void nodedel_deselect(void)
{
    struct nodedel_state *state = &nodedel_state;

    memset(state, 0, sizeof *state);
    state->over = ID_NONE;
}

static int nodedel_handle_event(const SDL_Event *e)
{
    struct nodedel_state *state = &nodedel_state;
    SDL_Point tmp;
    fpoint mouse;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = from_screen(tmp);

    switch (e->type) {
        case SDL_MOUSEMOTION:
            state->over = canvas_find_node_at(mouse);
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->over == ID_NONE) break;
            canvas_delete_node(state->over);
            state->over = ID_NONE;
            break;
    }

    return 0;
}

static void nodedel_render(SDL_Renderer *renderer)
{
    struct nodedel_state *state = &nodedel_state;
    if (state->over == ID_NONE) return;

    const struct node *node = canvas_node(state->over);

    SDL_Point points[4] = {
        to_screen(canvas_vertex(node->v[0])->p),
        to_screen(canvas_vertex(node->v[1])->p),
        to_screen(canvas_vertex(node->v[2])->p),
        to_screen(canvas_vertex(node->v[0])->p),
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLines(renderer, points, 4);
}

/* arcdraw */

static struct arcdraw_state {
    fpoint points[3];
    size_t n_points;
    int in_prompt;
} arcdraw_state;

static void arcdraw_reset(void)
{
    struct arcdraw_state *state = &arcdraw_state;

    memset(state, 0, sizeof *state);
}

static void arcdraw_sectors_ok(const char *str, void *ctx __attribute__((unused)))
{
    struct arcdraw_state *state = &arcdraw_state;
    fvector ab, ac;
    int s, n_sectors;
    float r, t, tab, tac;
    vertex_id vid[3];

    assert(state->n_points == 3);

    n_sectors = atoi(str);
    if (n_sectors <= 0) {
        arcdraw_reset();
        return;
    }

    r = lengthfv(subtractfp(state->points[1], state->points[0]));
    ab = subtractfp(state->points[1], state->points[0]);
    ac = subtractfp(state->points[2], state->points[0]);
    tab = atan2(ab.y, ab.x);
    tac = atan2(ac.y, ac.x);
    t = (tac - tab);
    if (t < 0) t += 2 * M_PI;
    t /= n_sectors;

    vid[0] = canvas_find_vertex_near(state->points[0], 0, NULL);
    if (vid[0] == ID_NONE)
        vid[0] = canvas_add_vertex(state->points[0]);
    for (s = 0; s < n_sectors; s++) {
        fpoint p1, p2;
        p1.x = state->points[0].x + r * cosf(tab + s * t);
        p1.y = state->points[0].y + r * sinf(tab + s * t);
        vid[1] = canvas_find_vertex_near(p1, 0, NULL);
        if (vid[1] == ID_NONE)
            vid[1] = canvas_add_vertex(p1);
        p2.x = state->points[0].x + r * cosf(tab + (s + 1) * t);
        p2.y = state->points[0].y + r * sinf(tab + (s + 1) * t);
        vid[2] = canvas_find_vertex_near(p2, 0, NULL);
        if (vid[2] == ID_NONE)
            vid[2] = canvas_add_vertex(p2);

        canvas_add_node(vid);
    }

    arcdraw_reset();
}

static void arcdraw_sectors_cancel(void *ctx __attribute__((unused)))
{
    arcdraw_reset();
}

static void arcdraw_start(fpoint p)
{
    struct arcdraw_state *state = &arcdraw_state;

    assert(state->n_points == 0);

    state->points[state->n_points++] = p;
}

static void arcdraw_next_point(fpoint p)
{
    struct arcdraw_state *state = &arcdraw_state;

    assert(state->n_points > 0);

    if (state->n_points >= 3) {
        state->in_prompt = 1;
        prompt("sectors: ", NULL, arcdraw_sectors_ok, arcdraw_sectors_cancel, NULL);
    }
    else {
        state->points[state->n_points++] = p;
    }
}

static void arcdraw_edit_point(const fpoint *p_abs, const SDL_Point *p_rel)
{
    struct arcdraw_state *state = &arcdraw_state;

    assert(state->n_points > 0);

    if (state->n_points > 3)
        return;

    if (p_abs) {
        state->points[state->n_points - 1] = *p_abs;
    }
    else if (p_rel) {
        SDL_Point p = to_screen(state->points[state->n_points - 1]);
        p.x += p_rel->x;
        p.y += p_rel->y;
        state->points[state->n_points - 1] = from_screen(p);
    }
}

static int arcdraw_handle_event(const SDL_Event *e)
{
    struct arcdraw_state *state = &arcdraw_state;
    SDL_Point arrows, tmp;
    fpoint mouse;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = from_screen(tmp);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->n_points == 0) break;
            if (e->key.keysym.sym == SDLK_ESCAPE)
                arcdraw_reset();
            else if (e->key.keysym.sym == SDLK_RETURN)
                arcdraw_next_point(mouse);
            break;
        case SDL_KEYDOWN:
            if (state->n_points == 0) break;
            arrows.x = arrows.y = 0;
            if (e->key.keysym.sym == SDLK_DOWN) arrows.y ++;
            else if (e->key.keysym.sym == SDLK_UP) arrows.y --;
            else if (e->key.keysym.sym == SDLK_LEFT) arrows.x --;
            else if (e->key.keysym.sym == SDLK_RIGHT) arrows.x ++;
            else break;
            arcdraw_edit_point(NULL, &arrows);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points > 0) break;
            if (state->in_prompt) break;
            canvas_find_vertex_near(mouse, TOOL_SNAP2, &mouse);
            arcdraw_start(mouse);
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0)  break;
            if (state->in_prompt) break;
            canvas_find_vertex_near(mouse, TOOL_SNAP2, &mouse);
            arcdraw_edit_point(&mouse, NULL);
            break;
        case SDL_MOUSEBUTTONUP:
            if (state->in_prompt) break;
            if (e->button.button == SDL_BUTTON_RIGHT)
                arcdraw_reset();
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points == 0) break;
            canvas_find_vertex_near(mouse, TOOL_SNAP2, &mouse);
            arcdraw_next_point(mouse);
            break;
    }

    return 0;
}

static void arcdraw_render(SDL_Renderer *renderer)
{
    struct arcdraw_state *state = &arcdraw_state;
    SDL_Point points[3] = {
        to_screen(state->points[0]),
        to_screen(state->points[1]),
        to_screen(state->points[2]),
    };
    SDL_Point ab, ac;

    if (state->n_points == 0) return;

    if (state->n_points == 2) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawLine(renderer,
                           points[0].x, points[0].y,
                           points[1].x, points[1].y);
    }
    else if (state->n_points == 3) {
        ab = psubtractp(points[1], points[0]);
        ac = psubtractp(points[2], points[0]);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderDrawLine(renderer,
                           points[0].x, points[0].y,
                           points[1].x, points[1].y);
        SDL_RenderDrawLine(renderer,
                           points[0].x, points[0].y,
                           points[2].x, points[2].y);

        arcRGBA(renderer,
                points[0].x,
                points[0].y,
                lroundf(lengthp(points[0], points[1])),
                atan2(ab.y, ab.x) * 180 / M_PI,
                atan2(ac.y, ac.x) * 180 / M_PI,
                255, 0, 0, 255);
    }
}

struct tool tools[] = {
    { &nodedraw_reset, &nodedraw_reset, &nodedraw_handle_event, &nodedraw_render, "draw nodes" },
    { &vertmove_select, &vertmove_reset, &vertmove_handle_event, &vertmove_render, "move vertices" },
    { &nodedel_select, &nodedel_deselect, &nodedel_handle_event, &nodedel_render, "delete nodes" },
    { &arcdraw_reset, &arcdraw_reset, &arcdraw_handle_event, &arcdraw_render, "draw arcs" },
};
