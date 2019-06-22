ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wextra

bin_PROGRAMS =      \
    sad             \
    tools/mapedit

sad_CFLAGS = $(SDL_CFLAGS)
sad_LDADD = $(SDL_LIBS)
sad_SOURCES =       \
    engine/main.c

tools_mapedit_CFLAGS = $(SDL2_TTF_CFLAGS) $(SDL2_GFX_CFLAGS) $(SDL_CFLAGS) $(JANSSON_CFLAGS)
tools_mapedit_LDADD = $(SDL2_TTF_LIBS) $(SDL2_GFX_LIBS) $(SDL_LIBS) $(JANSSON_LIBS)
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
