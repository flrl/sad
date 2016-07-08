#include <config.h>

#include <stdio.h>

#include <SDL.h>

#include "mapedit/canvas.h"

static const int SNAP2 = (5*5);

int main(int argc __attribute__((unused)),
         char **argv __attribute__((unused)))
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    int shutdown = 0;
    SDL_Point new_node[3] = {0};
    unsigned new_node_points = 0;
    SDL_Point tmp = {0};
    int i;

    const uint32_t window_flags = 0;
    window = SDL_CreateWindow("hello world",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              400, 300,
                              window_flags);

    const uint32_t renderer_flags = SDL_RENDERER_ACCELERATED
                                  | SDL_RENDERER_PRESENTVSYNC;
    renderer = SDL_CreateRenderer(window, -1, renderer_flags);

    while (!shutdown) {
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    shutdown = 1;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (new_node_points >= 3) break;
                    tmp.x = e.button.x;
                    tmp.y = e.button.y;
                    canvas_find_vertex_near(&tmp, SNAP2, &tmp);
                    new_node[new_node_points] = tmp;
                    if (new_node_points == 0) {
                        new_node_points ++;
                        new_node[new_node_points] = tmp;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (new_node_points == 0) break;
                    if (new_node_points >= 3) break;
                    tmp.x = e.button.x;
                    tmp.y = e.button.y;
                    canvas_find_vertex_near(&tmp, SNAP2, &tmp);
                    new_node[new_node_points] = tmp;
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (new_node_points >= 3) break;
                    new_node_points ++;
                    tmp.x = e.button.x;
                    tmp.y = e.button.y;
                    canvas_find_vertex_near(&tmp, SNAP2, &tmp);
                    new_node[new_node_points] = tmp;
                    if (new_node_points == 3) {
                        vertex_id v[3];
                        for (i = 0; i < 3; i++) {
                            v[i] = canvas_find_vertex_near(&new_node[i], 0, NULL);
                            if (v[i] == ID_NONE)
                                v[i] = canvas_add_vertex(&new_node[i]);
                        }
                        canvas_add_node(v);
                        new_node_points = 0;
                    }
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        if (shutdown) break;

        canvas_render(renderer);

        if (new_node_points) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

            switch (new_node_points) {
                case 1:
                    SDL_RenderDrawLine(renderer,
                        new_node[0].x, new_node[0].y,
                        new_node[1].x, new_node[1].y);
                    break;
                case 2:
                case 3:
                    SDL_RenderDrawLines(renderer, new_node, 3);
                    SDL_RenderDrawLine(renderer,
                        new_node[2].x, new_node[2].y,
                        new_node[0].x, new_node[0].y);
                    break;

                default:
                    break;
            }
        }

        SDL_RenderPresent(renderer);
    }

    canvas_reset();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
