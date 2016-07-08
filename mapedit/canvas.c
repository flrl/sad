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

static void vertex_nodes_ensure(struct vertex *vertex, size_t n) {
    size_t i;

    if (vertex->nodes_alloc >= vertex->nodes_count + n) {
        return;
    }
    else if (!vertex->nodes) {
        vertex->nodes = malloc(n * sizeof vertex->nodes[0]);
        assert(vertex->nodes != NULL);
        for (i = 0; i < n; i++)
            vertex->nodes[i] = ID_NONE;
        vertex->nodes_alloc = n;
        vertex->nodes_count = 0;
    }
    else {
        while (vertex->nodes_alloc < vertex->nodes_count + n)
            vertex->nodes_alloc += vertex->nodes_alloc;

        vertex->nodes = realloc(vertex->nodes,
                                vertex->nodes_alloc * sizeof vertex->nodes[0]);
        assert(vertex->nodes != NULL);
        for (i = vertex->nodes_count; i < vertex->nodes_alloc; i++)
            vertex->nodes[i] = ID_NONE;
    }
}

vertex_id canvas_add_vertex(const SDL_Point *p) {
    verts_ensure(1);
    vertex_id id = verts_count++;
    verts[id].id = id;
    memcpy(&verts[id].p, p, sizeof *p);
    return id;
}

void canvas_del_vertex(vertex_id id) {
    assert(id < verts_count);

    verts[id].id = ID_NONE;
}

void canvas_set_vertex(vertex_id id, const SDL_Point *p) {
    assert(id < verts_count);

    verts[id].p = *p;
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
        if (v->id == ID_NONE) continue;
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

const struct vertex *canvas_vertex(vertex_id id) {
    assert(id < verts_count);

    return &verts[id];
}

static void vertex_add_nodeid(struct vertex *vertex, node_id nodeid) {
    size_t i;
    assert(nodeid < nodes_count);

    for (i = 0; i < vertex->nodes_count; i++)
        if (vertex->nodes[i] == nodeid) return;

    vertex_nodes_ensure(vertex, 1);
    vertex->nodes[vertex->nodes_count++] = nodeid;
}

static void vertex_del_nodeid(struct vertex *vertex, node_id nodeid) {
    size_t i;
    assert(nodeid < nodes_count);

    for (i = 0; i < vertex->nodes_count; i++)
        if (vertex->nodes[i] == nodeid) break;

    if (i == vertex->nodes_count)
        return; /* not found */

    for (; i < vertex->nodes_count; i++)
        vertex->nodes[i] = vertex->nodes[i + 1];

    vertex->nodes_count --;

    for (; i < vertex->nodes_alloc; i++)
        vertex->nodes[i] = ID_NONE;
}

node_id canvas_add_node(vertex_id v[3]) {
    size_t i;

    nodes_ensure(1);

    node_id id = nodes_count++;

    nodes[id].id = id;
    for (i = 0; i < 3; i++) {
        nodes[id].v[i] = v[i];
        vertex_add_nodeid(&verts[v[i]], id);
    }

    return id;
}

void canvas_delete_node(node_id id) {
    size_t i;

    assert(id < nodes_count);

    nodes[id].id = ID_NONE;
    for (i = 0; i < 3; i++) {
        vertex_del_nodeid(&verts[nodes[id].v[i]], id);
        if (verts[nodes[id].v[i]].nodes_count == 0)
            canvas_del_vertex(nodes[id].v[i]);
    }
}

static int cross(SDL_Point a, SDL_Point b) {
    return a.x * b.y - a.y * b.x;
}

static SDL_Point subtract(const SDL_Point *a, const SDL_Point *b) {
    SDL_Point tmp;
    tmp.x = a->x - b->x;
    tmp.y = a->y - b->y;
    return tmp;
}

static int same_side(const SDL_Point *p1, const SDL_Point *p2,
                     const SDL_Point *a, const SDL_Point *b) {
    int cp1 = cross(subtract(b,a), subtract(p1,a));
    int cp2 = cross(subtract(b,a), subtract(p2,a));
    if (cp1 >= 0 && cp2 >= 0)
        return 1;
    if (cp1 < 0 && cp2 < 0)
        return 1;

    return 0;
}

node_id canvas_find_node_at(const SDL_Point *p) {
    node_id id;

    for (id = 0; id < nodes_count; id++) {
        struct node *node = &nodes[id];
        if (node->id == ID_NONE) continue;

        if (!same_side(p, &verts[node->v[2]].p, &verts[node->v[0]].p, &verts[node->v[1]].p))
            continue;

        if (!same_side(p, &verts[node->v[0]].p, &verts[node->v[1]].p, &verts[node->v[2]].p))
            continue;

        if (!same_side(p, &verts[node->v[1]].p, &verts[node->v[2]].p, &verts[node->v[0]].p))
            continue;

        return id;
    }

    return ID_NONE;
}

const struct node *canvas_node(node_id id) {
    assert(id < nodes_count);

    return &nodes[id];
}

void canvas_render(SDL_Renderer *renderer) {
    node_id i;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    for (i = 0; i < nodes_count; i++) {
        const struct node *n = &nodes[i];
        if (n->id == ID_NONE) continue;

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
