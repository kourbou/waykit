#include <cairo/cairo.h>

#include "display.h"
#include "window.h"
#include "util.h"

void render(uint32_t width, uint32_t height,
        void *pixels, cairo_t *cr)
{
    cairo_set_source_rgb(cr, 0, 0.7, 0.7);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle (cr, 100, 100, 200, 200);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
}

int main(int argc, char** argv)
{
    struct k_display *disp = k_display_connect();
    struct k_window *win = k_window_create(disp, 400, 400);

    k_window_register_draw(win, &render);

    k_display_main(disp);

    k_window_destroy(win);
    k_display_disconnect(disp);

    return 0;
}

