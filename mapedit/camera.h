#ifndef MAPEDIT_CAMERA_H
#define MAPEDIT_CAMERA_H

#include <SDL.h>

extern SDL_Point camera_offset;
extern SDL_Point camera_centre;

void camera_init(SDL_Renderer *renderer);
void camera_destroy(void);

SDL_Point to_screen(SDL_Point p);
SDL_Point from_screen(SDL_Point p);

int camera_handle_event(const SDL_Event *e);

#endif
