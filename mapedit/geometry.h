#ifndef MAPEDIT_GEOMETRY_H
#define MAPEDIT_GEOMETRY_H

#include <SDL_rect.h>

#define INV_SQRT2f (1.0f / (M_SQRT2))


SDL_Point paddp(SDL_Point a, SDL_Point b);
SDL_Point psubtractp(SDL_Point a, SDL_Point b);
int crossp(SDL_Point a, SDL_Point b);
int same_sidep(SDL_Point p1, SDL_Point p2, SDL_Point a, SDL_Point b);
float lengthp(SDL_Point a, SDL_Point b);

typedef struct s_f2 {
    float x;
    float y;
} fvector;

float dotfv(fvector a, fvector b);
float lengthfv(fvector v);
#endif
