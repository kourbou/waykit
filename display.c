#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <wayland-client.h>
#include "xdg-shell-unstable-v6.h"
#include "util.h"

#include "display.h"

/* Used to send our own events to poll() */
int pipefd[2];

static void _handle_signal(int signum)
{
    uint8_t ev;

    switch(signum) {
        case SIGINT:
            ev = KE_BRK;
            break;
    }

    write(pipefd[1], &ev, sizeof(uint8_t));
}

/* wl_output listener */
static void _handle_geometry(void *data, struct wl_output *wl_output,
        int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
        int32_t subpixel, const char *make, const char *model,
        int32_t transform)
{
    struct wk_display *disp = data;
    struct wk_monitor *new_mon = fzalloc(sizeof(struct wk_monitor));

    new_mon->x = x;
    new_mon->y = y;
    new_mon->phys_w = physical_width;
    new_mon->phys_h = physical_height;
    new_mon->subpixel = subpixel;
    new_mon->make = make;
    new_mon->model = model;
    new_mon->transform = transform;

    /* Push to the top of the monitor list */
    new_mon->next = disp->mon_head;
    disp->mon_head = new_mon;
}

static void _handle_mode(void *data, struct wl_output *wl_output, uint32_t flags,
        int32_t width, int32_t height, int32_t refresh)
{
    struct wk_display *disp = data;
    struct wk_mode *new_mode = fzalloc(sizeof(struct wk_mode));

    new_mode->width = width;
    new_mode->height = height;
    new_mode->refresh = refresh;

    if(flags & WL_OUTPUT_MODE_CURRENT)
        new_mode->current = true;
    if(flags & WL_OUTPUT_MODE_PREFERRED)
        new_mode->preferred = true;

    /* Push to the top of mode list */
    new_mode->next = disp->mode_head;
    disp->mode_head = new_mode;
}

static void _handle_done(void *data, struct wl_output *wl_output)
{
    struct wk_display *disp = data;
    disp->output_done = true;
}

static void _handle_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
    struct wk_display *disp = data;
    disp->scale_factor = factor;
}

struct wl_output_listener output_listener = {
    .geometry = _handle_geometry,
    .mode = _handle_mode,
    .done = _handle_done,
    .scale = _handle_scale
};
/* end wl_output listener */


/* wl_registry listerner */
static void _handle_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version)
{
    vlog("Global declared: %s v%d", interface, version);

    struct wk_display *disp = data;
    /* Cycle through the interfaces we need */
    if(strcmp(interface, wl_compositor_interface.name) == 0) {
        disp->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, min(3, version));
    } else if(strcmp(interface, wl_shm_interface.name) == 0) {
        disp->shm = wl_registry_bind(registry, name, &wl_shm_interface, min(1, version));
    } else if(strcmp(interface, wl_seat_interface.name) == 0) {
        disp->seat = wl_registry_bind(registry, name, &wl_seat_interface, min(4, version));
    } else if(strcmp(interface, wl_output_interface.name) == 0) {
        disp->output = wl_registry_bind(registry, name, &wl_output_interface, min(2, version));
    } else if(strcmp(interface, zxdg_shell_v6_interface.name) == 0) {
        disp->shell = wl_registry_bind(registry, name, &zxdg_shell_v6_interface, min(1, version));
    }

    /* Just add more interfaces here */
}

static void _handle_global_remove(void *data, struct wl_registry *registry,
        uint32_t name)
{
    vlog("Global removed: %d", name);
}

struct wl_registry_listener registry_listener = {
    .global = _handle_global,
    .global_remove = _handle_global_remove
};
/* end wl_registry listener */

/* zxdg_shell listener */
static void _handle_ping(void *data, struct zxdg_shell_v6 *zxdg_shell_v6,
        uint32_t serial)
{
    struct wk_display *disp = data;
    zxdg_shell_v6_pong(disp->shell, serial);
}

struct zxdg_shell_v6_listener shell_listener = {
    .ping = _handle_ping
};
/* end zxdg_shell listener */

struct wk_display *wk_display_connect()
{
    struct wk_display* disp = fzalloc(sizeof(struct wk_display));
    nlog("Connecting to display");

    /* Used for handling our own events in the loop */
    if(pipe(pipefd) < 0) {
        nlog(red("Failed to pipe()"));
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, _handle_signal);


    /* Connect to display. NULL is the default "wayland-0" */
    disp->display = failsafe(wl_display_connect(NULL));
    disp->registry = wl_display_get_registry(disp->display);

    /* Attach the listener and send the display as data */
    wl_registry_add_listener(disp->registry, &registry_listener, disp);
    wl_display_roundtrip(disp->display);

    /* Dirty trick to check that we have the ifaces */
    failsafe(disp->compositor);
    failsafe(disp->shm);
    failsafe(disp->shell);
    failsafe(disp->seat);
    failsafe(disp->output);

    wl_output_add_listener(disp->output, &output_listener, disp);
    zxdg_shell_v6_add_listener(disp->shell, &shell_listener, disp);

    return disp;
}

void wk_display_main(struct wk_display* disp)
{
    struct pollfd pfd[2];
    int ret, num;

    /* The display fd is first */
    pfd[0].fd = wl_display_get_fd(disp->display);
    pfd[0].events = POLLIN | POLLERR | POLLHUP;

    /* Our events go second */
    pfd[1].fd = pipefd[0];
    pfd[1].events = POLLIN;

    /* Entering main loop */
    while(1) {
        wl_display_dispatch_pending(disp->display);
        ret = wl_display_flush(disp->display);
        if(ret < 0 && errno != EAGAIN) {
            nlog(red("wl_display_flush error"));
            break;
        }

        num = poll(&pfd[0], 2, -1);
        if((num < 0) && (errno != EINTR)) {
            nlog(red("poll error"));
            break;
        }

        if(num > 0) {
            uint32_t wevent = pfd[0].revents;
            uint32_t kevent = pfd[1].revents;

            /* Handle our events after wayland's */
            if((kevent & POLLIN) && !(wevent & POLLIN)) {
                uint8_t event;
                read(pipefd[0], &event, sizeof(uint8_t));

                bool exit;

                switch(event) {
                    case KE_BRK:
                        exit = true;
                        break;
                }

                if(exit)
                    break;
            }

            /* Loop is closing and we have no new data */
            if(((wevent & POLLERR) || (wevent & POLLHUP)) && !(wevent & POLLIN))
                break;

            if(wevent & POLLIN) {
                ret = wl_display_dispatch(disp->display);
                if(ret == -1) {
                    nlog(red("wl_display_dispatch error"));
                    break;
                }
            }

            /* Render only once we've dealt with all events */
            if(!disp->window->buffer->busy) {
                wk_window_render(disp->window);
            }
        }
    }
    /* Exiting main loop */
}

void wk_display_disconnect(struct wk_display* disp)
{
    /* Free the wayland interfaces */
    wl_compositor_destroy(disp->compositor);
    wl_shm_destroy(disp->shm);
    zxdg_shell_v6_destroy(disp->shell);
    wl_seat_destroy(disp->seat);
    wl_output_destroy(disp->output);

    /* And disconnect from the server */
    wl_registry_destroy(disp->registry);
    nlog("Disconnecting from display");
    wl_display_disconnect(disp->display);

    struct wk_monitor* mon_head = disp->mon_head;
    while(mon_head != NULL) {
        struct wk_monitor* to_del = mon_head;
        mon_head = mon_head->next;
        free(to_del);
    }

    struct wk_mode* mode_head = disp->mode_head;
    while(mode_head != NULL) {
        struct wk_mode* to_del = mode_head;
        mode_head = mode_head->next;
        free(to_del);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    free(disp);
}

