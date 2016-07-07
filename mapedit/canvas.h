#ifndef MAPEDIT_CANVAS_H
#define MAPEDIT_CANVAS_H

#include <SDL.h>

struct triangle {
    SDL_Point p[3];
};

void canvas_add_triangle(const struct triangle *triangle);
void canvas_render(SDL_Renderer *renderer);
void canvas_reset(void);

#endif
