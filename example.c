#include <cairo/cairo.h>
#include <math.h>

#include "display.h"
#include "window.h"
#include "util.h"


#define BODY_WIDTH      175
#define NECK_HEIGHT     100
#define NECK_WIDTH      120

void render(uint32_t width, uint32_t height,
        void *pixels, cairo_t *cr)
{
    int center_x = width / 2;
    int center_y = height / 2;

    /* White background*/
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    /* Body */
    cairo_set_source_rgb(cr, 1, 0, 1);
    cairo_arc(cr, center_x, center_y, BODY_WIDTH, 0, M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 1, 1, 0);
    cairo_rectangle(cr, center_x - BODY_WIDTH, center_y - NECK_HEIGHT, NECK_WIDTH, NECK_HEIGHT);
    cairo_fill(cr);
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

