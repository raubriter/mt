/* See LICENSE file for copyright and license details. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

void *
xmalloc(size_t size)
{
	void *p;

	if (!(p = malloc(size)))
		die("malloc:");
	return p;
}

void *
xrealloc(void *ptr, size_t size)
{
	void *p;

	if (!(p = realloc(ptr, size)))
		die("realloc:");
	return p;
}

char *
xstrdup(const char *s)
{
	char *p;

	if (!(p = strdup(s)))
		die("strdup:");
	return p;
}