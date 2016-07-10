#include "mapedit/geometry.h"

SDL_Point addp(SDL_Point a, SDL_Point b)
{
    SDL_Point result = { a.x + b.x, a.y + b.y };
    return result;
}

SDL_Point subtractp(SDL_Point a, SDL_Point b)
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
    int cp1 = crossp(subtractp(b,a), subtractp(p1,a));
    int cp2 = crossp(subtractp(b,a), subtractp(p2,a));
    if (cp1 >= 0 && cp2 >= 0)
        return 1;
    if (cp1 < 0 && cp2 < 0)
        return 1;

    return 0;
}

float dotf(const float a[2], const float b[2])
{
    return a[0] * b[0] + a[1] * b[1];
}
