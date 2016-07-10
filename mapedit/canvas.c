#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/stat.h>

#include <jansson.h>

#include "mapedit/camera.h"
#include "mapedit/canvas.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static struct vertex *verts = NULL;
static size_t verts_alloc = 0;
static size_t verts_count = 0;

static struct node *nodes = NULL;
static size_t nodes_alloc = 0;
static size_t nodes_count = 0;

static SDL_Texture *texture = NULL;
static int is_dirty = 0;

static void canvas_reset(void)
{
    if (nodes) free(nodes);
    nodes = NULL;
    nodes_count = nodes_alloc = 0;

    if (verts) {
        size_t i;
        for (i = 0; i < verts_alloc; i++)
            if (verts[i].nodes) free(verts[i].nodes);
        free(verts);
    }
    verts = NULL;
    verts_count = verts_alloc = 0;

    if (texture) SDL_DestroyTexture(texture);
    texture = NULL;

    camera_centre.x = camera_centre.y = 0;
    is_dirty = 0;
}

void canvas_init(const char *filename)
{
    canvas_reset();

    if (filename) canvas_load(filename);
}

void canvas_destroy(void)
{
    canvas_reset();
}

static void verts_ensure(size_t n)
{
    size_t i;

    if (verts_alloc >= verts_count + n) {
        return;
    }
    else if (!verts) {
        verts = malloc(n * sizeof verts[0]);
        assert(verts != NULL);
        memset(verts, 0, n * sizeof verts[0]);
        for (i = 0; i < n; i++)
            verts[i].id = ID_NONE;
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
        for (i = verts_count; i < verts_alloc; i++)
            verts[i].id = ID_NONE;
    }
}

static void nodes_ensure(size_t n)
{
    size_t i;

    if (nodes_alloc >= nodes_count + n) {
        return;
    }
    else if (!nodes) {
        nodes = malloc(n * sizeof nodes[0]);
        assert(nodes != NULL);
        memset(nodes, 0, n * sizeof nodes[0]);
        for (i = 0; i < n; i++)
            nodes[i].id = ID_NONE;
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
        for (i = nodes_count; i < nodes_alloc; i++)
            nodes[i].id = ID_NONE;
    }
}

static void vertex_nodes_ensure(struct vertex *vertex, size_t n)
{
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

vertex_id canvas_add_vertex(const SDL_Point *p)
{
    verts_ensure(1);
    vertex_id id = verts_count++;
    verts[id].id = id;
    verts[id].p = *p;
    is_dirty = 1;
    return id;
}

void canvas_del_vertex(vertex_id id)
{
    assert(id < verts_count);

    verts[id].id = ID_NONE;
    is_dirty = 1;
}

void canvas_set_vertex(vertex_id id, const SDL_Point *p)
{
    assert(id < verts_count);

    verts[id].p = *p;
    is_dirty = 1;
}

void canvas_offset_vertex(vertex_id id, const SDL_Point *p)
{
    assert(id < verts_count);
    verts[id].p.x += p->x;
    verts[id].p.y += p->y;
    is_dirty = 1;
}

SDL_Point canvas_get_vertex(vertex_id id, int *x, int *y)
{
    assert (id < verts_count);

    if (x) *x = verts[id].p.x;
    if (y) *y = verts[id].p.y;

    return verts[id].p;
}

vertex_id canvas_find_vertex_near(const SDL_Point *p, int snap2, SDL_Point *out)
{
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

const struct vertex *canvas_vertex(vertex_id id)
{
    assert(id < verts_count);

    return &verts[id];
}

static void vertex_add_nodeid(struct vertex *vertex, node_id nodeid)
{
    size_t i;
    assert(nodeid < nodes_count);

    for (i = 0; i < vertex->nodes_count; i++)
        if (vertex->nodes[i] == nodeid) return;

    vertex_nodes_ensure(vertex, 1);
    vertex->nodes[vertex->nodes_count++] = nodeid;
}

static void vertex_del_nodeid(struct vertex *vertex, node_id nodeid)
{
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

node_id canvas_add_node(vertex_id v[3])
{
    size_t i;

    if (v[0] == v[1] || v[1] == v[2] || v[2] == v[0])
        return ID_NONE;

    nodes_ensure(1);

    node_id id = nodes_count++;

    nodes[id].id = id;
    for (i = 0; i < 3; i++) {
        nodes[id].v[i] = v[i];
        vertex_add_nodeid(&verts[v[i]], id);
    }

    is_dirty = 1;
    return id;
}

void canvas_delete_node(node_id id)
{
    size_t i;

    assert(id < nodes_count);

    nodes[id].id = ID_NONE;
    for (i = 0; i < 3; i++) {
        vertex_del_nodeid(&verts[nodes[id].v[i]], id);
        if (verts[nodes[id].v[i]].nodes_count == 0)
            canvas_del_vertex(nodes[id].v[i]);
    }
    is_dirty = 1;
}

static int cross(SDL_Point a, SDL_Point b)
{
    return a.x * b.y - a.y * b.x;
}

static int same_side(const SDL_Point *p1, const SDL_Point *p2,
                     const SDL_Point *a, const SDL_Point *b)
{
    int cp1 = cross(subtract(b,a), subtract(p1,a));
    int cp2 = cross(subtract(b,a), subtract(p2,a));
    if (cp1 >= 0 && cp2 >= 0)
        return 1;
    if (cp1 < 0 && cp2 < 0)
        return 1;

    return 0;
}

node_id canvas_find_node_at(const SDL_Point *p)
{
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

const struct node *canvas_node(node_id id)
{
    assert(id < nodes_count);

    return &nodes[id];
}

int canvas_handle_event(const SDL_Event *e)
{
    is_dirty = 1;
    return 0;
}

void canvas_render(SDL_Renderer *renderer)
{
    if (is_dirty || !texture) {
        SDL_Rect viewport;
        node_id i;

        SDL_RenderGetViewport(renderer, &viewport);

        if (texture) SDL_DestroyTexture(texture);

        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    viewport.w, viewport.h);

        SDL_SetRenderTarget(renderer, texture);

        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 80);
        SDL_RenderClear(renderer);

        SDL_Rect tmprect = { 0, 0, viewport.w, viewport.h };
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderDrawRect(renderer, &tmprect);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

        for (i = 0; i < nodes_count; i++) {
            const struct node *n = &nodes[i];
            if (n->id == ID_NONE) continue;

            SDL_Point points[4] = {
                subtract(&verts[n->v[0]].p, &camera_offset),
                subtract(&verts[n->v[1]].p, &camera_offset),
                subtract(&verts[n->v[2]].p, &camera_offset),
                subtract(&verts[n->v[0]].p, &camera_offset),
            };

            SDL_RenderDrawLines(renderer, points, 4);
        }

        SDL_SetRenderTarget(renderer, NULL);
        is_dirty = 0;
    }

    if (texture)
        SDL_RenderCopy(renderer, texture, NULL, NULL);
}

void canvas_save(const char *filename)
{
    FILE *out = NULL;
    json_t *jverts = NULL;
    json_t *jnodes = NULL;
    json_t *jcanvas = NULL;
    char *out_data;
    size_t i, j;

    assert(filename != NULL);

    jverts = json_object();
    assert(jverts != NULL);

    for (i = 0; i < verts_count; i++) {
        const struct vertex *v = &verts[i];
        char id_buf[16];
        if (v->id == ID_NONE) continue;
        fprintf(stderr, "\rgathering vertices (%zu/%zu)...", i + 1, verts_count);

        json_t *jvp = json_pack("[i, i]", v->p.x, v->p.y);

        json_t *jvn = json_array();
        for (j = 0; j < v->nodes_count; j++) {
            if (v->nodes[j] == ID_NONE) continue;
            json_array_append_new(jvn, json_integer(v->nodes[j]));
        }

        json_t *jv = json_pack("{ s: o, s: o }",
                               "p", jvp,
                               "nodes", jvn);
        snprintf(id_buf, sizeof id_buf, "%u", v->id);

        json_object_set_new(jverts, id_buf, jv);
    }
    fprintf(stderr, " done\n");

    jnodes = json_object();
    assert(jverts != NULL);

    for (i = 0; i < nodes_count; i++) {
        const struct node *n = &nodes[i];
        char id_buf[16];
        if (n->id == ID_NONE) continue;
        fprintf(stderr, "\rgathering nodes (%zu/%zu)...", i + 1, nodes_count);

        json_t *jnv = json_pack("[i, i, i]", n->v[0], n->v[1], n->v[2]);

        json_t *jn = json_pack("{ s: o }", "v", jnv);

        snprintf(id_buf, sizeof id_buf, "%u", n->id);

        json_object_set_new(jnodes, id_buf, jn);
    }
    fprintf(stderr, " done\n");

    jcanvas = json_object();
    assert(jcanvas != NULL);

    json_object_set_new(jcanvas, "vertices", jverts);
    json_object_set_new(jcanvas, "nodes", jnodes);

    out = fopen(filename, "w");
    assert(out != NULL);

    out_data = json_dumps(jcanvas, JSON_PRESERVE_ORDER | JSON_INDENT(2));
    fprintf(out, "%s\n", out_data);
    free(out_data);

    fclose(out);

    fprintf(stderr, "wrote canvas to %s\n", filename);

    json_decref(jcanvas);
}

void canvas_load(const char *filename)
{
    json_t *jcanvas = NULL;
    json_t *jverts = NULL;
    json_t *jnodes = NULL;
    json_error_t error;
    struct stat stat_buf;
    const size_t flags = JSON_REJECT_DUPLICATES;

    canvas_reset();

    if (!filename) return;

    if (stat(filename, &stat_buf) < 0) {
        fprintf(stderr, "unable to read %s: %s\n", filename, strerror(errno));
        return;
    }

    jcanvas = json_load_file(filename, flags, &error);
    if (!jcanvas) {
        // FIXME print the error out?
        return;
    }

    jverts = json_object_get(jcanvas, "vertices");

    if (jverts) {
        const char *key;
        json_t *jvert;

        /* n.b. these won't be in order, so be careful */
        json_object_foreach(jverts, key, jvert) {
            vertex_id id = strtoul(key, NULL, 10);
            if (id >= verts_count)
                verts_ensure(1 + id - verts_count);

            verts[id].id = id;
            verts_count = MAX(verts_count, id + 1);
        }
    }

    jnodes = json_object_get(jcanvas, "nodes");

    if (jnodes) {
        const char *key;
        json_t *jnode;

        /* n.b. these won't be in order, so be careful */
        json_object_foreach(jnodes, key, jnode) {
            node_id id = strtoul(key, NULL, 10);
            if (id >= nodes_count)
                nodes_ensure(1 + id - nodes_count);

            nodes[id].id = id;

            json_unpack(jnode, "{ s: [i, i, i !] !}",
                        "v", &nodes[id].v[0], &nodes[id].v[1], &nodes[id].v[2]);

            nodes_count = MAX(nodes_count, id + 1);
        }
    }

    if (jverts) {
        const char *key;
        json_t *jvert;

        /* n.b. these won't be in order, so be careful */
        json_object_foreach(jverts, key, jvert) {
            vertex_id vid = strtoul(key, NULL, 10);
            assert(verts[vid].id == vid);

            json_t *jnode_ids = NULL;
            json_unpack(jvert, "{ s: [i, i !], s: o !}",
                        "p", &verts[vid].p.x, &verts[vid].p.y,
                        "nodes", &jnode_ids);

            size_t i;
            json_t *jnode_id;
            json_array_foreach(jnode_ids, i, jnode_id) {
                int tmp;
                json_unpack(jnode_id, "i", &tmp);
                vertex_add_nodeid(&verts[vid], tmp);
            }
        }
    }
}
