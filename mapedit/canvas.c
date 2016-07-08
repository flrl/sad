#include <assert.h>
#include <stdlib.h>

#include "mapedit/canvas.h"

static struct vertex *verts = NULL;
static size_t verts_alloc = 0;
static size_t verts_count = 0;

static struct node *nodes = NULL;
static size_t nodes_alloc = 0;
static size_t nodes_count = 0;

static void verts_ensure(size_t n) {
    if (verts_alloc >= verts_count + n) {
        return;
    }
    else if (!verts) {
        verts = malloc(n * sizeof verts[0]);
        assert(verts != NULL);
        memset(verts, 0, n * sizeof verts[0]);
        verts_alloc = n;
        verts_count = 0;
    }
    else {
        while (verts_alloc < verts_count + n)
            verts_alloc += verts_alloc;

        verts = realloc(verts, verts_alloc * sizeof verts[0]);
        assert(verts != NULL);
        memset(&verts[verts_count],
               0,
               (verts_alloc - verts_count) * sizeof verts[0]);
    }
}

static void nodes_ensure(size_t n) {
    if (nodes_alloc >= nodes_count + n) {
        return;
    }
    else if (!nodes) {
        nodes = malloc(n * sizeof nodes[0]);
        assert(nodes != NULL);
        memset(nodes, 0, n * sizeof nodes[0]);
        nodes_alloc = n;
        nodes_count = 0;
    }
    else {
        while (nodes_alloc < nodes_count + n)
            nodes_alloc += nodes_alloc;

        nodes = realloc(nodes, nodes_alloc * sizeof nodes[0]);
        assert(nodes != NULL);
        memset(&nodes[nodes_count],
               0,
               (nodes_alloc - nodes_count) * sizeof nodes[0]);
    }
}

vertex_id canvas_add_vertex(const SDL_Point *p) {
    verts_ensure(1);
    vertex_id id = verts_count++;
    memcpy(&verts[id].p, p, sizeof *p);
    return id;
}

void canvas_vertex_set(vertex_id id, int x, int y) {
    assert(id < verts_count);

    verts[id].p.x = x;
    verts[id].p.y = y;
}

SDL_Point canvas_get_vertex(vertex_id id, int *x, int *y) {
    assert (id < verts_count);

    if (x) *x = verts[id].p.x;
    if (y) *y = verts[id].p.y;

    return verts[id].p;
}

vertex_id canvas_find_vertex_near(const SDL_Point *p, int snap2, SDL_Point *out) {
    vertex_id id;

    assert (snap2 >= 0);

    for (id = 0; id < verts_count; id++) {
        const struct vertex *v = &verts[id];
        if (p->x == v->p.x && p->y == v->p.y) {
            goto found;
        }
        else {
            int dx = p->x - v->p.x;
            int dy = p->y - v->p.y;
            int d2 = dx * dx + dy * dy;
            if (d2 <= snap2)
                goto found;
        }
    }

    return ID_NONE;

found:
    if (out) *out = verts[id].p;
    return id;
}

node_id   canvas_add_node(vertex_id v[3]) {
    nodes_ensure(1);

    node_id id = nodes_count++;

    nodes[id].v[0] = v[0];
    nodes[id].v[1] = v[1];
    nodes[id].v[2] = v[2];
    return id;
}

void canvas_render(SDL_Renderer *renderer) {
    node_id i;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    for (i = 0; i < nodes_count; i++) {
        const struct node *n = &nodes[i];
        SDL_Point points[4] = {
            verts[n->v[0]].p,
            verts[n->v[1]].p,
            verts[n->v[2]].p,
            verts[n->v[0]].p,
        };

        SDL_RenderDrawLines(renderer, points, 4);
    }
}

void canvas_reset(void) {
    if (nodes) free(nodes);
    nodes = NULL;
    nodes_count = nodes_alloc = 0;

    if (verts) free(verts);
    verts = NULL;
    verts_count = verts_alloc = 0;
}
