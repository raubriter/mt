/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "config.h"
#include "logo.h"
#include "util.h"

#define MAX_Q           1000
#define INIT_BUF_SIZE   (1 << 20)
#define LOGO_SIZE       32
#define LOGO_SMALL      6
#define IMG_CACHE_SIZE  4
#define PAN_STEP        50
#define MAX_ZOOM        20
#define TIMER_SECS      (TIMER_HOURS * 3600)

enum { ST_START, ST_RUN, ST_WAIT };

static Display *dpy;
static Window win;
static GC gc;
static XFontStruct *font;
static int sw, sh;
static int running = 1;
static int state = ST_START;
static int cur_pool_idx;
static int pool_size;
static int pan_x, pan_y;
static int zoom_level;
static time_t start_time;

struct img_entry {
	char *path;
	int w, h;
	uint32_t *pixels;
	unsigned long stamp;
};

static struct img_entry img_cache[IMG_CACHE_SIZE];
static unsigned long img_cache_clock;

static XImage *screen_img;

static char *q_files[MAX_Q];
static char q_answers[MAX_Q];
static int active_pool[MAX_Q];

static int
parse_answer(const char *name, char *answer_out)
{
	size_t n;
	char c;

	n = strlen(name);
	if (n < 8 || strcmp(name + n - 7, ".ff.bz2") != 0)
		return 0;

	c = name[n - 8];
	if (c >= 'a' && c <= 'e')
		c = c - 'a' + 'A';
	else if (c < 'A' || c > 'E')
		return 0;

	*answer_out = c;
	return 1;
}

static int
load_questions(void)
{
	DIR *dir;
	struct dirent *ent;
	struct stat st;
	int total;
	char ans;
	char fpath[PATH_MAX];

	dir = opendir(DATA_DIR);
	if (!dir)
		return 0;

	total = 0;
	while ((ent = readdir(dir)) != NULL && total < MAX_Q) {
		if (ent->d_name[0] == '.')
			continue;
		if (!parse_answer(ent->d_name, &ans))
			continue;

		snprintf(fpath, sizeof(fpath), "%s/%s", DATA_DIR, ent->d_name);
		if (stat(fpath, &st) < 0 || !S_ISREG(st.st_mode))
			continue;

		q_files[total]   = xstrdup(fpath);
		q_answers[total] = ans;
		total++;
	}
	closedir(dir);

	return total;
}

static void
reset_game(int num_all)
{
	int i, j, tmp;
	int *indices;

	pool_size = (PACK_SIZE > 0 && PACK_SIZE < num_all) ? PACK_SIZE : num_all;
	indices = xmalloc(num_all * sizeof(int));
	for (i = 0; i < num_all; i++)
		indices[i] = i;

	for (i = num_all - 1; i > 0; i--) {
		j = rand() % (i + 1);
		tmp = indices[i];
		indices[i] = indices[j];
		indices[j] = tmp;
	}

	for (i = 0; i < pool_size; i++)
		active_pool[i] = indices[i];

	free(indices);

	cur_pool_idx = 0;
	pan_x = 0;
	pan_y = 0;
}

static unsigned char *
load_bz2(const char *path, size_t *out_size)
{
	int pipefd[2], status;
	pid_t pid;
	size_t cap, size;
	unsigned char *buf;
	ssize_t n;

	cap = INIT_BUF_SIZE;
	size = 0;
	buf = xmalloc(cap);

	if (pipe(pipefd) < 0)
		die("pipe:");

	pid = fork();
	if (pid < 0)
		die("fork:");

	if (pid == 0) {
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		execlp("bzip2", "bzip2", "-dc", path, (char *)NULL);
		_exit(1);
	}

	close(pipefd[1]);

	while ((n = read(pipefd[0], buf + size, cap - size)) > 0) {
		size += n;
		if (size == cap) {
			cap <<= 1;
			buf = xrealloc(buf, cap);
		}
	}

	close(pipefd[0]);
	if (waitpid(pid, &status, 0) < 0)
		die("waitpid:");
	if (n < 0)
		die("read:");
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		free(buf);
		return NULL;
	}

	*out_size = size;
	return buf;
}

static struct img_entry *
get_image(const char *path)
{
	struct img_entry *e;
	unsigned char *map;
	size_t map_size;
	int w, h, x, y, i, victim;
	unsigned long oldest;
	uint32_t *pixels;
	uint16_t *raw;

	for (i = 0; i < IMG_CACHE_SIZE; i++) {
		if (img_cache[i].path && strcmp(img_cache[i].path, path) == 0) {
			img_cache[i].stamp = ++img_cache_clock;
			return &img_cache[i];
		}
	}

	map = load_bz2(path, &map_size);
	if (!map)
		return NULL;

	if (map_size < 16 || memcmp(map, "farbfeld", 8) != 0) {
		free(map);
		return NULL;
	}

	w = (map[8]  << 24) | (map[9]  << 16) | (map[10] << 8) | map[11];
	h = (map[12] << 24) | (map[13] << 16) | (map[14] << 8) | map[15];

	if (w <= 0 || h <= 0 ||
	    (size_t)w * (size_t)h * 8 + 16 > map_size) {
		free(map);
		return NULL;
	}

	pixels = xmalloc((size_t)w * h * sizeof(uint32_t));
	raw = (uint16_t *)(map + 16);
	for (y = 0; y < h; y++) {
		uint16_t *sr = raw + (size_t)y * w * 4;
		uint32_t *dr = pixels + (size_t)y * w;
		for (x = 0; x < w; x++)
			dr[x] = ((uint32_t)(sr[x*4]   >> 8) << 16) |
			        ((uint32_t)(sr[x*4+1] >> 8) <<  8) |
			         (uint32_t)(sr[x*4+2] >> 8);
	}
	free(map);

	victim = -1;
	oldest = ~0UL;
	for (i = 0; i < IMG_CACHE_SIZE; i++) {
		if (!img_cache[i].path) { victim = i; break; }
		if (img_cache[i].stamp < oldest) {
			oldest = img_cache[i].stamp;
			victim = i;
		}
	}
	if (img_cache[victim].path) {
		free(img_cache[victim].path);
		free(img_cache[victim].pixels);
	}

	e = &img_cache[victim];
	e->path   = xstrdup(path);
	e->w      = w;
	e->h      = h;
	e->pixels = pixels;
	e->stamp  = ++img_cache_clock;
	return e;
}

static void
draw_image(const char *path)
{
	struct img_entry *e;
	double scale;
	int draw_w, draw_h, offset_x, offset_y, max_pan_x, max_pan_y;
	int clip_x0, clip_y0, clip_x1, clip_y1, vis_w, vis_h;
	int sx, sy, stride;
	unsigned int *dst;

	e = get_image(path);
	if (!e)
		return;

	scale  = ((double)sh / (double)e->h) * (1.0 + (zoom_level * 0.1));
	draw_h = (int)(e->h * scale);
	draw_w = (int)(e->w * ((double)draw_h / (double)e->h));

	max_pan_x = (draw_w > sw) ? (draw_w - sw) / 2 : 0;
	max_pan_y = (draw_h > sh) ? (draw_h - sh) / 2 : 0;

	if (pan_x >  max_pan_x) pan_x =  max_pan_x;
	if (pan_x < -max_pan_x) pan_x = -max_pan_x;
	if (pan_y >  max_pan_y) pan_y =  max_pan_y;
	if (pan_y < -max_pan_y) pan_y = -max_pan_y;

	offset_x = (sw - draw_w) / 2 + pan_x;
	offset_y = (sh - draw_h) / 2 + pan_y;

	clip_x0 = offset_x < 0 ? 0 : offset_x;
	clip_y0 = offset_y < 0 ? 0 : offset_y;
	clip_x1 = offset_x + draw_w > sw ? sw : offset_x + draw_w;
	clip_y1 = offset_y + draw_h > sh ? sh : offset_y + draw_h;
	vis_w = clip_x1 - clip_x0;
	vis_h = clip_y1 - clip_y0;
	if (vis_w <= 0 || vis_h <= 0)
		return;

	if (!screen_img || screen_img->width != sw || screen_img->height != sh) {
		if (screen_img)
			XDestroyImage(screen_img);
		screen_img = XCreateImage(dpy,
		                          DefaultVisual(dpy, DefaultScreen(dpy)),
		                          DefaultDepth(dpy, DefaultScreen(dpy)),
		                          ZPixmap, 0, NULL, sw, sh, 32, 0);
		screen_img->data = xmalloc((size_t)screen_img->bytes_per_line * sh);
	}

	stride = screen_img->bytes_per_line / 4;
	dst = (unsigned int *)screen_img->data;

	for (sy = 0; sy < vis_h; sy++) {
		int iy = (clip_y0 + sy) - offset_y;
		int src_y = (int)((long long)iy * e->h / draw_h);
		uint32_t *sr;
		unsigned int *dr;

		if (src_y < 0)        src_y = 0;
		if (src_y >= e->h)    src_y = e->h - 1;
		sr = e->pixels + (size_t)src_y * e->w;
		dr = dst + (size_t)sy * stride;

		for (sx = 0; sx < vis_w; sx++) {
			int ix = (clip_x0 + sx) - offset_x;
			int src_x = (int)((long long)ix * e->w / draw_w);
			if (src_x < 0)     src_x = 0;
			if (src_x >= e->w) src_x = e->w - 1;
			dr[sx] = sr[src_x];
		}
	}

	XPutImage(dpy, win, gc, screen_img, 0, 0,
	          clip_x0, clip_y0, vis_w, vis_h);
}

static void
draw_timer(void)
{
	time_t now;
	int secs, text_w, rect_w, rect_x, y;
	char buf[16];

	now = time(NULL);
	secs = (int)(now - start_time);
	if (COUNT_UP) {
		if (secs > TIMER_SECS)
			secs = TIMER_SECS;
	} else {
		secs = TIMER_SECS - secs;
		if (secs < 0)
			secs = 0;
	}

	snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
	         secs / 3600, (secs % 3600) / 60, secs % 60);

	text_w = XTextWidth(font, buf, strlen(buf));
	rect_w = text_w + 12;
	rect_x = sw - rect_w - 20;
	y = 20;

	XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
	XFillRectangle(dpy, win, gc, rect_x, y, rect_w, 30);

	XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
	XDrawString(dpy, win, gc, rect_x + 6, y + 20, buf, strlen(buf));
}

static void
pick_next(void)
{
	int old_idx;

	pan_x = 0;
	pan_y = 0;

	if (pool_size > 1) {
		old_idx = cur_pool_idx;
		while (cur_pool_idx == old_idx)
			cur_pool_idx = rand() % pool_size;
	} else {
		cur_pool_idx = 0;
	}
}

static void
draw_screen(void)
{
	int q_real_idx;
	const char *msg;
	int len, tw;

	if (state == ST_START) {
		XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
		XFillRectangle(dpy, win, gc, 0, 0, sw, sh);
		XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
		draw_logo(dpy, win, gc, LOGO_SIZE,
		          (sw - (LOGO_WIDTH * LOGO_SIZE)) / 2,
		          (sh - (LOGO_HEIGHT * LOGO_SIZE)) / 2);
		return;
	}

	XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
	XFillRectangle(dpy, win, gc, 0, 0, sw, sh);

	XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
	draw_logo(dpy, win, gc, LOGO_SMALL, 20, 20);

	if (state == ST_WAIT) {
		msg = "AGUARDE";
		len = strlen(msg);
		tw = XTextWidth(font, msg, len);
		XDrawString(dpy, win, gc, (sw - tw) / 2, sh / 2, msg, len);
		draw_timer();
		return;
	}

	q_real_idx = active_pool[cur_pool_idx];
	draw_image(q_files[q_real_idx]);
	draw_timer();
}

static void
evaluate(int ans_idx)
{
	int i, q_real_idx;

	q_real_idx = active_pool[cur_pool_idx];

	if (ans_idx == (q_answers[q_real_idx] - 'A')) {
		for (i = cur_pool_idx; i < pool_size - 1; i++)
			active_pool[i] = active_pool[i + 1];
		pool_size--;

		if (pool_size == 0) {
			state = ST_WAIT;
			draw_screen();
			return;
		}
		pick_next();
	} else {
		pick_next();
	}
	draw_screen();
}

static void
skip(void)
{
	pick_next();
	draw_screen();
}

int
main(void)
{
	int screen, total, idx;
	Atom wm_state, fullscreen;
	KeySym key;
	XEvent ev;
	fd_set fds;
	struct timeval tv;

	srand(time(NULL));

	total = load_questions();
	if (total == 0)
		die("no questions found in %s", DATA_DIR);

	dpy = XOpenDisplay(NULL);
	if (!dpy)
		die("cannot open display");

	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);

	win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, sw, sh, 0,
	                          BlackPixel(dpy, screen),
	                          BlackPixel(dpy, screen));

	XSelectInput(dpy, win, ExposureMask | KeyPressMask);
	XMapWindow(dpy, win);

	wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
	fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	XChangeProperty(dpy, win, wm_state, XA_ATOM, 32, PropModeReplace,
	                (unsigned char *)&fullscreen, 1);

	gc = XCreateGC(dpy, win, 0, NULL);

	font = XLoadQueryFont(dpy, "10x20");
	if (!font)
		font = XLoadQueryFont(dpy, "fixed");
	if (font)
		XSetFont(dpy, gc, font->fid);

	while (running) {
		while (XPending(dpy)) {
			XNextEvent(dpy, &ev);

			if (ev.type == Expose) {
				draw_screen();
			} else if (ev.type == KeyPress) {
				key = XLookupKeysym(&ev.xkey, 0);

				if (key == XK_Escape) {
					running = 0;
				} else if (key == XK_Return) {
					if (state == ST_START) {
						state = ST_RUN;
						reset_game(total);
						start_time = time(NULL);
						draw_screen();
					}
				} else if (key == XK_space) {
					if (state == ST_RUN)
						skip();
				} else if (state == ST_RUN) {
					if (key == XK_Left) {
						pan_x += PAN_STEP;
						draw_screen();
					} else if (key == XK_Right) {
						pan_x -= PAN_STEP;
						draw_screen();
					} else if (key == XK_Up) {
						pan_y += PAN_STEP;
						draw_screen();
					} else if (key == XK_Down) {
						pan_y -= PAN_STEP;
						draw_screen();
					} else if (key == XK_plus || key == XK_equal) {
						if (zoom_level < MAX_ZOOM) {
							zoom_level++;
							draw_screen();
						}
					} else if (key == XK_minus) {
						if (zoom_level > 0) {
							zoom_level--;
							draw_screen();
						}
					} else {
						idx = -1;
						if (key == XK_a || key == XK_A)
							idx = 0;
						else if (key == XK_b || key == XK_B)
							idx = 1;
						else if (key == XK_c || key == XK_C)
							idx = 2;
						else if (key == XK_d || key == XK_D)
							idx = 3;
						else if (key == XK_e || key == XK_E)
							idx = 4;
						if (idx >= 0)
							evaluate(idx);
					}
				}
			}
		}

		if (state == ST_RUN || state == ST_WAIT) {
			if (time(NULL) - start_time >= TIMER_SECS) {
				state = ST_START;
				draw_screen();
			} else if (state == ST_WAIT &&
			           time(NULL) - start_time >= TIMER_WAIT_MINUTES * 60) {
				state = ST_START;
				draw_screen();
			}
		}

		FD_ZERO(&fds);
		FD_SET(ConnectionNumber(dpy), &fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if (select(ConnectionNumber(dpy) + 1, &fds, NULL, NULL, &tv) == 0) {
			if (state == ST_RUN || state == ST_WAIT)
				draw_timer();
		}
	}

	XCloseDisplay(dpy);
	return 0;
}