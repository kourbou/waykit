#ifndef K_WINDOW_H
#define K_WINDOW_H

#include <stdbool.h>
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "xdg-shell-unstable-v6.h"
#include "display.h"

/* The callback for drawing */
// TODO: Use frame.c to create frames and have callbacks for all of them
struct k_draw_node {
    void (*function)(uint32_t width, uint32_t height,
        void *pixels, cairo_t *cairo);
    struct k_draw_node *next;
};

/* A buffer struct that is used internally */
struct k_window_buffer {
    uint32_t width, height;
    bool busy;

    void *pixels;
    struct wl_buffer *wl_buffer;

    cairo_surface_t *cairo_surface;
    cairo_t *cairo;
};

/* Formats are found by the format event of wl_shm_listener */
struct k_window_format {
    uint32_t value;

    /* Next format in list */
    struct k_window_format *next;
};

/* Main window structure */
struct k_window {
    /* Wayland objects */
    struct wl_surface *surface;
    struct zxdg_surface_v6 *zxdg_surface;
    struct zxdg_toplevel_v6 *zxdg_toplevel;

    /* Creating display */
    struct k_display *disp;

    /* Window objects */
    uint32_t flags;
    int32_t width;
    int32_t height;
    char *title;
    struct k_window_buffer *buffer;

    /* List of k_draw callbacks */
    struct k_draw_node *draw_cb_head;

    /* List of shm formats */
    struct k_window_format *format_head;
};

/* Functions */
struct k_window *k_window_create(struct k_display *display, int width, int height);
void k_window_register_draw(struct k_window *win, void (*fun)(uint32_t width,
            uint32_t height, void *pixels, cairo_t *cairo));
void k_window_render(struct k_window *win);
void k_window_destroy(struct k_window *window);
#endif
