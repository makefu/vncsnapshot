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
# dependancies

argsresources.o: argsresources.c vncsnapshot.h rfb.h rfbproto.h
buffer.o: buffer.c vncsnapshot.h rfb.h rfbproto.h
cursor.o: cursor.c vncsnapshot.h rfb.h rfbproto.h
listen.o: listen.c vncsnapshot.h rfb.h rfbproto.h
rfbproto.o: rfbproto.c vncsnapshot.h rfb.h rfbproto.h vncauth.h \
  protocols/rre.c protocols/corre.c \
  protocols/hextile.c protocols/zlib.c protocols/tight.c
sockets.o: sockets.c vncsnapshot.h rfb.h rfbproto.h
tunnel.o: tunnel.c vncsnapshot.h rfb.h rfbproto.h
vncsnapshot.o: vncsnapshot.c vncsnapshot.h rfb.h rfbproto.h
vncauth.o: vncauth.c stdhdrs.h rfb.h rfbproto.h vncauth.h d3des.h
