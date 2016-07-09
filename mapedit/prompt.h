#ifndef MAPEDIT_PROMPT_H
#define MAPEDIT_PROMPT_H

typedef void (prompt_ok_cb)(const char *, void *);
typedef void (prompt_cancel_cb)(void *);

void prompt_init(void);
void prompt_destroy(void);

void prompt_start(const char *prompt_text, SDL_Rect dstrect,
                  prompt_ok_cb *ok_cb, prompt_cancel_cb *cancel_cb,
                  void *prompt_context);

int prompt_handle_event(const SDL_Event *e);

void prompt_render(SDL_Renderer *renderer);

#endif
