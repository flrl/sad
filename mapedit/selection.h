#ifndef MAPEDIT_SELECTION_H
#define MAPEDIT_SELECTION_H

#include "mapedit/canvas.h"

void selection_clear_nodes(void);
int selection_has_node(node_id id);
void selection_add_node(node_id id);
void selection_remove_node(node_id id);

void selection_clear_vertices(void);
int selection_has_vertex(vertex_id id);
void selection_add_vertex(vertex_id id);
void selection_remove_vertex(vertex_id id);

#endif
