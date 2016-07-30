#ifndef MAPEDIT_GEOMETRY_H
#define MAPEDIT_GEOMETRY_H

#include <SDL_rect.h>

#define INV_SQRT2f (1.0f / (M_SQRT2))


SDL_Point paddp(SDL_Point a, SDL_Point b);
SDL_Point psubtractp(SDL_Point a, SDL_Point b);
int crossp(SDL_Point a, SDL_Point b);
int same_sidep(SDL_Point p1, SDL_Point p2, SDL_Point a, SDL_Point b);
float lengthp(SDL_Point a, SDL_Point b);

struct s_f2 {
    float x;
    float y;
};

typedef struct s_f2 fpoint;
typedef struct s_f2 fvector;

float dotfv(fvector a, fvector b);
float crossfv(fvector a, fvector b);
float length2fv(fvector v);
float lengthfv(fvector v);

int same_sidefp(fpoint p1, fpoint p2, fpoint a, fpoint b);

fpoint addfp(fpoint p, fvector v);
fvector subtractfp(fpoint a, fpoint b);
fvector scalefv(fvector v, float l);
fvector projectfv(fvector a, fvector b);
#endif
