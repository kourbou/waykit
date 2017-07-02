#ifndef WK_EVENT_H
#define WK_EVENT_H

#include <cairo/cairo.h>
#include "display.h"
#include "window.h"

#define WK_MAX_EVENTS   500

typedef int (*wk_context_func)(struct wk_context *ctx,
        struct wk_event *ev, cairo_t *cairo);

struct wk_event {
    struct wk_event *prev;

    int repeat;
    int type;
    void *data;
}

struct wk_context {
    struct wk_window  *win;
    struct wk_context *prev;
    struct wk_context *next;
    wk_context_func callback;

    cairo_surface_t surface;
    cairo_t cairo;

    int queue_enq, queue_deq;
    struct wk_event queue[WK_MAX_EVENTS];

    int layer, x, y, width, height, retcode;
}

void wk_event_enqueue(struct wk_context *ctx, struct wk_event *in);
void wk_event_dequeue(struct wk_context *ctx, struct wk_event *out);
void wk_event_rsqueue(struct wk_context *ctx);

void wk_event_prepare(struct wk_display *disp);

/* wk_context_func return values */

#define WKR_FINISH      0
#define WKR_RECALL      1

/* wk_event types */

#define WKE_BEGIN       0 /* Called when context is first created */
#define WKE_END         1 /* Called when context is destroyed */
/* #define WKE_MOUSECLK 2 Called when mouse clicked, wip */

#endif
