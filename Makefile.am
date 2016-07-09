ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wextra $(SDL_CFLAGS) $(JANSSON_CFLAGS)
AM_LDFLAGS = $(SDL_LIBS) $(JANSSON_LIBS)

bin_PROGRAMS =      \
    sad             \
    tools/mapedit

sad_SOURCES =       \
    engine/main.c

tools_mapedit_SOURCES = \
    mapedit/canvas.c    \
    mapedit/main.c      \
    mapedit/tools.c
