#include <config.h>

#include <stdio.h>

#include <SDL.h>

#include "mapedit/canvas.h"
#include "mapedit/prompt.h"
#include "mapedit/tools.h"

const char *filename = NULL;

static void filename_ok(const char *text, void *context);

int main(int argc, char **argv)
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    struct tool *tool = NULL;
    const SDL_Rect prompt_rect = { 0, 280, 400, 20 };
    int shutdown = 0;

    if (argc > 1) {
        filename = argv[1];
        canvas_load(filename);
    }

    const uint32_t window_flags = 0;
    window = SDL_CreateWindow("hello world",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              400, 300,
                              window_flags);

    const uint32_t renderer_flags = SDL_RENDERER_ACCELERATED
                                  | SDL_RENDERER_PRESENTVSYNC;
    renderer = SDL_CreateRenderer(window, -1, renderer_flags);

    tool = &tools[TOOL_NODEDRAW];
    tool->select();

    prompt_init();

    while (!shutdown) {
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                shutdown = 1;
                break;
            }

            if (prompt_handle_event(&e))
                break;

            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_d:
                        tool->deselect();
                        tool = &tools[TOOL_NODEDEL];
                        tool->select();
                        break;
                    case SDLK_n:
                        tool->deselect();
                        tool = &tools[TOOL_NODEDRAW];
                        tool->select();
                        break;
                    case SDLK_s:
                        if ((e.key.keysym.mod & KMOD_SHIFT)) {
                            prompt_start("filename? ", prompt_rect,
                                         &filename_ok, NULL,
                                         NULL);
                            break;
                        }
                        if (!filename) break;
                        canvas_save(filename);
                        break;
                    case SDLK_v:
                        tool->deselect();
                        tool = &tools[TOOL_VERTMOVE];
                        tool->select();
                        break;
                }
            }

            tool->handle_event(&e);
        }

        if (shutdown) break;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        canvas_render(renderer);

        tool->render(renderer);

        prompt_render(renderer);

        SDL_RenderPresent(renderer);
    }

    canvas_reset();

    tool->deselect();

    prompt_destroy();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}

static void filename_ok(const char *text, void *context __attribute__((unused)))
{
    canvas_save(text);
}
