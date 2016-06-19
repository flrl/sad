#include <config.h>

#include <stdio.h>

#include <SDL.h>

int main(int argc __attribute__((unused)),
         char **argv __attribute__((unused)))
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    int shutdown = 0;

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

        if (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    shutdown = 1;
                    break;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
