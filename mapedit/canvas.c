#include <stdlib.h>

#include "mapedit/canvas.h"

static struct triangle *triangles = NULL;
static size_t triangles_alloc = 0;
static size_t triangles_count = 0;

static void triangles_ensure(size_t n) {
    if (triangles_alloc >= triangles_count + n) {
        return;
    }
    else if (!triangles) {
        triangles = malloc(n * sizeof triangles[0]);
        triangles_alloc = n;
        triangles_count = 0;
    }
    else {
        while (triangles_alloc < triangles_count + n)
            triangles_alloc += triangles_alloc;

        triangles = realloc(triangles, triangles_alloc * sizeof triangles[0]);
    }
}

void canvas_add_triangle(const struct triangle *triangle) {
    triangles_ensure(1);

    memcpy(&triangles[triangles_count++], triangle, sizeof *triangle);
}

void canvas_render(SDL_Renderer *renderer) {
    size_t i;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    for (i = 0; i < triangles_count; i++) {
        const struct triangle *t = &triangles[i];

        SDL_RenderDrawLines(renderer, t->p, 3);
        SDL_RenderDrawLine(renderer, t->p[2].x, t->p[2].y,
                                     t->p[0].x, t->p[0].y);
    }
}

void canvas_reset(void) {
    if (triangles) free(triangles);
    triangles = NULL;
    triangles_count = triangles_alloc = 0;
}
