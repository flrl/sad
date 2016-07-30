#ifndef MAPEDIT_CANVAS_H
#define MAPEDIT_CANVAS_H

#include <SDL.h>

#include "mapedit/geometry.h"

typedef unsigned node_id;
typedef unsigned vertex_id;
#define ID_NONE ((unsigned)(-1))
#define ID_MAX  ((unsigned)(0xFFFE))

struct vertex {
    vertex_id id;
    fpoint p;
    node_id *nodes;
    size_t nodes_alloc;
    size_t nodes_count;
};

struct node {
    node_id id;
    vertex_id v[3];
};

void canvas_init(const char *filename);
void canvas_destroy(void);

node_id canvas_add_node(vertex_id v[3]);
void canvas_delete_node(node_id id);
node_id canvas_find_node_at(fpoint p);
const struct node *canvas_node(node_id id);

vertex_id canvas_add_vertex(fpoint p);
void canvas_delete_vertex(vertex_id id);
void canvas_edit_vertex(vertex_id id, const fpoint *p_abs, const fvector *p_rel);
vertex_id canvas_find_vertex_near(fpoint p, double snap, fpoint *out);
const struct vertex *canvas_vertex(vertex_id id);

int canvas_handle_event(const SDL_Event *e);
void canvas_render(SDL_Renderer *renderer);

int canvas_is_dirty(void);
void canvas_save(const char *filename);
void canvas_load(const char *filename);

#endif
