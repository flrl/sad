#ifndef MAPEDIT_TOOLS_H
#define MAPEDIT_TOOLS_H

#include <SDL.h>

enum tool_id {
    TOOL_NODEDRAW = 0,

    TOOL_LASTTOOL, /* keep last */
};

struct tool;

typedef void (tool_select)(void);
typedef void (tool_deselect)(void);
typedef int (tool_event_handler)(const SDL_Event *e);
typedef void (tool_renderer)(SDL_Renderer *renderer);

struct tool {
    tool_select        *select;
    tool_deselect      *deselect;
    tool_event_handler *handle_event;
    tool_renderer      *render;
};

extern struct tool tools[TOOL_LASTTOOL];

#endif
