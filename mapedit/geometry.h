#ifndef MAPEDIT_GEOMETRY_H
#define MAPEDIT_GEOMETRY_H

#include <SDL_rect.h>

#define INV_SQRT2 (1.0 / (M_SQRT2))

SDL_Point paddp(SDL_Point a, SDL_Point b);
SDL_Point psubtractp(SDL_Point a, SDL_Point b);
int crossp(SDL_Point a, SDL_Point b);
int same_sidep(SDL_Point p1, SDL_Point p2, SDL_Point a, SDL_Point b);
double lengthp(SDL_Point a, SDL_Point b);

struct s_f2 {
    float x;
    float y;
};

typedef struct s_f2 fpoint;
typedef struct s_f2 fvector;

double dotfv(fvector a, fvector b);
double crossfv(fvector a, fvector b);
double length2fv(fvector v);
double lengthfv(fvector v);

int same_sidefp(fpoint p1, fpoint p2, fpoint a, fpoint b);

fpoint addfp(fpoint p, fvector v);
fvector subtractfp(fpoint a, fpoint b);
fvector scalefv(fvector v, double l);
fvector unitfv(fvector v);
fvector projectfv(fvector a, fvector b);

fvector averagefv(fvector *cma, size_t *counter, fvector v);

#endif
