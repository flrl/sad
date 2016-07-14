#ifndef MAPEDIT_GEOMETRY_H
#define MAPEDIT_GEOMETRY_H

#include <SDL_rect.h>

#define INV_SQRT2f (1.0f / (M_SQRT2))

SDL_Point addp(SDL_Point a, SDL_Point b);
SDL_Point subtractp(SDL_Point a, SDL_Point b);

int crossp(SDL_Point a, SDL_Point b);

int same_sidep(SDL_Point p1, SDL_Point p2, SDL_Point a, SDL_Point b);

float flengthp(SDL_Point a, SDL_Point b);

float dotf(const float a[2], const float b[2]);
#endif
