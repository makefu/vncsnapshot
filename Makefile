# Makefile for vncsnapshot, Unix/Linux platforms.
#
# $Id$

#
# INSTALLER: Adjust these lines to point to your installation's location of
# Zlib and JPEG lib.
ZLIB_INCLUDE = -I/usr/local/include
ZLIB_LIB = -L/usr/local/lib -lz
JPEG_INCLUDE = -I/usr/local/include
JPEG_LIB = -L/usr/local/lib -ljpeg


# Other libraries:
# SOLARIS:
# EXTRALIBS = -lsocket -lnsl
# EXTRAINCLUDES =
# Linux:
EXTRALIBS =
EXTRAINCLUDES =

# Compilation Flags. Season to taste.
CC = gcc
CDEBUGFLAGS = -O2

# You shouldn't need to change anything below.
INCLUDES = $(ZLIB_INCLUDE) $(JPEG_INCLUDE) $(EXTRAINCLUDES)
CFLAGS =  $(CDEBUGFLAGS) $(INCLUDES)


SRCS = \
  argsresources.c \
  buffer.c \
  cursor.c \
  listen.c \
  rfbproto.c \
  sockets.c \
  tunnel.c \
  vncsnapshot.c \
  d3des.c vncauth.c

OBJS = $(SRCS:.c=.o)

all: vncsnapshot

vncsnapshot: $(OBJS)
	$(LINK.c) $(CDEBUGFLAGS) -o $@ $(OBJS) $(ZLIB_LIB) $(JPEG_LIB) $(EXTRALIBS)


clean:
	-rm -f $(OBJS) vncsnapshot core

reallyclean: clean
	-rm -f *~
# dependancies - manual
