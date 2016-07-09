#include <assert.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "mapedit/prompt.h"

static TTF_Font *font = NULL;

typedef void (prompt_ok_cb)(const char *, void *);
typedef void (prompt_cancel_cb)(void *);

static struct prompt_state {
    int in_use;
    char *prompt;
    SDL_Rect dstrect;
    prompt_ok_cb *ok_cb;
    prompt_cancel_cb *cancel_cb;
    void *context;
    char *text;
    int dirty;
    SDL_Texture *texture;
} prompt_state = {0};

void prompt_init(void)
{
    if (!TTF_WasInit()) TTF_Init();

    font = TTF_OpenFont("data/Vera.ttf", 16);
}

void prompt_destroy(void)
{
    if (font) TTF_CloseFont(font);
    font = NULL;

    if (TTF_WasInit()) TTF_Quit();
}

void prompt_start(const char *prompt_text, SDL_Rect dstrect,
                  prompt_ok_cb *ok_cb, prompt_cancel_cb *cancel_cb,
                  void *prompt_context)
{
    struct prompt_state *state = &prompt_state;

    assert(!state->in_use);

    fprintf(stderr, "prompting for %s...\n", prompt_text);

    state->in_use = 1;
    state->prompt = strdup(prompt_text);
    state->dstrect = dstrect;
    state->text = malloc(1024); // FIXME do this properly
    memset(state->text, 0, 1024);
    state->ok_cb = ok_cb;
    state->cancel_cb = cancel_cb;
    state->context = prompt_context;

    SDL_StartTextInput();
}

static void done_ok(void)
{
    struct prompt_state *state = &prompt_state;

    assert(state->in_use);

    SDL_StopTextInput();

    state->ok_cb(state->text, state->context);

    free(state->text);
    free(state->prompt);
    memset(state, 0, sizeof *state);
}

static void done_cancel(void)
{
    struct prompt_state *state = &prompt_state;

    assert(state->in_use);

    SDL_StopTextInput();

    state->cancel_cb(state->context);

    free(state->text);
    free(state->prompt);
    memset(state, 0, sizeof *state);
}

int prompt_handle_event(const SDL_Event *e)
{
    struct prompt_state *state = &prompt_state;
    int handled = 0;

    if (!state->in_use) return 0;

    switch (e->type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            if (e->key.keysym.sym == SDLK_RETURN)
                done_ok();
            else if (e->key.keysym.sym == SDLK_ESCAPE)
                done_cancel();
            handled = 1;
            break;
        case SDL_TEXTINPUT:
            fprintf(stderr, "textinput: %s\n", e->text.text);
            strlcat(state->text, e->text.text, 1024);
            state->dirty = 1;
            handled = 1;
            break;
    }

    return handled;
}

void prompt_render(SDL_Renderer *renderer)
{
    static const SDL_Color color = { 255, 255, 255, 255 };
    struct prompt_state *state = &prompt_state;

    if (!state->in_use) return;

    if (state->dirty || !state->texture) {
        if (state->texture)
            SDL_DestroyTexture(state->texture);

        // FIXME do the prompt too
        SDL_Surface *surface = TTF_RenderUTF8_Blended(font, state->text, color);
        state->texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    // FIXME deal with scaling and shit
    SDL_RenderCopy(renderer, state->texture, NULL, &state->dstrect);
}
