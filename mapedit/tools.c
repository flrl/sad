#include "mapedit/canvas.h"
#include "mapedit/tools.h"

static const int TOOL_SNAP2 = (5*5);

static struct nodedraw_state {
    SDL_Point points[3];
    unsigned n_points;
} nodedraw_state;

static void nodedraw_select(void) {
    struct nodedraw_state *state = &nodedraw_state;

    memset(state, 0, sizeof *state);
}

static void nodedraw_deselect() {
    struct nodedraw_state *state = &nodedraw_state;

    memset(state, 0, sizeof *state);
}

static int nodedraw_handle_event(const SDL_Event *e) {
    struct nodedraw_state *state = &nodedraw_state;
    SDL_Point tmp;
    int i;

    switch (e->type) {
        case SDL_KEYUP:
            if (e->key.keysym.sym == SDLK_ESCAPE)
                state->n_points = 0;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button != SDL_BUTTON_LEFT) break;
            if (state->n_points >= 3) break;
            tmp.x = e->button.x;
            tmp.y = e->button.y;
            canvas_find_vertex_near(&tmp, TOOL_SNAP2, &tmp);
            state->points[state->n_points] = tmp;
            if (state->n_points == 0) {
                /* start next point at current location */
                state->points[++ state->n_points] = tmp;
            }
            break;
        case SDL_MOUSEMOTION:
            if (state->n_points == 0 || state->n_points >= 3) break;
            tmp.x = e->motion.x;
            tmp.y = e->motion.y;
            canvas_find_vertex_near(&tmp, TOOL_SNAP2, &tmp);
            state->points[state->n_points] = tmp;
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button == SDL_BUTTON_RIGHT) {
                state->n_points = 0;
                break;
            }
            if (e->button.button != SDL_BUTTON_LEFT) break;
            tmp.x = e->button.x;
            tmp.y = e->button.y;
            canvas_find_vertex_near(&tmp, TOOL_SNAP2, &tmp);
            state->points[state->n_points] = tmp;
            state->n_points ++;
            if (state->n_points < 3) {
                /* start next point at current location */
                state->points[state->n_points] = tmp;
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

static void nodedraw_render(SDL_Renderer *renderer) {
    struct nodedraw_state *state = &nodedraw_state;

    if (!state->n_points) return;

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    switch (state->n_points) {
        case 1:
            SDL_RenderDrawLine(renderer,
                state->points[0].x, state->points[0].y,
                state->points[1].x, state->points[1].y);
            break;
        case 2:
        case 3:
            SDL_RenderDrawLines(renderer, state->points, 3);
            SDL_RenderDrawLine(renderer,
                state->points[2].x, state->points[2].y,
                state->points[0].x, state->points[0].y);
            break;

        default:
            break;
    }
}

struct tool tools[] = {
    { &nodedraw_select, &nodedraw_deselect, &nodedraw_handle_event, &nodedraw_render },
};
