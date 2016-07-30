#ifndef MAPEDIT_COLOUR_H
#define MAPEDIT_COLOUR_H

#include <SDL_pixels.h>

#define C(c) (c).r,(c).g,(c).b,(c).a

extern const SDL_Color main_background;

extern const SDL_Color view_background;
extern const SDL_Color view_node;
extern const SDL_Color view_edge;
extern const SDL_Color view_grid_coord;
extern const SDL_Color view_grid_major;
extern const SDL_Color view_grid_minor;
extern const SDL_Color view_mouse_pos;

extern const SDL_Color tool_draw0;
extern const SDL_Color tool_draw1;
extern const SDL_Color tool_move_high;
extern const SDL_Color tool_del_high;

extern const SDL_Color prompt_text;
extern const SDL_Color prompt_fill;

#endif
