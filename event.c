#include "util.h"
#include "event.h"

/* Enqueue an event in the context */
void wk_event_enqueue(struct wk_context *ctx, struct wk_event *ev)
{
    if(ctx->queue_enq >= WK_MAX_EVENTS)
        failsafe(0); /* Too many events! */

    ctx->queue_enq++;
    ctx->queue[ctx->queue_enq] = ev;
}

/* Dequeue an event in the context */
void wk_event_dequeue(struct wk_context *ctx, struct wk_event *out)
{
    /* If we're out of events, just return */
    if(ctx->queue_deq >= ctx->queue_enq)
        return;

    ctx->queue_deq++;
    out = ctx->queue[ctx->queue_deq];
}

/* Reset the context's event queue */
void wk_event_rsqueue(struct wk_context *ctx)
{
    /* Set the top and bottom pointer back to 0 */
    ctx->queue_enq = 0;
    ctx->queue_deq = 0;
}

static void _handle_capabilities(void *data, struct wl_seat *wl_seat,
        uint32_t capabilities)
{

}

struct wl_seat_listener

void wk_event_prepare(struct wk_display *disp)
{
    failsafe(disp->seat);

}
