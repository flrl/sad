#include "mapedit/canvas.h"
#include "mapedit/tools.h"

static const int TOOL_SNAP2 = (5*5);

/*** nodedraw ***/
static struct nodedraw_state {
    SDL_Point points[3];
    unsigned n_points;
} nodedraw_state;

static void nodedraw_select(void)
{
    struct nodedraw_state *state = &nodedraw_state;

    memset(state, 0, sizeof *state);
}

static void nodedraw_deselect()
{
    struct nodedraw_state *state = &nodedraw_state;

    memset(state, 0, sizeof *state);
}

static int nodedraw_handle_event(const SDL_Event *e)
{
    struct nodedraw_state *state = &nodedraw_state;
    SDL_Point mouse;
    SDL_Point arrows;
    int i;

    SDL_GetMouseState(&mouse.x, &mouse.y);

    switch (e->type) {
        case SDL_KEYUP:
            if (e->key.keysym.sym == SDLK_ESCAPE) {
                state->n_points = 0;
                break;
            }
            if (state->n_points == 0) break;
            if (state->n_points >= 3) break;
            if (e->key.keysym.sym == SDLK_RETURN) {
                /* nasty c&p from mouseup */
                state->n_points ++;
                if (state->n_points < 3) {
                    /* start next point at current location */
                    state->points[state->n_points] = mouse;
                    break;
                }
                else {
                    vertex_id v[3];
                    for (i = 0; i < 3; i++) {
                        v[i] = canvas_find_vertex_near(&state->points[i], 0, NULL);
                        if (v[i] == ID_NONE)
                            v[i] = canvas_add_vertex(&state->points[i]);
                    }
                    canvas_add_node(v);
                    state->n_points = 0;
                }
                break;
            }
            break;
        case SDL_KEYDOWN:
            if (state->n_points == 0) break;
            if (state->n_points >= 3) break;
            arrows.x = arrows.y = 0;
            if (e->key.keysym.sym == SDLK_DOWN) arrows.y ++;
            if (e->key.keysym.sym == SDLK_UP) arrows.y --;
            if (e->key.keysym.sym == SDLK_LEFT) arrows.x --;
            if (e->key.keysym.sym == SDLK_RIGHT) arrows.x ++;
            if (arrows.x == 0 && arrows.y == 0) break;
            state->points[state->n_points].x += arrows.x;
            state->points[state->n_points].y += arrows.y;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points >= 3) break;
            canvas_find_vertex_near(&mouse, TOOL_SNAP2, &mouse);
            state->points[state->n_points] = mouse;
            if (state->n_points == 0) {
                /* start next point at current location */
                state->points[++ state->n_points] = mouse;
            }
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0 || state->n_points >= 3) break;
            canvas_find_vertex_near(&mouse, TOOL_SNAP2, &mouse);
            state->points[state->n_points] = mouse;
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button == SDL_BUTTON_RIGHT) {
                state->n_points = 0;
                break;
            }
            if (e->button.button != SDL_BUTTON_LEFT) break;
            canvas_find_vertex_near(&mouse, TOOL_SNAP2, &mouse);
            state->points[state->n_points] = mouse;
            state->n_points ++;
            if (state->n_points < 3) {
                /* start next point at current location */
                state->points[state->n_points] = mouse;
                break;
            }
            else {
                vertex_id v[3];
                for (i = 0; i < 3; i++) {
                    v[i] = canvas_find_vertex_near(&state->points[i], 0, NULL);
                    if (v[i] == ID_NONE)
                        v[i] = canvas_add_vertex(&state->points[i]);
                }
                canvas_add_node(v);
                state->n_points = 0;
            }
            break;
    }

    return 0;
}

static void nodedraw_render(SDL_Renderer *renderer)
{
    struct nodedraw_state *state = &nodedraw_state;

    if (!state->n_points) return;


    switch (state->n_points) {
        case 1:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawLine(renderer,
                state->points[0].x, state->points[0].y,
                state->points[1].x, state->points[1].y);
            break;
        case 2:
        case 3:
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawLines(renderer, state->points, 3);
            SDL_RenderDrawLine(renderer,
                state->points[2].x, state->points[2].y,
                state->points[0].x, state->points[0].y);
            break;

        default:
            break;
    }
}

/*** vertmove ***/

static struct vertmove_state {
    vertex_id selected;
    SDL_Point orig_point;
} vertmove_state;

static void vertmove_select(void)
{
    struct vertmove_state *state = &vertmove_state;

    memset(state, 0, sizeof *state);
    state->selected = ID_NONE;
}

static void vertmove_deselect(void)
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

    SDL_GetMouseState(&mouse.x, &mouse.y);

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
            state->orig_point = canvas_get_vertex(vid, NULL, NULL);
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

    if (state->selected == ID_NONE) return;

    const struct vertex *vertex = canvas_vertex(state->selected);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, state->orig_point.x, state->orig_point.y,
                                 vertex->p.x, vertex->p.y);
}

/*** nodedel ***/

static struct nodedel_state {
    node_id over;
} nodedel_state;

static void nodedel_select(void)
{
    struct nodedel_state *state = &nodedel_state;
    SDL_Point tmp;

    SDL_GetMouseState(&tmp.x, &tmp.y);

    memset(state, 0, sizeof *state);
    state->over = canvas_find_node_at(&tmp);
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
        canvas_vertex(node->v[0])->p,
        canvas_vertex(node->v[1])->p,
        canvas_vertex(node->v[2])->p,
        canvas_vertex(node->v[0])->p,
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLines(renderer, points, 4);
}

struct tool tools[] = {
    { &nodedraw_select, &nodedraw_deselect, &nodedraw_handle_event, &nodedraw_render },
    { &vertmove_select, &vertmove_deselect, &vertmove_handle_event, &vertmove_render },
    { &nodedel_select, &nodedel_deselect, &nodedel_handle_event, &nodedel_render },
};
