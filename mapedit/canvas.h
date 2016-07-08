#ifndef MAPEDIT_CANVAS_H
#define MAPEDIT_CANVAS_H

#include <SDL.h>

struct node {
    SDL_Point p[3];
};

void canvas_add_node(const struct node *node);
void canvas_render(SDL_Renderer *renderer);
void canvas_reset(void);

#endif
