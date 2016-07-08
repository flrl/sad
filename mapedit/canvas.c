#include <stdlib.h>

#include "mapedit/canvas.h"

static struct node *nodes = NULL;
static size_t nodes_alloc = 0;
static size_t nodes_count = 0;

static void nodes_ensure(size_t n) {
    if (nodes_alloc >= nodes_count + n) {
        return;
    }
    else if (!nodes) {
        nodes = malloc(n * sizeof nodes[0]);
        nodes_alloc = n;
        nodes_count = 0;
    }
    else {
        while (nodes_alloc < nodes_count + n)
            nodes_alloc += nodes_alloc;

        nodes = realloc(nodes, nodes_alloc * sizeof nodes[0]);
    }
}

void canvas_add_node(const struct node *node) {
    nodes_ensure(1);

    memcpy(&nodes[nodes_count++], node, sizeof *node);
}

void canvas_render(SDL_Renderer *renderer) {
    size_t i;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    for (i = 0; i < nodes_count; i++) {
        const struct node *n = &nodes[i];

        SDL_RenderDrawLines(renderer, n->p, 3);
        SDL_RenderDrawLine(renderer, n->p[2].x, n->p[2].y,
                                     n->p[0].x, n->p[0].y);
    }
}

void canvas_reset(void) {
    if (nodes) free(nodes);
    nodes = NULL;
    nodes_count = nodes_alloc = 0;
}
