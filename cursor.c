/*
 *  Copyright (C) 2001 Const Kaplinsky.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * cursor.c - code to support cursor shape updates (XCursor, RichCursor).
 */
static char ID = "$Id$";

#include "vncsnapshot.h"


#define OPER_SAVE     0
#define OPER_RESTORE  1

/* Data kept for RichCursor encoding support. */
static Bool prevRichCursorSet = False;
static CARD8 *rcSource, *rcMask;
static int rcHotX, rcHotY, rcWidth, rcHeight;
static int rcCursorX = 0, rcCursorY = 0;
static int rcLockX, rcLockY, rcLockWidth, rcLockHeight;
static Bool rcCursorHidden, rcLockSet;

static Bool SoftCursorInLockedArea(void);
static void SoftCursorCopyArea(int oper);
static void SoftCursorDraw(void);
static void FreeCursors(Bool setDotCursor);

static char * rcSavedArea = NULL;


/*********************************************************************
 * HandleRichCursor(). RichCursor shape updates support. This
 * variation of cursor shape updates cannot be supported directly via
 * Xlib cursors so we have to emulate cursor operating on the frame
 * buffer (that is why we call it "software cursor").
 ********************************************************************/

Bool HandleRichCursor(int xhot, int yhot, int width, int height)
{
  size_t bytesPerRow, bytesMaskData;
  char *buf;
  CARD8 *ptr;
  int x, y, b;

  bytesPerRow = (width + 7) / 8;
  bytesMaskData = bytesPerRow * height;

  FreeCursors(True);

  if (width * height == 0)
    return True;

  /* Read cursor pixel data. */

  rcSource = malloc(width * height * (myFormat.bitsPerPixel / 8));
  if (rcSource == NULL)
    return False;

  if (!ReadFromRFBServer((char *)rcSource,
			 width * height * (myFormat.bitsPerPixel / 8))) {
    free(rcSource);
    return False;
  }

  /* Read and decode mask data. */

  buf = malloc(bytesMaskData);
  if (buf == NULL) {
    free(rcSource);
    return False;
  }

  if (!ReadFromRFBServer(buf, bytesMaskData)) {
    free(rcSource);
    free(buf);
    return False;
  }

  rcMask = malloc(width * height);
  if (rcMask == NULL) {
    free(rcSource);
    free(buf);
    return False;
  }

  ptr = rcMask;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width / 8; x++) {
      for (b = 7; b >= 0; b--) {
	*ptr++ = buf[y * bytesPerRow + x] >> b & 1;
      }
    }
    for (b = 7; b > 7 - width % 8; b--) {
      *ptr++ = buf[y * bytesPerRow + x] >> b & 1;
    }
  }

  free(buf);

  /* Set remaining data associated with cursor. */

  rcHotX = xhot;
  rcHotY = yhot;
  rcWidth = width;
  rcHeight = height;

  SoftCursorCopyArea(OPER_SAVE);
  SoftCursorDraw();

  rcCursorHidden = False;
  rcLockSet = False;

  prevRichCursorSet = True;
  return True;
}


/*********************************************************************
 * SoftCursorLockArea(). This function should be used to prevent
 * collisions between simultaneous framebuffer update operations and
 * cursor drawing operations caused by movements of pointing device.
 * The parameters denote a rectangle where mouse cursor should not be
 * drawn. Every next call to this function expands locked area so
 * previous locks remain active.
 ********************************************************************/

void SoftCursorLockArea(int x, int y, int w, int h)
{
  int newX, newY;

  if (!prevRichCursorSet)
    return;

  if (!rcLockSet) {
    rcLockX = x;
    rcLockY = y;
    rcLockWidth = w;
    rcLockHeight = h;
    rcLockSet = True;
  } else {
    newX = (x < rcLockX) ? x : rcLockX;
    newY = (y < rcLockY) ? y : rcLockY;
    rcLockWidth = (x + w > rcLockX + rcLockWidth) ?
      (x + w - newX) : (rcLockX + rcLockWidth - newX);
    rcLockHeight = (y + h > rcLockY + rcLockHeight) ?
      (y + h - newY) : (rcLockY + rcLockHeight - newY);
    rcLockX = newX;
    rcLockY = newY;
  }

  if (!rcCursorHidden && SoftCursorInLockedArea()) {
    SoftCursorCopyArea(OPER_RESTORE);
    rcCursorHidden = True;
  }
}

/*********************************************************************
 * SoftCursorUnlockScreen(). This function discards all locks
 * performed since previous SoftCursorUnlockScreen() call.
 ********************************************************************/

void SoftCursorUnlockScreen(void)
{
  if (!prevRichCursorSet)
    return;

  if (rcCursorHidden) {
    SoftCursorCopyArea(OPER_SAVE);
    SoftCursorDraw();
    rcCursorHidden = False;
  }
  rcLockSet = False;
}
/*********************************************************************
 * Internal (static) low-level functions.
 ********************************************************************/

static Bool SoftCursorInLockedArea(void)
{
  return (rcLockX < rcCursorX - rcHotX + rcWidth &&
	  rcLockY < rcCursorY - rcHotY + rcHeight &&
	  rcLockX + rcLockWidth > rcCursorX - rcHotX &&
	  rcLockY + rcLockHeight > rcCursorY - rcHotY);
}

static void SoftCursorCopyArea(int oper)
{
  int x, y, w, h;

  x = rcCursorX - rcHotX;
  y = rcCursorY - rcHotY;
  if (x >= si.framebufferWidth || y >= si.framebufferHeight)
    return;

  w = rcWidth;
  h = rcHeight;
  if (x < 0) {
    w += x;
    x = 0;
  } else if (x + w > si.framebufferWidth) {
    w = si.framebufferWidth - x;
  }
  if (y < 0) {
    h += y;
    y = 0;
  } else if (y + h > si.framebufferHeight) {
    h = si.framebufferHeight - y;
  }

  if (oper == OPER_SAVE) {
    if (rcSavedArea != NULL) {
        free(rcSavedArea);
        rcSavedArea = NULL;
    }
    rcSavedArea = CopyScreenToData(x, y, w, h);
  } else {
    CopyDataToScreen(rcSavedArea, x, y, w, h);
  }
}

static void SoftCursorDraw(void)
{
  int x, y, x0, y0;
  int offset, bytesPerPixel;
  char *pos;

  bytesPerPixel = myFormat.bitsPerPixel / 8;

  /* FIXME: Speed optimization is possible. */
  for (y = 0; y < rcHeight; y++) {
    y0 = rcCursorY - rcHotY + y;
    if (y0 >= 0 && y0 < si.framebufferHeight) {
      for (x = 0; x < rcWidth; x++) {
	x0 = rcCursorX - rcHotX + x;
	if (x0 >= 0 && x0 < si.framebufferWidth) {
	  offset = y * rcWidth + x;
	  if (rcMask[offset]) {
	    pos = (char *)&rcSource[offset * bytesPerPixel];
	    CopyDataToScreen(pos, x0, y0, 1, 1);
	  }
	}
      }
    }
  }
}

static void FreeCursors(Bool setDotCursor)
{
  if (prevRichCursorSet) {
    SoftCursorCopyArea(OPER_RESTORE);
    free(rcSource);
    free(rcMask);
    prevRichCursorSet = False;
  }

  if (rcSavedArea != NULL) {
    free(rcSavedArea);
    rcSavedArea = NULL;
  }
}

