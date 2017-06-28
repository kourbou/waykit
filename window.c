#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "xdg-shell-unstable-v6.h"
#include "util.h"
#include "event.h"

#include "window.h"

/* Declared before the shell_surface listener */
static struct wk_window_buffer *_create_buffer(struct wk_window *win, uint32_t width,
        uint32_t height);

/* wl_buffer listener */
static void _handle_release(void *data, struct wl_buffer *wl_buffer)
{
    struct wk_window_buffer *buf = data;
    buf->busy = false;
}

struct wl_buffer_listener buffer_listener = {
    .release = _handle_release
};
/* end wl_buffer listener */

/* wl_shm listener */
static void _handle_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    struct wk_window *win = data;
    struct wk_window_format *fmt = fzalloc(sizeof(struct wk_window_format));

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
    struct wk_window *win = data;

    if((win->width != win->buffer->width) || (win->height != win->buffer->height))
        win->buffer = _create_buffer(win, win->width, win->height);

    vlog("surf configure: %d %d %d", serial, win->width, win->height);
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

    struct wk_window *win = data;
    win->width = width;
    win->height = height;
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

static void _delete_buffer(struct wk_window_buffer *buffer)
{
    cairo_surface_destroy(buffer->cairo_surface);
    cairo_destroy(buffer->cairo);
    wl_buffer_destroy(buffer->wl_buffer);
    free(buffer);
}

static struct wk_window_buffer *_create_buffer(struct wk_window *win, uint32_t width,
        uint32_t height)
{
    char *name;
    uint32_t stride = width * 4;
    uint32_t size = stride * height;

    if(win->buffer)
       _delete_buffer(win->buffer);

    struct wk_window_buffer* buf = fzalloc(sizeof(struct wk_window_buffer));
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
    //buf->cairo_surface = cairo_image_surface_create_for_data(buf->pixels,
    //                CAIRO_FORMAT_ARGB32, width, height, stride);
    //buf->cairo = cairo_create(buf->cairo_surface);

    wl_buffer_add_listener(buf->wl_buffer, &buffer_listener, buf);

    win->buffer = buf;

    nlog("created buffer");
    return buf;
}

struct wk_window *wk_window_create(struct wk_display *disp, int width, int height)
{
    failsafe(disp);

    struct wk_window *win = fzalloc(sizeof(struct wk_window));
    win->disp = disp;
    win->surface = wl_compositor_create_surface(disp->compositor);
    win->zxdg_surface = zxdg_shell_v6_get_xdg_surface(disp->shell, win->surface);
    win->zxdg_toplevel = zxdg_surface_v6_get_toplevel(win->zxdg_surface);
    win->buffer = _create_buffer(win, width, height);

    wl_shm_add_listener(disp->shm, &shm_listener, win);

    zxdg_surface_v6_add_listener(win->zxdg_surface, &zxdg_surface_listener, win);
    zxdg_toplevel_v6_add_listener(win->zxdg_toplevel, &zxdg_toplevel_listener, win);
    disp->window = win;

    win->width = width;
    win->height = height;

    wl_surface_commit(win->surface);

    nlog("window created");
    return win;
}

void wk_window_register_draw(struct wk_window *win, void (*fun)(uint32_t width,
            uint32_t height, void *pixels, cairo_t *cairo))
{
    struct wk_draw_node *new = fzalloc(sizeof(struct wk_draw_node));
    new->function = fun;

    new->next = win->draw_cb_head;
    win->draw_cb_head = new;
}

void wk_window_render(struct wk_window *win)
{
    for(struct wk_draw_node* node = win->draw_cb_head; node != NULL; node = node->next) {
        (*node->function)(win->buffer->width, win->buffer->height,
                win->buffer->pixels, win->buffer->cairo);
    }

    wl_surface_attach(win->surface, win->buffer->wl_buffer, 0, 0);

    // TODO: I am redrawing the whole screen :/ We need those frames bad.
    wl_surface_damage(win->surface, 0, 0, win->buffer->width, win->buffer->height);
    wl_surface_commit(win->surface);
}

void wk_window_destroy(struct wk_window *win)
{
    _delete_buffer(win->buffer);

    zxdg_surface_v6_destroy(win->zxdg_surface);
    zxdg_toplevel_v6_destroy(win->zxdg_toplevel);
    wl_surface_destroy(win->surface);

    struct wk_window_format *fmt_head = win->format_head;
    while(fmt_head != NULL) {
        struct wk_window_format* to_del = fmt_head;
        fmt_head = fmt_head->next;
        free(to_del);
    }

    struct wk_context *ctx_head = win->context_head;
    while(ctx_head != NULL) {
        struct wk_context *to_del = ctx_head;

        cairo_destroy(to_del->cairo);
        cairo_surface_destroy(to_del->surface);

        ctx_head = ctx_head->next;
        free(to_del);
    }

    free(win);
    return;
}

struct wk_context* wk_window_context(struct wk_window *win, wk_context_func function,
        int layer, int x, int y, int width, int height)
{
    //TODO: Check that the window is big enough for the context?...

    struct wk_context *new = fzalloc(sizeof wk_context);

    new->win = win;

    new->layer = layer;
    new->x = x;
    new->y = y;
    new->width = width;
    new->height = height;

    if(!win->context_head) {
        win->context_head = new;
        win->context_tail = new;
    } else {
        new->prev = win->context_tail;
        win->context_tail->next = new;
        win->context_tail = new;
    }

    // Should create a surface with the right offset etc...
    new->surface = cairo_image_surface_create_for_data(
            win->buffer->pixels[((y * win->buffer->width) * 4) + (x * 4)],
            CAIRO_FORMAT_ARGB32,
            width,
            height,
            width * 4);
    new->cairo = cairo_create(new->surface);

    // Actually all contexts should have a cairo_surface and cairo (ctx) created with
    // their zone so that they don’t mess up other contexts

    // Look at libuv source, it's pretty code
    // https://github.com/libuv/libuv/blob/fd7ce57f2b14651482c227898f6999664db937de/src/unix/loop.c
}

void wk_window_remove_context(struct wk_window *win, struct wk_context *remove)
{
    remove->prev = remove->next;
    cairo_destroy(remove->cairo);
    cairo_surface_destroy(remove->surface);
    free(remove);
}
