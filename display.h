#ifndef K_DISPLAY_H
#define K_DISPLAY_H

#include <stdint.h>
#include <wayland-client.h>
#include <stdbool.h>

#include "window.h"

/* Events sent k_display_emit() */
#define KE_BRK  0

/* Modes are found by the mode event of wl_ouput_listener */
struct k_mode {
    /* See wl_output_listener structure in wayland-client-protocol.h */
    bool preferred;
    bool current;
    int32_t width;
    int32_t height;
    int32_t refresh;

    /* Points to next k_mode in list */
    struct k_mode *next;
};

/* Monitors are found by the geometry event of wl_ouput_listener */
struct k_monitor {
    /* See wl_output_listener structure in wayland-client-protocol.h */
    int32_t x;
    int32_t y;
    int32_t phys_w;
    int32_t phys_h;
    int32_t subpixel;
    const char* make;
    const char* model;
    int32_t transform;

    /* Points to next k_monitor in the list */
    struct k_monitor *next;
};

/* Main structure */
struct k_display {
    /* Wayland objects */
    struct wl_display* display;
    struct wl_registry* registry;

    /* Wayland interfaces (see on_reg_global) */
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_seat* seat;
    struct wl_output* output;

    /* wl_output lists */
    struct k_monitor *mon_head;
    struct k_mode *mode_head;
    int32_t scale_factor;
    bool output_done;

    /* xdg_shell */
    struct zxdg_shell_v6 *shell;

    /* The display's window */
    // TODO: Enable support for multiple windows (i.e. create list here)
    struct k_window *window;
};

/* Functions */
struct k_display *k_display_connect();
void k_display_main(struct k_display *disp);
void k_display_disconnect(struct k_display *disp);

#endif /* K_DISPLAY_H */
