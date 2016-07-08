#include "mapedit/canvas.h"
#include "mapedit/tools.h"

static const int TOOL_SNAP2 = (5*5);

static struct nodedraw_state {
    SDL_Point new_node[3];
    unsigned new_node_points;
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
        case SDL_MOUSEBUTTONDOWN:
            if (state->new_node_points >= 3) break;
            tmp.x = e->button.x;
            tmp.y = e->button.y;
            canvas_find_vertex_near(&tmp, TOOL_SNAP2, &tmp);
            state->new_node[state->new_node_points] = tmp;
            if (state->new_node_points == 0) {
                state->new_node_points ++;
                state->new_node[state->new_node_points] = tmp;
            }
            break;
        case SDL_MOUSEMOTION:
            if (state->new_node_points == 0) break;
            if (state->new_node_points >= 3) break;
            tmp.x = e->button.x;
            tmp.y = e->button.y;
            canvas_find_vertex_near(&tmp, TOOL_SNAP2, &tmp);
            state->new_node[state->new_node_points] = tmp;
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button == SDL_BUTTON_RIGHT) {
                state->new_node_points = 0;
                break;
            }
            if (state->new_node_points >= 3) break;
            tmp.x = e->button.x;
            tmp.y = e->button.y;
            canvas_find_vertex_near(&tmp, TOOL_SNAP2, &tmp);
            state->new_node[state->new_node_points] = tmp;
            state->new_node_points ++;
            if (state->new_node_points == 3) {
                vertex_id v[3];
                for (i = 0; i < 3; i++) {
                    v[i] = canvas_find_vertex_near(&state->new_node[i], 0, NULL);
                    if (v[i] == ID_NONE)
                        v[i] = canvas_add_vertex(&state->new_node[i]);
                }
                canvas_add_node(v);
                state->new_node_points = 0;
            }
            else {
                state->new_node[state->new_node_points] = tmp;
            }
            break;
    }

    return 0;
}

static void nodedraw_render(SDL_Renderer *renderer) {
    struct nodedraw_state *state = &nodedraw_state;

    if (!state->new_node_points) return;

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    switch (state->new_node_points) {
        case 1:
            SDL_RenderDrawLine(renderer,
                state->new_node[0].x, state->new_node[0].y,
                state->new_node[1].x, state->new_node[1].y);
            break;
        case 2:
        case 3:
            SDL_RenderDrawLines(renderer, state->new_node, 3);
            SDL_RenderDrawLine(renderer,
                state->new_node[2].x, state->new_node[2].y,
                state->new_node[0].x, state->new_node[0].y);
            break;

        default:
            break;
    }
}

struct tool tools[] = {
    { &nodedraw_select, &nodedraw_deselect, &nodedraw_handle_event, &nodedraw_render },
};
