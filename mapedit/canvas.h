#ifndef MAPEDIT_CANVAS_H
#define MAPEDIT_CANVAS_H

#include <SDL.h>

typedef unsigned node_id;
typedef unsigned vertex_id;
#define ID_NONE ((unsigned)(-1))

struct vertex {
    vertex_id id;
    SDL_Point p;
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
node_id canvas_find_node_at(SDL_Point p);
const struct node *canvas_node(node_id id);

vertex_id canvas_add_vertex(SDL_Point p);
void canvas_delete_vertex(vertex_id id);
void canvas_edit_vertex(vertex_id id, const SDL_Point *p_abs, const SDL_Point *p_rel);
vertex_id canvas_find_vertex_near(SDL_Point p, int snap2, SDL_Point *out);
const struct vertex *canvas_vertex(vertex_id id);

int canvas_handle_event(const SDL_Event *e);
void canvas_render(SDL_Renderer *renderer);

int canvas_is_dirty(void);
void canvas_save(const char *filename);
void canvas_load(const char *filename);

#endif
