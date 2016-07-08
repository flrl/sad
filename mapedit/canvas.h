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
    node_id id;
    vertex_id v[3];
};

node_id canvas_add_node(vertex_id v[3]);
void canvas_delete_node(node_id id);
node_id canvas_find_node_at(const SDL_Point *p);
const struct node *canvas_node(node_id id);

vertex_id canvas_add_vertex(const SDL_Point *p);
void canvas_set_vertex(vertex_id id, const SDL_Point *p);
SDL_Point canvas_get_vertex(vertex_id id, int *x, int *y);
vertex_id canvas_find_vertex_near(const SDL_Point *p, int snap2, SDL_Point *out);
const struct vertex *canvas_vertex(vertex_id id);

void canvas_render(SDL_Renderer *renderer);
void canvas_reset(void);

#endif
