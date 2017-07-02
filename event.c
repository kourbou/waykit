#include "util.h"
#include "event.h"

struct wl_pointer *pointer;

/* Enqueue an event in the context */
void wk_event_enqueue(struct wk_context *ctx, struct wk_event *ev)
{
    if(ctx->queue_enq >= WK_MAX_EVENTS)
        failsafe(0); /* Too many events! */

    ctx->queue[ctx->queue_enq] = ev;
    ctx->queue_enq++;
}

/* Dequeue an event in the context */
void wk_event_dequeue(struct wk_context *ctx, struct wk_event *out)
{
    /* If we're out of events, just return */
    if(ctx->queue_deq >= ctx->queue_enq)
        return;

    out = ctx->queue[ctx->queue_deq];
    ctx->queue_deq++;
}

/* Reset the context's event queue */
void wk_event_rsqueue(struct wk_context *ctx)
{
    /* Set the top and bottom pointer back to 0 */
    ctx->queue_enq = 0;
    ctx->queue_deq = 0;
}

void _handle_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
        struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    // TODO: Check that there is a context there and send them the event
}

void _handle_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
        struct wl_surface *surface)
{

}

void _handle_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time,
        wl_fixed_t surface_x, wl_fixed_t surface_y)
{

}

void _handle_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
        uint32_t time, uint32_t button, uint32_t state)
{

}

void _handle_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
        uint32_t axis, wl_fixed_t value)
{

}

void _handle_frame(void *data, struct wl_pointer *wl_pointer)
{

}

struct wl_pointer_listener pointer_listener {
    .enter = _handle_enter,
    .leave = _handle_leave,
    .motion = _handle_motion,
    .button = _handle_button,
    .axis = _handle_axis,
    .frame = _handle_frame,
};

static void _handle_capabilities(void *data, struct wl_seat *wl_seat,
        uint32_t caps)
{
    if((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer) {
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
        vlog("Pointer found!");
    } else if(!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer) {
        wl_pointer_destroy(pointer);
        pointer = NULL;
        vlog("Pointer destroyed?!");
    }
}

struct wl_seat_listener seat_listener {
    .capabilities = _handle_capabilites,
    .name = NULL
};

void wk_event_prepare(struct wk_display *disp)
{
    failsafe(disp->seat);
    wl_seat_add_listener(seat, &seat_listener, NULL);
}
