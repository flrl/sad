ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wextra \
    $(SDL_CFLAGS) $(SDL2_TTF_CFLAGS) $(SDL2_GFX_CFLAGS) $(JANSSON_CFLAGS)
AM_LDFLAGS = $(SDL_LIBS) $(SDL2_TTF_LIBS) $(SDL2_GFX_LIBS) $(JANSSON_LIBS)

bin_PROGRAMS =      \
    sad             \
    tools/mapedit

sad_SOURCES =       \
    engine/main.c

tools_mapedit_SOURCES = \
    mapedit/canvas.c    \
    mapedit/colour.c    \
    mapedit/dcstring.c  \
    mapedit/geometry.c  \
    mapedit/main.c      \
    mapedit/prompt.c    \
    mapedit/selection.c \
    mapedit/tools.c     \
    mapedit/util.c      \
    mapedit/view.c

data_DATA = \
    data/Vera.ttf
