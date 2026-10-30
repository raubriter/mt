/* See LICENSE file for copyright and license details. */

#ifndef LOGO_H
#define LOGO_H

#include <X11/Xlib.h>

#define LOGO_WIDTH  16
#define LOGO_HEIGHT 7

void draw_logo(Display *, Window, GC, int, int, int);

#endif