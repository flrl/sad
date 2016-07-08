#include <config.h>

#include <stdio.h>

#include <SDL.h>

#include "mapedit/canvas.h"

int main(int argc __attribute__((unused)),
         char **argv __attribute__((unused)))
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    int shutdown = 0;
    struct node new_node = {0};
    unsigned new_node_points = 0;

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
                    new_node.p[new_node_points].x = e.button.x;
                    new_node.p[new_node_points].y = e.button.y;
                    if (new_node_points == 0) {
                        new_node_points ++;
                        new_node.p[new_node_points].x = e.button.x;
                        new_node.p[new_node_points].y = e.button.y;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (new_node_points == 0) break;
                    if (new_node_points >= 3) break;
                    new_node.p[new_node_points].x = e.button.x;
                    new_node.p[new_node_points].y = e.button.y;
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (new_node_points >= 3) break;
                    new_node_points ++;
                    new_node.p[new_node_points].x = e.button.x;
                    new_node.p[new_node_points].y = e.button.y;
                    if (new_node_points == 3) {
                        canvas_add_node(&new_node);
                        memset(&new_node, 0, sizeof new_node);
                        new_node_points = 0;
                    }
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        if (shutdown) break;

        if (new_node_points) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

            switch (new_node_points) {
                case 1:
                    SDL_RenderDrawLine(renderer,
                        new_node.p[0].x, new_node.p[0].y,
                        new_node.p[1].x, new_node.p[1].y);
                    break;
                case 2:
                case 3:
                    SDL_RenderDrawLines(renderer, new_node.p, 3);
                    SDL_RenderDrawLine(renderer,
                        new_node.p[2].x, new_node.p[2].y,
                        new_node.p[0].x, new_node.p[0].y);
                    break;

                default:
                    break;
            }
        }

        canvas_render(renderer);

        SDL_RenderPresent(renderer);
    }

    canvas_reset();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
