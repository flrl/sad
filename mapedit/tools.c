#include <assert.h>
#include <math.h>

#include "mapedit/camera.h"
#include "mapedit/canvas.h"
#include "mapedit/geometry.h"
#include "mapedit/tools.h"

static const int TOOL_SNAP2 = (5*5);

/*** nodedraw ***/
static struct nodedraw_state {
    SDL_Point points[3];
    unsigned n_points;
} nodedraw_state;

static void nodedraw_reset(void)
{
    struct nodedraw_state *state = &nodedraw_state;

    memset(state, 0, sizeof *state);
}

static void nodedraw_next_point(SDL_Point p)
{
    struct nodedraw_state *state = &nodedraw_state;
    vertex_id vid[3];
    size_t i;

    switch (state->n_points) {
        case 0:
            state->points[state->n_points++] = p;
        case 1:
        case 2:
            state->points[state->n_points++] = p;
            return;
        case 3:
        default:
            for (i = 0; i < 3; i++) {
                vid[i] = canvas_find_vertex_near(&state->points[i], 0, NULL);
                if (vid[i] == ID_NONE)
                    vid[i] = canvas_add_vertex(&state->points[i]);
            }
            canvas_add_node(vid);
            nodedraw_reset();
    }
}

static void nodedraw_edit_point(const SDL_Point *p_abs, const SDL_Point *p_rel)
{
    struct nodedraw_state *state = &nodedraw_state;

    if (state->n_points == 0 || state->n_points > 3)
        return;

    if (p_abs) {
        state->points[state->n_points - 1] = *p_abs;
    }
    else if (p_rel) {
        state->points[state->n_points - 1].x += p_rel->x;
        state->points[state->n_points - 1].y += p_rel->y;
    }
}

static void mouse_rotsnap(SDL_Point a, SDL_Point b, SDL_Point *out)
{
    const static float directions[8][2] = {
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
    const float vab[2] = { a.x - b.x, a.y - b.y };
    const float vab_len = sqrtf(vab[0] * vab[0] + vab[1] * vab[1]);
    size_t best_direction = 0;
    float best_result = 0.0f;
    size_t i;

    assert(out != NULL);

    for (i = 0; i < n_directions; i++) {
        float quality = dotf(directions[i], vab);
        if (quality < best_result) {
            best_result = quality;
            best_direction = i;
        }
    }

    out->x = a.x + lroundf(vab_len * directions[best_direction][0]);
    out->y = a.y + lround(vab_len * directions[best_direction][1]);
}

static int nodedraw_handle_event(const SDL_Event *e)
{
    struct nodedraw_state *state = &nodedraw_state;
    SDL_Point mouse;
    SDL_Point arrows;
    SDL_Keymod keymod;

    SDL_GetMouseState(&mouse.x, &mouse.y);
    mouse = addp(mouse, camera_offset);

    keymod = SDL_GetModState();
    if ((keymod & KMOD_SHIFT) && state->n_points > 1)
        mouse_rotsnap(state->points[state->n_points - 2],
                      mouse, &mouse);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->n_points == 0 || state->n_points > 3) break;
            if (e->key.keysym.sym == SDLK_ESCAPE)
                nodedraw_reset();
            else if (e->key.keysym.sym == SDLK_RETURN)
                nodedraw_next_point(mouse);
            break;
        case SDL_KEYDOWN:
            if (state->n_points == 0 || state->n_points > 3) break;
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
            if (state->n_points >= 3) break;
            canvas_find_vertex_near(&mouse, TOOL_SNAP2, &mouse);
            nodedraw_edit_point(&mouse, NULL);
            nodedraw_next_point(mouse);
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0 || state->n_points > 3) break;
            canvas_find_vertex_near(&mouse, TOOL_SNAP2, &mouse);
            nodedraw_edit_point(&mouse, NULL);
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button == SDL_BUTTON_RIGHT)
                nodedraw_reset();
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points == 0 || state->n_points > 3) break;
            canvas_find_vertex_near(&mouse, TOOL_SNAP2, &mouse);
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
        points[n] = subtractp(state->points[n], camera_offset);
    }

    switch (state->n_points) {
        case 1:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            /* complete the "line" */
            points[n++] = subtractp(state->points[0], camera_offset);
            break;
        case 2:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        case 3:
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            /* close the triangle */
            points[n++] = subtractp(state->points[0], camera_offset);
            break;

        default:
            break;
    }

    SDL_RenderDrawLines(renderer, points, n);
}

/*** vertmove ***/

static struct vertmove_state {
    vertex_id selected;
    SDL_Point orig_point;
} vertmove_state;

static void vertmove_reset(void)
{
    struct vertmove_state *state = &vertmove_state;

    memset(state, 0, sizeof *state);
    state->selected = ID_NONE;
}

static int vertmove_handle_event(const SDL_Event *e)
{
    struct vertmove_state *state = &vertmove_state;
    SDL_Point mouse;
    SDL_Point arrows;
    vertex_id vid;

    SDL_Keymod keymod;

    SDL_GetMouseState(&mouse.x, &mouse.y);
    mouse = addp(mouse, camera_offset);

    keymod = SDL_GetModState();
    if ((keymod & KMOD_SHIFT) && state->selected != ID_NONE)
        mouse_rotsnap(state->orig_point, mouse, &mouse);

    switch (e->type) {
        case SDL_KEYUP:
            if (state->selected == ID_NONE) break;
            if (e->key.keysym.sym == SDLK_ESCAPE) {
                canvas_set_vertex(state->selected, &state->orig_point);
                state->selected = ID_NONE;
                break;
            }
            if (e->key.keysym.sym == SDLK_RETURN) {
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
            canvas_offset_vertex(state->selected, &arrows);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            vid = canvas_find_vertex_near(&mouse, TOOL_SNAP2, &mouse);
            if (vid == ID_NONE) break;
            state->selected = vid;
            state->orig_point = canvas_vertex(vid)->p;
            break;
        case SDL_MOUSEMOTION:
            if (state->selected == ID_NONE) break;
            canvas_set_vertex(state->selected, &mouse);
            break;
        case SDL_MOUSEBUTTONUP:
            if (state->selected == ID_NONE) break;
            if (e->button.button != SDL_BUTTON_LEFT) break;
            canvas_set_vertex(state->selected, &mouse);
            state->selected = ID_NONE;
            break;
    }

    return 0;
}

static void vertmove_render(SDL_Renderer *renderer)
{
    struct vertmove_state *state = &vertmove_state;
    SDL_Point a, b;

    if (state->selected == ID_NONE) return;

    const struct vertex *vertex = canvas_vertex(state->selected);

    a = subtractp(state->orig_point, camera_offset);
    b = subtractp(vertex->p, camera_offset);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, a.x, a.y, b.x, b.y);
}

/*** nodedel ***/

static struct nodedel_state {
    node_id over;
} nodedel_state;

static void nodedel_select(void)
{
    struct nodedel_state *state = &nodedel_state;
    SDL_Point mouse;

    SDL_GetMouseState(&mouse.x, &mouse.y);
    mouse = addp(mouse, camera_offset);

    memset(state, 0, sizeof *state);
    state->over = canvas_find_node_at(&mouse);
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
    SDL_Point mouse;

    SDL_GetMouseState(&mouse.x, &mouse.y);
    mouse = addp(mouse, camera_offset);

    switch (e->type) {
        case SDL_MOUSEMOTION:
            state->over = canvas_find_node_at(&mouse);
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
        subtractp(canvas_vertex(node->v[0])->p, camera_offset),
        subtractp(canvas_vertex(node->v[1])->p, camera_offset),
        subtractp(canvas_vertex(node->v[2])->p, camera_offset),
        subtractp(canvas_vertex(node->v[0])->p, camera_offset),
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLines(renderer, points, 4);
}

struct tool tools[] = {
    { &nodedraw_reset, &nodedraw_reset, &nodedraw_handle_event, &nodedraw_render, "draw nodes" },
    { &vertmove_reset, &vertmove_reset, &vertmove_handle_event, &vertmove_render, "move vertices" },
    { &nodedel_select, &nodedel_deselect, &nodedel_handle_event, &nodedel_render, "delete nodes" },
};
