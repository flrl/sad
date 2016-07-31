#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <SDL2_gfxPrimitives.h>

#include "mapedit/canvas.h"
#include "mapedit/colour.h"
#include "mapedit/geometry.h"
#include "mapedit/prompt.h"
#include "mapedit/tools.h"
#include "mapedit/view.h"

#define TOOL_SNAP (5)

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
        canvas_add_node(vid[0], vid[1], vid[2]);
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
        state->points[state->n_points - 1] = addfp(state->points[state->n_points - 1],
                                                   vector_from_screen(*p_rel));
    }
}

static void mouse_rotsnap(fpoint a, fpoint b, fpoint *out)
{
    const static fvector directions[8] = {
        {        1.0f,  0.0f       },   // east
        {   INV_SQRT2,  INV_SQRT2  },   // northeast
        {        0.0f,  1.0f       },   // north
        {  -INV_SQRT2,  INV_SQRT2  },   // northwest
        {       -1.0f,  0.0f       },   // west
        {  -INV_SQRT2, -INV_SQRT2  },   // southwest
        {        0.0f, -1.0f       },   // south
        {   INV_SQRT2, -INV_SQRT2  },   // southeast
    };
    const static size_t n_directions = 8;
    fvector vab = subtractfp(a, b); // FIXME ?
    const double vab_len = lengthfv(vab);
    size_t best_direction = 0;
    double best_result = 0.0;
    size_t i;

    assert(out != NULL);

    for (i = 0; i < n_directions; i++) {
        double quality = dotfv(directions[i], vab);
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
    int handled = 0;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = point_from_screen(tmp);

    keymod = SDL_GetModState();
    if ((keymod & KMOD_SHIFT) && state->n_points > 1)
        mouse_rotsnap(state->points[state->n_points - 2],
                      mouse, &mouse);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->n_points == 0) break;
            handled = 1;
            if (e->key.keysym.sym == SDLK_ESCAPE)
                nodedraw_reset();
            else if (e->key.keysym.sym == SDLK_RETURN)
                nodedraw_next_point(mouse);
            break;
        case SDL_KEYDOWN:
            if (state->n_points == 0) break;
            handled = 1;
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
            handled = 1;
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            nodedraw_start(mouse);
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0) break;
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            nodedraw_edit_point(&mouse, NULL);
            break;
        case SDL_MOUSEBUTTONUP:
            if (state->n_points == 0) break;
            handled = 1;
            if (e->button.button != SDL_BUTTON_LEFT) {
                nodedraw_reset();
                break;
            }
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            nodedraw_next_point(mouse);
            break;
    }

    return handled;
}

static void nodedraw_render(SDL_Renderer *renderer)
{
    struct nodedraw_state *state = &nodedraw_state;
    SDL_Point points[4];
    size_t n;

    if (!state->n_points) return;

    for (n = 0; n < state->n_points; n++) {
        points[n] = point_to_screen(state->points[n]);
    }

    switch (state->n_points) {
        case 1:
            SDL_SetRenderDrawColor(renderer, C(tool_draw0));
            /* complete the "line" */
            points[n++] = point_to_screen(state->points[0]);
            break;
        case 2:
            SDL_SetRenderDrawColor(renderer, C(tool_draw0));
            break;
        case 3:
            SDL_SetRenderDrawColor(renderer, C(tool_draw1));
            /* close the triangle */
            points[n++] = point_to_screen(state->points[0]);
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
    mouse = point_from_screen(tmp);

    memset(state, 0, sizeof *state);
    state->selected = ID_NONE;
    state->hovered = canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), NULL);
}

static void vertmove_reset(void)
{
    struct vertmove_state *state = &vertmove_state;

    memset(state, 0, sizeof *state);
    state->selected = ID_NONE;
    state->hovered = ID_NONE;
}

static void vertmove_edit_vertex(const fpoint *p_abs, const SDL_Point *p_rel)
{
    struct vertmove_state *state = &vertmove_state;

    assert(state->selected != ID_NONE);

    if (p_abs) {
        canvas_edit_vertex(state->selected, p_abs, NULL);
    }
    else if (p_rel) {
        fvector offset = vector_from_screen(*p_rel);
        canvas_edit_vertex(state->selected, NULL, &offset);
    }
}

static void vertmove_abort(fpoint p)
{
    struct vertmove_state *state = &vertmove_state;

    if (state->selected == ID_NONE) return;

    vertmove_edit_vertex(&state->orig_point, NULL);
    vertmove_reset();

    state->hovered = canvas_find_vertex_near(p, scalar_from_screen(TOOL_SNAP), NULL);
}

static int vertmove_handle_event(const SDL_Event *e)
{
    struct vertmove_state *state = &vertmove_state;
    SDL_Point arrows, tmp;
    fpoint mouse;
    int handled = 0;

    SDL_Keymod keymod;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = point_from_screen(tmp);

    keymod = SDL_GetModState();
    if ((keymod & KMOD_SHIFT) && state->selected != ID_NONE)
        mouse_rotsnap(state->orig_point, mouse, &mouse);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->selected == ID_NONE) break;
            handled = 1;
            if (e->key.keysym.sym == SDLK_ESCAPE) {
                vertmove_abort(mouse);
            }
            else if (e->key.keysym.sym == SDLK_RETURN) {
                state->hovered = canvas_find_vertex_near(mouse,
                                                         scalar_from_screen(TOOL_SNAP),
                                                         NULL);
                state->selected = ID_NONE;
            }
            break;
        case SDL_KEYDOWN:
            if (state->selected == ID_NONE) break;
            handled = 1;
            arrows.x = arrows.y = 0;
            if (e->key.keysym.sym == SDLK_DOWN) arrows.y ++;
            if (e->key.keysym.sym == SDLK_UP) arrows.y --;
            if (e->key.keysym.sym == SDLK_LEFT) arrows.x --;
            if (e->key.keysym.sym == SDLK_RIGHT) arrows.x ++;
            if (arrows.x == 0 && arrows.y == 0) break;
            vertmove_edit_vertex(NULL, &arrows);
            break;
        case SDL_MOUSEMOTION:
            if (state->selected == ID_NONE) {
                state->hovered = canvas_find_vertex_near(mouse,
                                                         scalar_from_screen(TOOL_SNAP),
                                                         NULL);
            }
            else {
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
                vertmove_edit_vertex(&mouse, NULL);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            handled = 1;
            if (e->button.button != SDL_BUTTON_LEFT) {
                vertmove_abort(mouse);
                break;
            }
            if (state->selected == ID_NONE) {
                if (state->hovered == ID_NONE) break;

                state->selected = state->hovered;
                state->hovered = ID_NONE;
                state->orig_point = canvas_vertex(state->selected)->p;
            }
            else {
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
                vertmove_edit_vertex(&mouse, NULL);
                state->hovered = state->selected;
                state->selected = ID_NONE;
            }
            break;
    }

    return handled;
}

static void vertmove_render(SDL_Renderer *renderer)
{
    struct vertmove_state *state = &vertmove_state;
    SDL_Point a, b;

    if (state->selected != ID_NONE) {
        const struct vertex *vertex = canvas_vertex(state->selected);

        a = point_to_screen(state->orig_point);
        b = point_to_screen(vertex->p);

        SDL_SetRenderDrawColor(renderer, C(tool_draw0));
        SDL_RenderDrawLine(renderer, a.x, a.y, b.x, b.y);
    }
    else if (state->hovered != ID_NONE) {
        const struct vertex *vertex = canvas_vertex(state->hovered);
        SDL_Point p = point_to_screen(vertex->p);

        filledCircleRGBA(renderer, p.x, p.y, TOOL_SNAP - 1,
                         C(tool_move_high));
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
    mouse = point_from_screen(tmp);

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
    int handled = 0;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = point_from_screen(tmp);

    switch (e->type) {
        case SDL_MOUSEMOTION:
            state->over = canvas_find_node_at(mouse);
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->over == ID_NONE) break;
            handled = 1;
            canvas_delete_node(state->over);
            state->over = ID_NONE;
            break;
    }

    return handled;
}

static void nodedel_render(SDL_Renderer *renderer)
{
    struct nodedel_state *state = &nodedel_state;
    if (state->over == ID_NONE) return;

    const struct node *node = canvas_node(state->over);

    SDL_Point points[3] = {
        point_to_screen(canvas_vertex(node->v[0])->p),
        point_to_screen(canvas_vertex(node->v[1])->p),
        point_to_screen(canvas_vertex(node->v[2])->p),
    };

    filledTrigonRGBA(renderer,
                     points[0].x, points[0].y,
                     points[1].x, points[1].y,
                     points[2].x, points[2].y,
                     C(tool_del_high));
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
    double r, t, tab, tac;
    vertex_id a, b, c;

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

    a = canvas_find_vertex_near(state->points[0], 0, NULL);
    if (a == ID_NONE)
        a = canvas_add_vertex(state->points[0]);
    for (s = 0; s < n_sectors; s++) {
        fpoint p1, p2;
        p1.x = state->points[0].x + r * cos(tab + s * t);
        p1.y = state->points[0].y + r * sin(tab + s * t);
        b = canvas_find_vertex_near(p1, 0, NULL);
        if (b == ID_NONE)
            b = canvas_add_vertex(p1);
        p2.x = state->points[0].x + r * cos(tab + (s + 1) * t);
        p2.y = state->points[0].y + r * sin(tab + (s + 1) * t);
        c = canvas_find_vertex_near(p2, 0, NULL);
        if (c == ID_NONE)
            c = canvas_add_vertex(p2);

        canvas_add_node(a, b, c);
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
        state->points[state->n_points - 1] = addfp(state->points[state->n_points - 1],
                                                   vector_from_screen(*p_rel));
    }
}

static int arcdraw_handle_event(const SDL_Event *e)
{
    struct arcdraw_state *state = &arcdraw_state;
    SDL_Point arrows, tmp;
    fpoint mouse;
    int handled = 0;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = point_from_screen(tmp);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->n_points == 0) break;
            if (state->in_prompt) break;
            handled = 1;
            if (e->key.keysym.sym == SDLK_ESCAPE)
                arcdraw_reset();
            else if (e->key.keysym.sym == SDLK_RETURN)
                arcdraw_next_point(mouse);
            break;
        case SDL_KEYDOWN:
            if (state->n_points == 0) break;
            if (state->in_prompt) break;
            handled = 1;
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
            handled = 1;
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            arcdraw_start(mouse);
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0)  break;
            if (state->in_prompt) break;
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            arcdraw_edit_point(&mouse, NULL);
            break;
        case SDL_MOUSEBUTTONUP:
            if (state->in_prompt) break;
            if (state->n_points == 0) break;
            handled = 1;
            if (e->button.button != SDL_BUTTON_LEFT) {
                arcdraw_reset();
                break;
            }
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            arcdraw_next_point(mouse);
            break;
    }

    return handled;
}

static void arcdraw_render(SDL_Renderer *renderer)
{
    struct arcdraw_state *state = &arcdraw_state;
    SDL_Point points[3] = {
        point_to_screen(state->points[0]),
        point_to_screen(state->points[1]),
        point_to_screen(state->points[2]),
    };
    SDL_Point ab, ac;

    if (state->n_points == 0) return;

    if (state->n_points == 2) {
        SDL_SetRenderDrawColor(renderer, C(tool_draw0));
        SDL_RenderDrawLine(renderer,
                           points[0].x, points[0].y,
                           points[1].x, points[1].y);
    }
    else if (state->n_points == 3) {
        ab = psubtractp(points[1], points[0]);
        ac = psubtractp(points[2], points[0]);

        SDL_SetRenderDrawColor(renderer, C(tool_draw1));
        SDL_RenderDrawLine(renderer,
                           points[0].x, points[0].y,
                           points[1].x, points[1].y);
        SDL_RenderDrawLine(renderer,
                           points[0].x, points[0].y,
                           points[2].x, points[2].y);

        arcRGBA(renderer,
                points[0].x,
                points[0].y,
                lround(lengthp(points[0], points[1])),
                lround(atan2(ab.y, ab.x) * 180 / M_PI),
                lround(atan2(ac.y, ac.x) * 180 / M_PI),
                C(tool_draw0));
    }
}

/*** rectdraw ***/
static struct rectdraw_state {
    fpoint points[3];
    size_t n_points;
} rectdraw_state;

static void rectdraw_reset(void)
{
    struct rectdraw_state *state = &rectdraw_state;

    memset(state, 0, sizeof *state);
}

static void rectdraw_start(fpoint p)
{
    struct rectdraw_state *state = &rectdraw_state;

    assert(state->n_points == 0);

    state->points[state->n_points++] = p;
}

static void rectdraw_next_point(fpoint p)
{
    struct rectdraw_state *state = &rectdraw_state;

    assert(state->n_points > 0);

    if (state->n_points >= 3) {
        const fpoint a = state->points[0], b = state->points[1], p = state->points[2];
        fpoint c, d, e, r;
        fvector rp;
        vertex_id vert_a, vert_b, vert_c, vert_d, vert_e;

        r = addfp(a, projectfv(subtractfp(p, a), subtractfp(b, a)));
        rp = subtractfp(p, r);
        c = addfp(b, rp);
        d = addfp(a, rp);
        e = addfp(a, scalefv(subtractfp(c, a), 0.5));

        vert_a = canvas_find_vertex_near(a, 0, NULL);
        if (vert_a == ID_NONE) vert_a = canvas_add_vertex(a);

        vert_b = canvas_find_vertex_near(b, 0, NULL);
        if (vert_b == ID_NONE) vert_b = canvas_add_vertex(b);

        vert_c = canvas_find_vertex_near(c, 0, NULL);
        if (vert_c == ID_NONE) vert_c = canvas_add_vertex(c);

        vert_d = canvas_find_vertex_near(d, 0, NULL);
        if (vert_d == ID_NONE) vert_d = canvas_add_vertex(d);

        vert_e = canvas_find_vertex_near(e, 0, NULL);
        if (vert_e == ID_NONE) vert_e = canvas_add_vertex(e);

        canvas_add_node(vert_a, vert_b, vert_e);
        canvas_add_node(vert_b, vert_c, vert_e);
        canvas_add_node(vert_c, vert_d, vert_e);
        canvas_add_node(vert_d, vert_a, vert_e);

        rectdraw_reset();
    }
    else {
        state->points[state->n_points++] = p;
    }
}

static void rectdraw_edit_point(const fpoint *p_abs, const SDL_Point *p_rel)
{
    struct rectdraw_state *state = &rectdraw_state;

    assert(state->n_points > 0);

    if (state->n_points > 3)
        return;

    if (p_abs) {
        state->points[state->n_points - 1] = *p_abs;
    }
    else if (p_rel) {
        state->points[state->n_points - 1] = addfp(state->points[state->n_points - 1],
                                                   vector_from_screen(*p_rel));
    }
}

static int rectdraw_handle_event(const SDL_Event *e)
{
    struct rectdraw_state *state = &rectdraw_state;
    SDL_Point arrows, tmp;
    fpoint mouse;
    int handled = 0;

    SDL_GetMouseState(&tmp.x, &tmp.y);
    mouse = point_from_screen(tmp);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->n_points == 0) break;
            handled = 1;
            if (e->key.keysym.sym == SDLK_ESCAPE)
                rectdraw_reset();
            else if (e->key.keysym.sym == SDLK_RETURN)
                rectdraw_next_point(mouse);
            break;
        case SDL_KEYDOWN:
            if (state->n_points == 0) break;
            handled = 1;
            arrows.x = arrows.y = 0;
            if (e->key.keysym.sym == SDLK_DOWN) arrows.y ++;
            else if (e->key.keysym.sym == SDLK_UP) arrows.y --;
            else if (e->key.keysym.sym == SDLK_LEFT) arrows.x --;
            else if (e->key.keysym.sym == SDLK_RIGHT) arrows.x ++;
            else break;
            rectdraw_edit_point(NULL, &arrows);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points > 0) break;
            handled = 1;
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            rectdraw_start(mouse);
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0) break;
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            rectdraw_edit_point(&mouse, NULL);
            break;
        case SDL_MOUSEBUTTONUP:
            if (state->n_points == 0) break;
            handled = 1;
            if (e->button.button != SDL_BUTTON_LEFT) {
                rectdraw_reset();
                break;
            }
            if (ID_NONE == canvas_find_vertex_near(mouse, scalar_from_screen(TOOL_SNAP), &mouse))
                mouse = view_find_gridpoint_near(mouse, scalar_from_screen(TOOL_SNAP));
            rectdraw_next_point(mouse);
            break;
    }

    return handled;
}

static void rectdraw_render(SDL_Renderer *renderer)
{
    struct rectdraw_state *state = &rectdraw_state;
    SDL_Point points[5] = {
        point_to_screen(state->points[0]),
        point_to_screen(state->points[1]),
        { 0, 0 },
        { 0, 0 },
        point_to_screen(state->points[0]),
    };

    if (state->n_points == 0) return;

    if (state->n_points == 2) {
        SDL_SetRenderDrawColor(renderer, C(tool_draw0));
        SDL_RenderDrawLine(renderer,
                           points[0].x, points[0].y,
                           points[1].x, points[1].y);
    }
    else if (state->n_points == 3) {
        const fpoint a = state->points[0], b = state->points[1], p = state->points[2];
        fpoint c, d, r;
        fvector rp;
        SDL_Point screen_p;

        r = addfp(a, projectfv(subtractfp(p, a), subtractfp(b, a)));
        rp = subtractfp(p, r);
        c = addfp(b, rp);
        d = addfp(a, rp);

        points[2] = point_to_screen(c);
        points[3] = point_to_screen(d);

        SDL_SetRenderDrawColor(renderer, C(tool_draw1));
        SDL_RenderDrawLines(renderer, points, 5);

        SDL_SetRenderDrawColor(renderer, C(tool_draw0));
        screen_p = point_to_screen(p);
        SDL_RenderDrawPoint(renderer, screen_p.x, screen_p.y);
    }
}

struct tool tools[] = {
    { &nodedraw_reset, &nodedraw_reset, &nodedraw_handle_event, &nodedraw_render, "draw nodes" },
    { &vertmove_select, &vertmove_reset, &vertmove_handle_event, &vertmove_render, "move vertices" },
    { &nodedel_select, &nodedel_deselect, &nodedel_handle_event, &nodedel_render, "delete nodes" },
    { &arcdraw_reset, &arcdraw_reset, &arcdraw_handle_event, &arcdraw_render, "draw arcs" },
    { &rectdraw_reset, &rectdraw_reset, &rectdraw_handle_event, &rectdraw_render, "draw rectangles" },
};
