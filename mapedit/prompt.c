#include <assert.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "mapedit/colour.h"
#include "mapedit/prompt.h"

static TTF_Font *prompt_font = NULL;

typedef void (prompt_ok_cb)(const char *, void *);
typedef void (prompt_cancel_cb)(void *);

static struct prompt_state {
    int in_use;
    int dirty;
    char *query;
    char *input;
    SDL_Texture *texture;
    SDL_Rect fillrect;
    SDL_Rect dstrect;
    SDL_Rect srcrect;
    prompt_ok_cb *ok_cb;
    prompt_cancel_cb *cancel_cb;
    void *cb_context;
} prompt_state;

static void prompt_state_reset(void)
{
    struct prompt_state *state = &prompt_state;

    if (state->query) free(state->query);
    if (state->input) free(state->input);
    if (state->texture) SDL_DestroyTexture(state->texture);

    memset(state, 0, sizeof *state);
}

void prompt_init(void)
{
    if (!TTF_WasInit()) TTF_Init();

    memset(&prompt_state, 0, sizeof prompt_state);

    if (!prompt_font)
        prompt_font = TTF_OpenFont("data/Vera.ttf", 14);
}

void prompt_destroy(void)
{
    if (prompt_font) TTF_CloseFont(prompt_font);
    prompt_font = NULL;
    prompt_state_reset();

    if (TTF_WasInit()) TTF_Quit();
}

void prompt(const char *query, const char *initial,
            prompt_ok_cb *ok_cb, prompt_cancel_cb *cancel_cb,
            void *cb_context)
{
    struct prompt_state *state = &prompt_state;

    assert(!state->in_use);

    state->in_use = 1;
    state->dirty = 1;
    if (query) state->query = strdup(query);
    state->input = malloc(1024); // FIXME do this properly
    memset(state->input, 0, 1024);
    if (initial) strlcpy(state->input, initial, 1024);
    state->ok_cb = ok_cb;
    state->cancel_cb = cancel_cb;
    state->cb_context = cb_context;

    SDL_StartTextInput();
}

static void done_ok(void)
{
    struct prompt_state *state = &prompt_state;

    assert(state->in_use);

    SDL_StopTextInput();

    if (state->ok_cb)
        state->ok_cb(state->input, state->cb_context);

    prompt_state_reset();
}

static void done_cancel(void)
{
    struct prompt_state *state = &prompt_state;

    assert(state->in_use);

    SDL_StopTextInput();

    if (state->cancel_cb)
        state->cancel_cb(state->cb_context);

    prompt_state_reset();
}

int prompt_handle_event(const SDL_Event *e)
{
    struct prompt_state *state = &prompt_state;
    SDL_Point mouse;
    int handled = 0;

    if (!state->in_use) return 0;

    switch (e->type) {
        case SDL_WINDOWEVENT:
            state->dirty = 1;
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouse.x = e->button.x;
            mouse.y = e->button.y;
            if (!SDL_PointInRect(&mouse, &state->fillrect))
                done_cancel();
            handled = 1;
            break;
        case SDL_KEYUP:
            handled = 1;
            break;
        case SDL_KEYDOWN:
            if (e->key.keysym.sym == SDLK_RETURN)
                done_ok();
            else if (e->key.keysym.sym == SDLK_ESCAPE)
                done_cancel();
            else if (e->key.keysym.sym == SDLK_BACKSPACE) {
                size_t len = strlen(state->input);
                if (len) {
                    state->input[len - 1] = '\0';
                    state->dirty = 1;
                }
            }
            handled = 1;
            break;
        case SDL_TEXTINPUT:
            strlcat(state->input, e->text.text, 1024);
            state->dirty = 1;
            handled = 1;
            break;
    }

    return handled;
}

void prompt_render(SDL_Renderer *renderer)
{
    struct prompt_state *state = &prompt_state;

    if (!state->in_use) return;

    if (state->dirty) {
        SDL_Surface *surface;
        char *tmp;
        size_t len;

        if (state->texture) {
            SDL_DestroyTexture(state->texture);
            state->texture = NULL;
        }

        len = strlen(state->query) + strlen(state->input) + 1;
        tmp = malloc(len);
        snprintf(tmp, len, "%s%s", state->query, state->input);
        surface = TTF_RenderUTF8_Blended(prompt_font, tmp, prompt_text);
        free(tmp);

        if (surface) {
            SDL_RenderGetViewport(renderer, &state->fillrect);

            state->fillrect.y = state->fillrect.h - surface->h - 4;
            state->fillrect.h = 4 + surface->h;

            /* crop to window size, allowing 2 pixels padding each end */
            if (surface->w + 4 > state->fillrect.w) {
                state->srcrect.x = surface->w + 4 - state->fillrect.w;
                state->srcrect.w = state->fillrect.w - 4;
            }
            else {
                state->srcrect.x = 0;
                state->srcrect.w = surface->w;
            }
            state->srcrect.y = 0;
            state->srcrect.h = surface->h;

            state->dstrect.x = 2;
            state->dstrect.y = state->fillrect.y + 2;
            state->dstrect.w = state->srcrect.w;
            state->dstrect.h = state->srcrect.h;

            state->texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);

            state->dirty = 0;
        }
    }

    if (state->texture) {
        SDL_SetRenderDrawColor(renderer, C(prompt_fill));
        SDL_RenderFillRect(renderer, &state->fillrect);

        SDL_RenderCopy(renderer, state->texture, &state->srcrect, &state->dstrect);

        /* blinking cursor */
        if (SDL_GetTicks() % 1000 >= 500) {
            SDL_SetRenderDrawColor(renderer, C(prompt_text));
            SDL_RenderDrawLine(renderer,
                               state->dstrect.x + state->dstrect.w,
                               state->dstrect.y,
                               state->dstrect.x + state->dstrect.w,
                               state->dstrect.y + state->dstrect.h);
        }
    }
}
