#ifndef MAPEDIT_VIEW_H
#define MAPEDIT_VIEW_H

#include <SDL.h>

#include "mapedit/geometry.h"

extern fpoint camera_centre;
extern const float camera_unitpx;
extern float camera_zoom;

void view_init(SDL_Renderer *renderer);
void view_destroy(void);

void view_update(void);

void view_zoom_in(void);
void view_zoom_out(void);

SDL_Point to_screen(fpoint);
fpoint from_screen(SDL_Point p);

int view_handle_event(const SDL_Event *e);
void view_render(SDL_Renderer *renderer);

#endif
