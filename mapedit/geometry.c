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

float lengthp(SDL_Point a, SDL_Point b)
{
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
}

float dotfv(fvector a, fvector b)
{
    return a.x * b.x + a.y * b.y;
}

float crossfv(fvector a, fvector b)
{
    return a.x * b.y - a.y * b.x;
}

float length2fv(fvector v)
{
    return v.x * v.x + v.y * v.y;
}

float lengthfv(fvector v)
{
    return sqrtf(length2fv(v));
}

/* are p1 and p2 both on the same side of the line ab? */
int same_sidefp(fpoint p1, fpoint p2, fpoint a, fpoint b)
{
    float cp1 = crossfv(subtractfp(b, a), subtractfp(p1, a));
    float cp2 = crossfv(subtractfp(b, a), subtractfp(p2, a));
    if (cp1 >= 0 && cp2 >= 0)
        return 1;
    if (cp1 < 0 && cp2 < 0)
        return 1;

    return 0;
}

fpoint addfp(fpoint p, fvector v)
{
    fpoint result = { p.x + v.x, p.y + v.y };
    return result;
}

fvector subtractfp(fpoint a, fpoint b)
{
    fvector result = { a.x - b.x, a.y - b.y };
    return result;
}

fvector scalefv(fvector v, float l)
{
    fvector result = { v.x * l, v.y * l };
    return result;
}
