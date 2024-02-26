VERSION = 1.0

# customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I${X11INC}
LIBS = -L${X11LIB} -lX11

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\"

# debug
CFLAGS = -O0 -g -std=c99 -Wall -pedantic -D_DEFAULT_SOURCE -D_BSD_SOURCE ${INCS} ${CPPFLAGS}
LDFLAGS = ${LIBS}

# optimized
#CFLAGS = -O2 -std=c99 -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE -D_BSD_SOURCE ${INCS} ${CPPFLAGS}
#LDFLAGS = -s ${LIBS}

# compiler and linker
CC = cc
