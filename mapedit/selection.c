#include <assert.h>
#include <stdint.h>

#include "mapedit/canvas.h"
#include "mapedit/selection.h"

static uint8_t selected_nodes[8192] = {0};
static uint8_t selected_verts[8192] = {0};

void selection_clear_nodes(void)
{
    memset(selected_nodes, 0, sizeof selected_nodes);
}

int selection_has_node(node_id id)
{
    return ((selected_nodes[id >> 3] >> (id & 0x07)) & 0x01);
}

void selection_add_node(node_id id)
{
    if (id == ID_NONE) return;
    assert(id <= ID_MAX);

    selected_nodes[id >> 3] |= 0x01 << (id & 0x07);
}

void selection_remove_node(node_id id)
{
    if (id == ID_NONE) return;
    assert(id <= ID_MAX);

    selected_nodes[id >> 3] &= ~(0x01 << (id & 0x07));
}

void selection_clear_vertices(void)
{
    memset(selected_verts, 0, sizeof selected_verts);
}

int selection_has_vertex(vertex_id id)
{
    return ((selected_verts[id >> 3] >> (id & 0x07)) & 0x01);
}

void selection_add_vertex(vertex_id id)
{
    if (id == ID_NONE) return;
    assert(id <= ID_MAX);

    selected_verts[id >> 3] |= 0x01 << (id & 0x07);
}

void selection_remove_vertex(vertex_id id)
{
    if (id == ID_NONE) return;
    assert(id <= ID_MAX);

    selected_verts[id >> 3] &= ~(0x01 << (id & 0x07));
}
