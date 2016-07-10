#include <config.h>

#include <stdio.h>

#include <SDL.h>

#include "mapedit/camera.h"
#include "mapedit/canvas.h"
#include "mapedit/prompt.h"
#include "mapedit/tools.h"

static char *filename = NULL;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static struct tool *tool = NULL;

static void filename_ok(const char *text, void *context);
static void update_window_title(void);

int main(int argc, char **argv)
{
    int shutdown = 0;

    if (argc > 1) filename = strdup(argv[1]);

    const uint32_t window_flags = SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("(untitled) - (no tool selected)",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              400, 300,
                              window_flags);

    const uint32_t renderer_flags = SDL_RENDERER_ACCELERATED
                                  | SDL_RENDERER_PRESENTVSYNC;
    renderer = SDL_CreateRenderer(window, -1, renderer_flags);

    tool = &tools[TOOL_NODEDRAW];
    tool->select();

    camera_init(renderer);
    canvas_init(filename);
    prompt_init();

    update_window_title();

    while (!shutdown) {
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                shutdown = 1;
                break;
            }

            if (prompt_handle_event(&e))
                break;

            if (camera_handle_event(&e))
                break;

            if (canvas_handle_event(&e))
                break;

            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_d:
                        tool->deselect();
                        tool = &tools[TOOL_NODEDEL];
                        tool->select();
                        update_window_title();
                        break;
                    case SDLK_n:
                        tool->deselect();
                        tool = &tools[TOOL_NODEDRAW];
                        tool->select();
                        update_window_title();
                        break;
                    case SDLK_s:
                        if (!filename || (e.key.keysym.mod & KMOD_SHIFT)) {
                            prompt("save as: ", filename,
                                   &filename_ok, NULL, NULL);
                        }
                        else {
                            canvas_save(filename);
                        }
                        break;
                    case SDLK_v:
                        tool->deselect();
                        tool = &tools[TOOL_VERTMOVE];
                        tool->select();
                        update_window_title();
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

    tool->deselect();

    canvas_destroy();
    prompt_destroy();
    camera_destroy();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    if (filename) free(filename);
    return 0;
}

static void filename_ok(const char *text, void *context __attribute__((unused)))
{
    if (filename) free(filename);
    filename = strdup(text);
    canvas_save(filename);
    update_window_title();
}

static void update_window_title(void)
{
    char buf[1024] = {0};

    snprintf(buf, sizeof buf, "%s - %s",
        filename ? filename : "(untitled)",
        tool ? tool->desc : "(no tool selected)");

    SDL_SetWindowTitle(window, buf);
}
