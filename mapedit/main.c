#include <config.h>

#include <stdio.h>

#include <SDL.h>

struct triangle {
    SDL_Point p[3];
};

int main(int argc __attribute__((unused)),
         char **argv __attribute__((unused)))
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    int shutdown = 0;
    struct triangle triangle = {0};
    unsigned triangle_points = 0;

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
                    if (triangle_points >= 3) break;
                    triangle.p[triangle_points].x = e.button.x;
                    triangle.p[triangle_points].y = e.button.y;
                    if (triangle_points == 0) {
                        triangle_points ++;
                        triangle.p[triangle_points].x = e.button.x;
                        triangle.p[triangle_points].y = e.button.y;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (triangle_points == 0) break;
                    if (triangle_points >= 3) break;
                    triangle.p[triangle_points].x = e.button.x;
                    triangle.p[triangle_points].y = e.button.y;
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (triangle_points >= 3) break;
                    triangle_points ++;
                    triangle.p[triangle_points].x = e.button.x;
                    triangle.p[triangle_points].y = e.button.y;
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        if (shutdown) break;

        if (triangle_points) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

            switch (triangle_points) {
                case 1:
                    SDL_RenderDrawLine(renderer,
                        triangle.p[0].x, triangle.p[0].y,
                        triangle.p[1].x, triangle.p[1].y);
                    break;
                case 2:
                case 3:
                    SDL_RenderDrawLines(renderer, triangle.p, 3);
                    SDL_RenderDrawLine(renderer,
                        triangle.p[2].x, triangle.p[2].y,
                        triangle.p[0].x, triangle.p[0].y);
                    break;

                default:
                    break;
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
