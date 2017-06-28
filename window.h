#ifndef WK_WINDOW_H
#define WK_WINDOW_H

#include <stdbool.h>
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "xdg-shell-unstable-v6.h"
#include "display.h"
#include "event.h"

/* A buffer struct that is used internally */
struct wk_window_buffer {
    uint32_t width, height;
    bool busy;

    void *pixels;
    struct wl_buffer *wl_buffer;
};

/* Formats are found by the format event of wl_shm_listener */
struct wk_window_format {
    uint32_t value;

    /* Next format in list */
    struct wk_window_format *next;
};

/* Main window structure */
struct wk_window {
    /* Wayland objects */
    struct wl_surface *surface;
    struct zxdg_surface_v6 *zxdg_surface;
    struct zxdg_toplevel_v6 *zxdg_toplevel;

    /* Creating display */
    struct wk_display *disp;

    /* Window objects */
    uint32_t flags;
    int32_t width;
    int32_t height;
    char *title;
    struct wk_window_buffer *buffer;

    /* List of wk_contexts */
    struct wk_context *context_head;
    struct wk_context *context_tail;

    /* List of shm formats */
    struct wk_window_format *format_head;
};

/* Functions */
struct wk_window *wk_window_create(struct wk_display *disp, int width, int height);
void wk_window_register_draw(struct wk_window *win, void (*fun)(uint32_t width,
            uint32_t height, void *pixels, cairo_t *cairo));
void wk_window_render(struct wk_window *win);
void wk_window_destroy(struct wk_window *window);
#endif
