#ifndef MAPEDIT_VIEW_H
#define MAPEDIT_VIEW_H

#include <SDL.h>

#include "mapedit/geometry.h"

extern const float camera_unitpx;

void view_init(SDL_Renderer *renderer);
void view_destroy(void);

void view_update(void);

void view_zoom_in(void);
void view_zoom_out(void);

int scalar_to_screen(float f);
float scalar_from_screen(int i);

SDL_Point vectorxy_to_screen(float x, float y);
fvector vector_from_screenxy(int x, int y);
#define vector_to_screen(v) vectorxy_to_screen((v).x, (v).y)
#define vector_from_screen(v) vector_from_screenxy((v).x, (v).y)

SDL_Point pointxy_to_screen(float x, float y);
fpoint point_from_screenxy(int x, int y);
#define point_to_screen(p) pointxy_to_screen((p).x, (p).y)
#define point_from_screen(p) point_from_screenxy((p).x, (p).y)

int view_handle_event(const SDL_Event *e);
void view_render(SDL_Renderer *renderer);

#endif
