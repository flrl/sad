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

float lengthfv(fvector v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}
