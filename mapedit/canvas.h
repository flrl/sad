#ifndef MAPEDIT_CANVAS_H
#define MAPEDIT_CANVAS_H

#include <SDL.h>

typedef unsigned node_id;
typedef unsigned vertex_id;
#define ID_NONE ((unsigned)(-1))

struct vertex {
    SDL_Point p;
};

struct node {
    vertex_id v[3];
};

node_id   canvas_add_node(vertex_id v[3]);
vertex_id canvas_add_vertex(const SDL_Point *p);

void canvas_set_vertex(vertex_id id, const SDL_Point *p);

SDL_Point canvas_get_vertex(vertex_id id, int *x, int *y);

vertex_id canvas_find_vertex_near(const SDL_Point *p, int snap2, SDL_Point *out);

void canvas_render(SDL_Renderer *renderer);
void canvas_reset(void);

#endif
