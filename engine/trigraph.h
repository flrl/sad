#ifndef ENGINE_TRIGRAPH_H
#define ENGINE_TRIGRAPH_H

#include <stdint.h>

#define INDEX_NULL (UINT16_MAX)

struct vertex {
    float x;
    float y;
};

struct trinode {
    uint16_t vertices[3];
    uint16_t neighbours[3];
    uint8_t  costs[3];
    uint8_t  __pad;
};

struct trigraph {
    struct trinode *nodes;
    size_t nodes_size;
    size_t nodes_count;
    struct vertex *vertices;
    size_t vertices_size;
    size_t vertices_count;
};

#endif
