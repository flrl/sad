#ifndef MAPEDIT_TOOLS_H
#define MAPEDIT_TOOLS_H

#include <SDL.h>

enum tool_id {
    TOOL_NODEDRAW = 0,
    TOOL_VERTMOVE,
    TOOL_NODEDEL,
    TOOL_ARCDRAW,
    TOOL_RECTDRAW,
    TOOL_VERTSEL,

    TOOL_LASTTOOL, /* keep last */
};

typedef void (tool_select)(void);
typedef void (tool_deselect)(void);
typedef int (tool_event_handler)(const SDL_Event *e);
typedef void (tool_renderer)(SDL_Renderer *renderer);

struct tool {
    tool_select        *select;
    tool_deselect      *deselect;
    tool_event_handler *handle_event;
    tool_renderer      *render;
    const char         *desc;
};

extern struct tool tools[TOOL_LASTTOOL];

#endif
