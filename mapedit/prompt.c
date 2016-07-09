#include <assert.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "mapedit/prompt.h"

static TTF_Font *prompt_font = NULL;

typedef void (prompt_ok_cb)(const char *, void *);
typedef void (prompt_cancel_cb)(void *);

static struct prompt_state {
    int in_use;
    char *prompt;
    SDL_Rect dstrect;
    SDL_Rect srcrect;
    prompt_ok_cb *ok_cb;
    prompt_cancel_cb *cancel_cb;
    void *context;
    char *text;
    int dirty;
    SDL_Texture *texture;
} prompt_state;

void prompt_init(void)
{
    if (!TTF_WasInit()) TTF_Init();

    memset(&prompt_state, 0, sizeof prompt_state);

    if (!prompt_font)
        prompt_font = TTF_OpenFont("data/Vera.ttf", 16);
}

void prompt_destroy(void)
{
    if (prompt_font) TTF_CloseFont(prompt_font);
    prompt_font = NULL;

    if (TTF_WasInit()) TTF_Quit();
}

void prompt_start(const char *prompt_text, SDL_Rect dstrect,
                  prompt_ok_cb *ok_cb, prompt_cancel_cb *cancel_cb,
                  void *prompt_context)
{
    struct prompt_state *state = &prompt_state;

    assert(!state->in_use);

    state->in_use = 1;
    state->prompt = strdup(prompt_text);
    state->dstrect = dstrect;
    state->ok_cb = ok_cb;
    state->cancel_cb = cancel_cb;
    state->context = prompt_context;
    state->text = malloc(1024); // FIXME do this properly
    memset(state->text, 0, 1024);
    state->dirty = 1;

    SDL_StartTextInput();
}

static void state_reset(struct prompt_state *state)
{
    if (state->prompt) free(state->prompt);
    if (state->text) free(state->text);
    if (state->texture) SDL_DestroyTexture(state->texture);

    memset(state, 0, sizeof *state);
}

static void done_ok(void)
{
    struct prompt_state *state = &prompt_state;

    assert(state->in_use);

    SDL_StopTextInput();

    if (state->ok_cb)
        state->ok_cb(state->text, state->context);

    state_reset(state);
}

static void done_cancel(void)
{
    struct prompt_state *state = &prompt_state;

    assert(state->in_use);

    SDL_StopTextInput();

    if (state->cancel_cb)
        state->cancel_cb(state->context);

    state_reset(state);
}

int prompt_handle_event(const SDL_Event *e)
{
    struct prompt_state *state = &prompt_state;
    int handled = 0;

    if (!state->in_use) return 0;

    switch (e->type) {
        case SDL_KEYUP:
            handled = 1;
            break;
        case SDL_KEYDOWN:
            if (e->key.keysym.sym == SDLK_RETURN)
                done_ok();
            else if (e->key.keysym.sym == SDLK_ESCAPE)
                done_cancel();
            else if (e->key.keysym.sym == SDLK_BACKSPACE) {
                size_t len = strlen(state->text);
                if (len) {
                    state->text[len - 1] = '\0';
                    state->dirty = 1;
                }
            }
            handled = 1;
            break;
        case SDL_TEXTINPUT:
            strlcat(state->text, e->text.text, 1024);
            state->dirty = 1;
            handled = 1;
            break;
    }

    return handled;
}

void prompt_render(SDL_Renderer *renderer)
{
    static const SDL_Color text_color = { 255, 255, 255, 255 };
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

        len = strlen(state->prompt) + strlen(state->text) + 1;
        tmp = malloc(len);
        snprintf(tmp, len, "%s%s", state->prompt, state->text);
        surface = TTF_RenderUTF8_Blended(prompt_font, tmp, text_color);
        free(tmp);

        if (surface) {
            SDL_Rect viewport;

            SDL_RenderGetViewport(renderer, &viewport);

            if (surface->w > viewport.w) {
                state->srcrect.x = surface->w - viewport.w;
                state->srcrect.y = 0;
                state->srcrect.w = viewport.w;
                state->srcrect.h = surface->h;
            }
            else {
                state->srcrect.x = state->srcrect.y = 0;
                state->srcrect.w = surface->w;
                state->srcrect.h = surface->h;
            }
            state->dstrect.w = state->srcrect.w;
            state->dstrect.h = state->srcrect.h;
            state->texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            state->dirty = 0;
        }
    }

    if (state->texture)
        SDL_RenderCopy(renderer, state->texture, &state->srcrect, &state->dstrect);
}
