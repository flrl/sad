#include <assert.h>

#include "mapedit/geometry.h"

SDL_Point paddp(SDL_Point a, SDL_Point b)
{
    SDL_Point result = { a.x + b.x, a.y + b.y };
    return result;
}

SDL_Point psubtractp(SDL_Point a, SDL_Point b)
{
    SDL_Point result = { a.x - b.x, a.y - b.y };
    return result;
}

int crossp(SDL_Point a, SDL_Point b)
{
    return a.x * b.y - a.y * b.x;
}

/* are p1 and p2 both on the same side of the line ab? */
int same_sidep(SDL_Point p1, SDL_Point p2, SDL_Point a, SDL_Point b)
{
    int cp1 = crossp(psubtractp(b,a), psubtractp(p1,a));
    int cp2 = crossp(psubtractp(b,a), psubtractp(p2,a));
    if (cp1 >= 0 && cp2 >= 0)
        return 1;
    if (cp1 < 0 && cp2 < 0)
        return 1;

    return 0;
}

double lengthp(SDL_Point a, SDL_Point b)
{
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    return sqrt(dx * dx + dy * dy);
}

double dotfv(fvector a, fvector b)
{
    return (double) a.x * b.x + (double) a.y * b.y;
}

double crossfv(fvector a, fvector b)
{
    return (double) a.x * b.y - (double) a.y * b.x;
}

double length2fv(fvector v)
{
    return (double) v.x * v.x + (double) v.y * v.y;
}

double lengthfv(fvector v)
{
    return sqrt(length2fv(v));
}

/* are p1 and p2 both on the same side of the line ab? */
int same_sidefp(fpoint p1, fpoint p2, fpoint a, fpoint b)
{
    double cp1 = crossfv(subtractfp(b, a), subtractfp(p1, a));
    double cp2 = crossfv(subtractfp(b, a), subtractfp(p2, a));
    if (cp1 >= 0 && cp2 >= 0)
        return 1;
    if (cp1 < 0 && cp2 < 0)
        return 1;

    return 0;
}

fpoint addfp(fpoint p, fvector v)
{
    fpoint result = { (double) p.x + v.x, (double) p.y + v.y };
    return result;
}

fvector subtractfp(fpoint a, fpoint b)
{
    fvector result = { (double) a.x - b.x, (double) a.y - b.y };
    return result;
}

fvector scalefv(fvector v, double l)
{
    fvector result = { v.x * l, v.y * l };
    return result;
}

fvector unitfv(fvector v)
{
    fvector result = scalefv(v, 1.0 / sqrt(lengthfv(v)));
    return result;
}

fvector projectfv(fvector a, fvector b)
{
    fvector result = { 0.0f, 0.0f };
    if (length2fv(a) == 0.0 || length2fv(b) == 0.0)
        return result;
    result = scalefv(b, dotfv(a, b) / dotfv(b, b));
    return result;
}

/* cumulative moving average */
fvector averagefv(fvector *cma, size_t *counter, fvector v)
{
    assert(cma != NULL);
    assert(counter != NULL);

    if (*counter) {
        cma->x = (v.x + *counter * (double) cma->x) / (*counter + 1);
        cma->y = (v.y + *counter * (double) cma->y) / (*counter + 1);
        ++*counter;
    }
    else {
        *cma = v;
        *counter = 1;
    }

    return *cma;
}
