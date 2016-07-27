#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/stat.h>

#include <SDL2_gfxPrimitives.h>
#include <jansson.h>

#include "mapedit/canvas.h"
#include "mapedit/geometry.h"
#include "mapedit/util.h"
#include "mapedit/view.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static struct vertex *verts = NULL;
static size_t verts_alloc = 0;
static size_t verts_count = 0;

static struct node *nodes = NULL;
static size_t nodes_alloc = 0;
static size_t nodes_count = 0;

static SDL_Texture *texture = NULL;
static int is_data_dirty = 0;
static int is_view_dirty = 0;

#define canvas_dirty() do { is_data_dirty = is_view_dirty = 1; } while (0)

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
    is_data_dirty = is_view_dirty = 0;
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

vertex_id canvas_add_vertex(fpoint p)
{
    verts_ensure(1);
    vertex_id id = verts_count++;
    verts[id].id = id;
    verts[id].p = p;
    canvas_dirty();
    return id;
}

void canvas_delete_vertex(vertex_id id)
{
    assert(id < verts_count);

    verts[id].id = ID_NONE;
    canvas_dirty();
}

void canvas_edit_vertex(vertex_id id, const fpoint *p_abs, const fvector *p_rel)
{
    assert(id < verts_count);

    if (p_abs) {
        verts[id].p = *p_abs;
        canvas_dirty();
    }
    else if (p_rel) {
        verts[id].p = addfp(verts[id].p, *p_rel);
        canvas_dirty();
    }
}

vertex_id canvas_find_vertex_near(fpoint p, float snap2, fpoint *out)
{
    vertex_id id;

    assert (snap2 >= 0);

    for (id = 0; id < verts_count; id++) {
        const struct vertex *v = &verts[id];
        if (v->id == ID_NONE) continue;
        if (length2fv(subtractfp(p, v->p)) <= snap2) {
            if (out)
                *out = verts[id].p;
            return id;
        }
    }

    return ID_NONE;
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
    float winding;

    if (v[0] == v[1] || v[1] == v[2] || v[2] == v[0])
        return ID_NONE;

    nodes_ensure(1);
    node_id id = nodes_count++;
    nodes[id].id = id;

    winding = crossfv(subtractfp(verts[v[1]].p, verts[v[0]].p),
                      subtractfp(verts[v[2]].p, verts[v[0]].p));
    if (winding < 0) {
        vertex_id tmp = v[1];
        v[1] = v[2];
        v[2] = tmp;
    }

    for (i = 0; i < 3; i++) {
        assert(verts[v[i]].id == v[i]);
        nodes[id].v[i] = v[i];
        vertex_add_nodeid(&verts[v[i]], id);
    }

    canvas_dirty();
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
            canvas_delete_vertex(nodes[id].v[i]);
    }
    canvas_dirty();
}

node_id canvas_find_node_at(fpoint p)
{
    node_id id;

    for (id = 0; id < nodes_count; id++) {
        struct node *node = &nodes[id];
        if (node->id == ID_NONE) continue;

        if (!same_sidefp(p, verts[node->v[2]].p, verts[node->v[0]].p, verts[node->v[1]].p))
            continue;

        if (!same_sidefp(p, verts[node->v[0]].p, verts[node->v[1]].p, verts[node->v[2]].p))
            continue;

        if (!same_sidefp(p, verts[node->v[1]].p, verts[node->v[2]].p, verts[node->v[0]].p))
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
    switch (e->type) {
        case SDL_WINDOWEVENT:
        case SDL_MOUSEWHEEL:
            is_view_dirty = 1;
            break;
        case SDL_KEYUP:
            if (e->key.keysym.sym == SDLK_z) {
                camera_zoom_in();
                is_view_dirty = 1;
                return 1;
            }
            else if (e->key.keysym.sym == SDLK_x) {
                camera_zoom_out();
                is_view_dirty = 1;
                return 1;
            }
            break;
    }

    return 0;
}

void canvas_render(SDL_Renderer *renderer)
{
    if (is_view_dirty || !texture) {
        SDL_Rect viewport;
        SDL_Point p;
        node_id i;
        fpoint tl, br;
        float x, y;
        const float grid_subdiv = 0.25f;

        SDL_RenderGetViewport(renderer, &viewport);

        if (texture) SDL_DestroyTexture(texture);

        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    viewport.w, viewport.h);

        SDL_SetRenderTarget(renderer, texture);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (i = 0; i < nodes_count; i++) {
            const struct node *n = &nodes[i];
            if (n->id == ID_NONE) continue;

            SDL_Point points[3] = {
                to_screen(verts[n->v[0]].p),
                to_screen(verts[n->v[1]].p),
                to_screen(verts[n->v[2]].p),
            };

            filledTrigonRGBA(renderer,
                             points[0].x, points[0].y,
                             points[1].x, points[1].y,
                             points[2].x, points[2].y,
                             80, 80, 80, 255);
            trigonRGBA(renderer,
                       points[0].x, points[0].y,
                       points[1].x, points[1].y,
                       points[2].x, points[2].y,
                       120, 120, 120, 255);
        }

        p.x = viewport.x; p.y = viewport.y;
        tl = from_screen(p);
        p.x += viewport.w; p.y += viewport.h;
        br = from_screen(p);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        for (y = floorf(tl.y) - grid_subdiv;
             y <= ceilf(br.y) + grid_subdiv;
             y += grid_subdiv) {
            fpoint fstart = { tl.x, y }, fend = { br.x, y };
            SDL_Point start, end;

            start = to_screen(fstart);
            end = to_screen(fend);

            if (truncf(y) == y)
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 48);
            else
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 32);

            SDL_RenderDrawLine(renderer,
                               start.x, start.y,
                               end.x, end.y);
        }

        for (x = floorf(tl.x) - grid_subdiv;
             x <= ceilf(br.x) + grid_subdiv;
             x += grid_subdiv) {
            fpoint fstart = { x, tl.y }, fend = { x, br.y };
            SDL_Point start, end;

            start = to_screen(fstart);
            end = to_screen(fend);

            if (truncf(x) == x)
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 48);
            else
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 32);

            SDL_RenderDrawLine(renderer,
                               start.x, start.y,
                               end.x, end.y);
        }

        SDL_SetRenderTarget(renderer, NULL);
        is_view_dirty = 0;
    }

    if (texture)
        SDL_RenderCopy(renderer, texture, NULL, NULL);
}

int canvas_is_dirty(void)
{
    return is_data_dirty;
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

        json_t *jvp = json_pack("[f, f]", v->p.x, v->p.y);

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

    json_object_set_new(jcanvas, "scale", json_real(camera_unitpx));
    json_object_set_new(jcanvas, "vertices", jverts);
    json_object_set_new(jcanvas, "nodes", jnodes);

    out = fopen_with_backup(filename);
    assert(out != NULL);
    out_data = json_dumps(jcanvas, JSON_PRESERVE_ORDER | JSON_INDENT(2));
    assert(out_data != NULL);
    fprintf(out, "%s\n", out_data);
    free(out_data);
    fclose(out);

    json_decref(jcanvas);

    fprintf(stderr, "wrote canvas to %s\n", filename);
    is_data_dirty = 0;
}

void canvas_load(const char *filename)
{
    json_t *jcanvas = NULL;
    json_t *jverts = NULL;
    json_t *jnodes = NULL;
    json_t *jscale = NULL;
    json_error_t error;
    struct stat stat_buf;
    const size_t flags = JSON_REJECT_DUPLICATES;
    float scale;

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

    jscale = json_object_get(jcanvas, "scale");
    if (jscale) {
        scale = json_number_value(jscale) / camera_unitpx;
    }
    else {
        scale = 1 / camera_unitpx;
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
            double x, y;
            vertex_id vid = strtoul(key, NULL, 10);
            assert(verts[vid].id == vid);

            json_t *jnode_ids = NULL;
            json_unpack(jvert, "{ s: [F, F !], s: o !}",
                        "p", &x, &y,
                        "nodes", &jnode_ids);

            verts[vid].p.x = x * scale;
            verts[vid].p.y = y * scale;

            size_t i;
            json_t *jnode_id;
            json_array_foreach(jnode_ids, i, jnode_id) {
                int tmp;
                json_unpack(jnode_id, "i", &tmp);
                vertex_add_nodeid(&verts[vid], tmp);
            }
        }
    }

    is_view_dirty = 1;
}
