#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "xdg-shell-unstable-v6.h"
#include "util.h"

#include "window.h"

/* Declared before the shell_surface listener */
static struct k_window_buffer *_create_buffer(struct k_window *win, uint32_t width,
        uint32_t height);

/* wl_buffer listener */
static void _handle_release(void *data, struct wl_buffer *wl_buffer)
{
   struct k_window_buffer *buf = (struct k_window_buffer*)data;
   buf->busy = false;
}

struct wl_buffer_listener buffer_listener = {
    .release = _handle_release
};
/* end wl_buffer listener */

/* wl_shm listener */
static void _handle_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    struct k_window *win = (struct k_window*)data;
    struct k_window_format *fmt = fzalloc(sizeof(struct k_window_format));

    fmt->value = format;
    fmt->next = win->format_head;
    win->format_head = fmt;
}

struct wl_shm_listener shm_listener = {
    .format = _handle_format
};
/* end wl_shm listener */

/* zxdg_surface listener */
static void _handle_surf_configure(void *data, struct zxdg_surface_v6 *zxdg_surface,
        uint32_t serial)
{
    vlog("surf configure: %d", serial);

    zxdg_surface_v6_ack_configure(zxdg_surface, serial);
}

struct zxdg_surface_v6_listener zxdg_surface_listener = {
    .configure = _handle_surf_configure
};
/* end zxdg_surface listener */

/* zxdg_toplevel listener */
void _handle_configure(void *data, struct zxdg_toplevel_v6 *zxdg_toplevel_v6,
        int32_t width, int32_t height, struct wl_array *states)
{
    vlog("toplvl configure: %d %d", width, height);
    if(width == 0 || height == 0)
        return;

    struct k_window *win = (struct k_window*)data;
    win->width = width;
    win->height = height;

    if((win->width != win->buffer->width) || (win->height != win->buffer->height))
        win->buffer = _create_buffer(win, width, height);
}

void _handle_close(void *data, struct zxdg_toplevel_v6 *zxdg_toplevel_v6)
{
    nlog("toplevel requested close!");
}

struct zxdg_toplevel_v6_listener zxdg_toplevel_listener = {
    .configure = _handle_configure,
    .close = _handle_close
};
/* zxdg_toplevel listener */

/* Create pool file for our buffer */
static int _create_pool_file(size_t size, char **name)
{
    static const char template[] = "waykit-XXXXXX";

    const char *path = getenv("XDG_RUNTIME_DIR");
    if (!path)
        return -1;

    int ts = (path[strlen(path) - 1] == '/');

    *name = malloc(
            strlen(template) +
            strlen(path) +
            (ts ? 0 : 1) + 1);
    sprintf(*name, "%s%s%s", path, ts ? "" : "/", template);

    int fd = mkstemp(*name);
    if (fd < 0)
        return -1;

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static struct k_window_buffer *_create_buffer(struct k_window *win, uint32_t width,
        uint32_t height)
{
    char *name;
    uint32_t stride = width * 4;
    uint32_t size = stride * height;

    struct k_window_buffer* buf = fzalloc(sizeof(struct k_window_buffer));
    int fd = _create_pool_file(size, &name);

    // TODO: Check that we have the WL_SHM_FORMAT_ARGB8888 available
    buf->pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct wl_shm_pool *pool = wl_shm_create_pool(win->disp->shm, fd, size);
    buf->wl_buffer = wl_shm_pool_create_buffer(pool, 0,
                    width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    unlink(name);
    free(name);

    buf->busy = false;
    buf->width = width;
    buf->height = height;
    buf->cairo_surface = cairo_image_surface_create_for_data(buf->pixels,
                    CAIRO_FORMAT_ARGB32, width, height, stride);
    buf->cairo = cairo_create(buf->cairo_surface);

    wl_buffer_add_listener(buf->wl_buffer, &buffer_listener, buf);
    nlog("created buffer");
    return buf;
}

struct k_window *k_window_create(struct k_display *disp, int width, int height)
{
    failsafe(disp);

    struct k_window *win = fzalloc(sizeof(struct k_window));
    win->disp = disp;
    win->surface = wl_compositor_create_surface(disp->compositor);
    win->zxdg_surface = zxdg_shell_v6_get_xdg_surface(disp->shell, win->surface);
    win->zxdg_toplevel = zxdg_surface_v6_get_toplevel(win->zxdg_surface);
    win->buffer = _create_buffer(win, K_WINDOW_DEFAULT_WIDTH, K_WINDOW_DEFAULT_HEIGHT);

    wl_shm_add_listener(disp->shm, &shm_listener, win);

    zxdg_surface_v6_add_listener(win->zxdg_surface, &zxdg_surface_listener, win);
    zxdg_toplevel_v6_add_listener(win->zxdg_toplevel, &zxdg_toplevel_listener, win);
    disp->window = win;

    win->width = width;
    win->height = height;
    // TODO: Should we be setting the default x, y coords?
    zxdg_surface_v6_set_window_geometry(win->zxdg_surface, 20, 20, width, height);
    wl_display_roundtrip(win->disp->display);

    nlog("window created");
    return win;
}

void k_window_register_draw(struct k_window *win, void (*fun)(uint32_t width,
            uint32_t height, void *pixels, cairo_t *cairo))
{
    struct k_draw_node *new = fzalloc(sizeof(struct k_draw_node));
    new->function = fun;

    new->next = win->draw_cb_head;
    win->draw_cb_head = new;
}

void k_window_render(struct k_window *win)
{
    for(struct k_draw_node* node = win->draw_cb_head; node != NULL; node = node->next) {
        (*node->function)(win->buffer->width, win->buffer->height,
                win->buffer->pixels, win->buffer->cairo);
    }

    wl_surface_attach(win->surface, win->buffer->wl_buffer, 0, 0);

    // TODO: I am redrawing the whole screen :/ We need those frames bad.
    wl_surface_damage(win->surface, 0, 0, win->buffer->width, win->buffer->height);
    wl_surface_commit(win->surface);
}

void k_window_destroy(struct k_window *win)
{
    cairo_destroy(win->buffer->cairo);
    cairo_surface_destroy(win->buffer->cairo_surface);
    wl_buffer_destroy(win->buffer->wl_buffer);

    zxdg_surface_v6_destroy(win->zxdg_surface);
    wl_surface_destroy(win->surface);

    struct k_window_format *fmt_head = win->format_head;
    while(fmt_head != NULL) {
        struct k_window_format* to_del = fmt_head;
        fmt_head = fmt_head->next;
        free(to_del);
    }

    struct k_draw_node *dr_head = win->draw_cb_head;
    while(dr_head != NULL) {
        struct k_draw_node *to_del = dr_head;
        dr_head = dr_head->next;
        free(to_del);
    }

    free(win->buffer);
    free(win);
    return;
}
