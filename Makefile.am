ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wextra $(SDL_CFLAGS) $(SDL2_TTF_CFLAGS) $(JANSSON_CFLAGS)
AM_LDFLAGS = $(SDL_LIBS) $(SDL2_TTF_LIBS) $(JANSSON_LIBS)

bin_PROGRAMS =      \
    sad             \
    tools/mapedit

sad_SOURCES =       \
    engine/main.c

tools_mapedit_SOURCES = \
    mapedit/canvas.c    \
    mapedit/main.c      \
    mapedit/prompt.c    \
    mapedit/tools.c

data_DATA = \
    data/Vera.ttf
