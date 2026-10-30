/* See LICENSE file for copyright and license details. */
#include <X11/Xlib.h>

#include "logo.h"

void
draw_logo(Display *dpy, Window win, GC gc, int u, int start_x, int start_y)
{
	XRectangle rects[5];

	rects[0].x = start_x;
	rects[0].y = start_y;
	rects[0].width = LOGO_WIDTH * u;
	rects[0].height = 2 * u;

	rects[1].x = start_x;
	rects[1].y = start_y + 2 * u;
	rects[1].width = 2 * u;
	rects[1].height = (LOGO_HEIGHT - 2) * u;

	rects[2].x = start_x + 4 * u;
	rects[2].y = start_y + 2 * u;
	rects[2].width = 2 * u;
	rects[2].height = (LOGO_HEIGHT - 2) * u;

	rects[3].x = start_x + 8 * u;
	rects[3].y = start_y + 2 * u;
	rects[3].width = 2 * u;
	rects[3].height = (LOGO_HEIGHT - 2) * u;

	rects[4].x = start_x + 12 * u;
	rects[4].y = start_y + 2 * u;
	rects[4].width = 2 * u;
	rects[4].height = (LOGO_HEIGHT - 2) * u;

	XFillRectangles(dpy, win, gc, rects, 5);
}