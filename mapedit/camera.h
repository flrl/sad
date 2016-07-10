#ifndef MAPEDIT_CAMERA_H
#define MAPEDIT_CAMERA_H

#include <SDL.h>

extern SDL_Point camera_offset;
extern SDL_Point camera_centre;

void camera_init(SDL_Renderer *renderer);
void camera_destroy(void);

int camera_handle_event(const SDL_Event *e);

SDL_Point add(SDL_Point a, SDL_Point b);
SDL_Point subtract(const SDL_Point *a, const SDL_Point *b);

#endif
