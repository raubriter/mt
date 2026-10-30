# See LICENSE file for copyright and license details.

VERSION = 1.0

PREFIX = /usr/local

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

INCS = -I${X11INC}
LIBS = -L${X11LIB} -lX11

CPPFLAGS = -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = ${LIBS}

CC = cc