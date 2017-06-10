#include <cairo/cairo.h>
#include <math.h>

#include "display.h"
#include "window.h"
#include "util.h"

#define BODY_WIDTH      175
#define WING_X          -25
#define WING_Y          44

#define NECK_HEIGHT     60
#define NECK_WIDTH      120

#define BEAK_HEIGHT     50
#define BEAK_WIDTH      43

#define EYE_OFF_X       NECK_WIDTH/2 - 20
#define EYE_OFF_Y       -5
#define EYE_HEIGHT      22
#define EYE_WIDTH       14

#define LEG_LENGTH      110
#define LEG_FRONT       50
#define LEG_BACK        25

void render(uint32_t width, uint32_t height,
        void *pixels, cairo_t *cr);

int main(int argc, char** argv)
{
    struct wk_display *disp = wk_display_connect();
    struct wk_window *win = wk_window_create(disp, 800, 600);

    wk_window_register_draw(win, &render);

    wk_display_main(disp);

    wk_window_destroy(win);
    wk_display_disconnect(disp);

    return 0;
}

void render(uint32_t width, uint32_t height,
        void *pixels, cairo_t *cr)
{
    int center_x = width / 2;
    int center_y = height / 2;

    /* White background*/
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);


    /* Legs */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_width(cr, EYE_WIDTH + 5);

    cairo_move_to(cr, center_x + LEG_BACK, center_y + LEG_LENGTH);
    cairo_line_to(cr, center_x + LEG_BACK, center_y + LEG_LENGTH*2);
    cairo_stroke(cr);

    cairo_move_to(cr, center_x - LEG_FRONT, center_y + LEG_LENGTH);
    cairo_line_to(cr, center_x - LEG_FRONT, center_y + LEG_LENGTH*2);
    cairo_stroke(cr);

    /* Body */
    cairo_set_source_rgb(cr, 1, 0, 1);
    cairo_arc(cr, center_x, center_y, BODY_WIDTH, 0, M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 1, 1, 0);
    cairo_rectangle(cr, center_x - BODY_WIDTH, center_y - NECK_HEIGHT, NECK_WIDTH, NECK_HEIGHT);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 1);
    cairo_arc_negative(cr, center_x - WING_X, center_y - WING_Y, BODY_WIDTH - 60, M_PI - M_PI/8, 2 * M_PI - M_PI/8);
    cairo_fill(cr);

    /* Head */
    cairo_set_source_rgb(cr, 0, 1, 1);
    cairo_arc(cr, center_x - BODY_WIDTH + NECK_WIDTH/2, center_y - NECK_HEIGHT, NECK_WIDTH/2, M_PI, 2 * M_PI);
    cairo_fill(cr);

    /* Beak */
    int beak_offset = NECK_WIDTH/2 - sqrt(pow(NECK_WIDTH/2, 2) - pow(BEAK_HEIGHT/2, 2)) + 1;
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_move_to(cr, center_x - BODY_WIDTH + beak_offset, center_y - NECK_HEIGHT + BEAK_HEIGHT/2);
    cairo_line_to(cr, center_x - BODY_WIDTH + beak_offset, center_y - NECK_HEIGHT - BEAK_HEIGHT/2);
    cairo_line_to(cr, center_x - BODY_WIDTH - BEAK_WIDTH, center_y - NECK_HEIGHT);
    cairo_close_path(cr);
    cairo_fill(cr);

    /* Eye */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, EYE_WIDTH);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, center_x - BODY_WIDTH + EYE_OFF_X, center_y - NECK_HEIGHT + EYE_OFF_Y);
    cairo_line_to(cr, center_x - BODY_WIDTH + EYE_OFF_X, center_y - NECK_HEIGHT + EYE_OFF_Y - EYE_HEIGHT);
    cairo_stroke(cr);
}
