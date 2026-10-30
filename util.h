/* See LICENSE file for copyright and license details. */

#include <stddef.h>

void die(const char *fmt, ...);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);