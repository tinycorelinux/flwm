// Rotated text drawing with X.

// Original code:
// Copyright (c) 1992 Alan Richardson (mppa3@uk.ac.sussex.syma) */
//
// Modifications for fltk:
// Copyright (c) 1997 Bill Spitzak (spitzak@d2.com)
// Modifications are to draw using the current fl_font.  All fonts
// used are cached in local structures.  This can get real expensive,
// use "

/* xvertext, Copyright (c) 1992 Alan Richardson (mppa3@uk.ac.sussex.syma)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  All work developed as a consequence of the use of
 * this program should duly acknowledge such use. No representations are
 * made about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 */

// if not defined then portions not used by flwm are included:
#define FLWM 1

/* ********************************************************************** */

#include <FL/x.H>
#if FL_MAJOR_VERSION < 2
# define XWindow Window
#endif
#include <FL/fl_draw.H>
#include "Rotated.H"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct BitmapStruct {
  int			 bit_w;
  int			 bit_h;
  Pixmap bm;
};

struct XRotCharStruct {
  int			 ascent;
  int			 descent;
  int			 lbearing;
  int			 rbearing;
  int			 width;
  BitmapStruct	 glyph;
};

struct XRotFontStruct {
  int			 dir;
  int			 height;
  int			 max_ascent;
  int			 max_descent;
  int			 max_char;
  int			 min_char;
  XFontStruct*		 xfontstruct;
  XRotCharStruct	 per_char[256];
};

/* *** Load the rotated version of a given font *** */

static XRotFontStruct*
XRotLoadFont(Display *dpy, XFontStruct* fontstruct, int dir)
{
  char val;
  XImage *I1, *I2;
  Pixmap canvas;
  XWindow root;
  int screen;
  GC font_gc;
  char text[3];/*, errstr[300];*/

  XRotFontStruct *rotfont;
  int ichar, i, j, index, boxlen = 60;
  int vert_w, vert_h, vert_len, bit_w, bit_h, bit_len;
  int min_char, max_char;
  unsigned char *vertdata, *bitdata;
  int ascent, descent, lbearing, rbearing;
  int on = 1, off = 0;

  /* useful macros ... */
  screen = DefaultScreen(dpy);
  root = DefaultRootWindow(dpy);

  /* create the depth 1 canvas bitmap ... */
  canvas = XCreatePixmap(dpy, root, boxlen, boxlen, 1);

  /* create a GC ... */
  font_gc = XCreateGC(dpy, canvas, 0, 0);
  XSetBackground(dpy, font_gc, off);

  XSetFont(dpy, font_gc, fontstruct->fid);

  /* allocate space for rotated font ... */
  rotfont = (XRotFontStruct *)malloc((unsigned)sizeof(XRotFontStruct));

  /* determine which characters are defined in font ... */
  min_char = fontstruct->min_char_or_byte2;
  if (min_char<0)  min_char = 0;
  rotfont->min_char = min_char;
  max_char = fontstruct->max_char_or_byte2;
  if (max_char>255) max_char = 255;
  rotfont->max_char = max_char;

  /* some overall font data ... */
  rotfont->dir = dir;
  rotfont->max_ascent = fontstruct->max_bounds.ascent;
  rotfont->max_descent = fontstruct->max_bounds.descent;
  rotfont->height = rotfont->max_ascent+rotfont->max_descent;

  rotfont->xfontstruct = fontstruct;
  /* remember xfontstruct for `normal' text ... */
  if (dir != 0) {
    /* font needs rotation ... */
    /* loop through each character ... */
    for (ichar = min_char; ichar <= max_char; ichar++) {

      index = ichar-fontstruct->min_char_or_byte2;

      XCharStruct* charstruct;
      if (fontstruct->per_char)
	charstruct = fontstruct->per_char+index;
      else
	charstruct = &fontstruct->min_bounds;

      /* per char dimensions ... */
      ascent =   rotfont->per_char[ichar].ascent   = charstruct->ascent;
      descent =  rotfont->per_char[ichar].descent  = charstruct->descent;
      lbearing = rotfont->per_char[ichar].lbearing = charstruct->lbearing;
      rbearing = rotfont->per_char[ichar].rbearing = charstruct->rbearing;
                 rotfont->per_char[ichar].width    = charstruct->width;

      /* some space chars have zero body, but a bitmap can't have ... */
      if (!ascent && !descent)
	ascent =   rotfont->per_char[ichar].ascent =   1;
      if (!lbearing && !rbearing)
	rbearing = rotfont->per_char[ichar].rbearing = 1;

      /* glyph width and height when vertical ... */
      vert_w = rbearing-lbearing;
      vert_h = ascent+descent;

      /* width in bytes ... */
      vert_len = (vert_w-1)/8+1;

      XSetForeground(dpy, font_gc, off);
      XFillRectangle(dpy, canvas, font_gc, 0, 0, boxlen, boxlen);

      /* draw the character centre top right on canvas ... */
      sprintf(text, "%c", ichar);
      XSetForeground(dpy, font_gc, on);
      XDrawImageString(dpy, canvas, font_gc, boxlen/2 - lbearing,
		       boxlen/2 - descent, text, 1);

      /* reserve memory for first XImage ... */
      vertdata = (unsigned char *) malloc((unsigned)(vert_len*vert_h));

      /* create the XImage ... */
      I1 = XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap,
			0, (char *)vertdata, vert_w, vert_h, 8, 0);

//      if (I1 == NULL) ... do something here

      I1->byte_order = I1->bitmap_bit_order = MSBFirst;

      /* extract character from canvas ... */
      XGetSubImage(dpy, canvas, boxlen/2, boxlen/2-vert_h,
		   vert_w, vert_h, 1, XYPixmap, I1, 0, 0);
      I1->format = XYBitmap;

      /* width, height of rotated character ... */
      if (dir == 2) {
	bit_w = vert_w;
	bit_h = vert_h;
      } else {
	bit_w = vert_h;
	bit_h = vert_w;
      }

      /* width in bytes ... */
      bit_len = (bit_w-1)/8 + 1;

      rotfont->per_char[ichar].glyph.bit_w = bit_w;
      rotfont->per_char[ichar].glyph.bit_h = bit_h;

      /* reserve memory for the rotated image ... */
      bitdata = (unsigned char *)calloc((unsigned)(bit_h*bit_len), 1);

      /* create the image ... */
      I2 = XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap, 0,
			(char *)bitdata, bit_w, bit_h, 8, 0);

//    if (I2 == NULL) ... error

      I2->byte_order = I2->bitmap_bit_order = MSBFirst;

      /* map vertical data to rotated character ... */
      for (j = 0; j < bit_h; j++) {
	for (i = 0; i < bit_w; i++) {
	  /* map bits ... */
	  if (dir == 1)
	    val = vertdata[i*vert_len + (vert_w-j-1)/8] &
	      (128>>((vert_w-j-1)%8));

	  else if (dir == 2)
	    val = vertdata[(vert_h-j-1)*vert_len + (vert_w-i-1)/8] &
	      (128>>((vert_w-i-1)%8));

	  else
	    val = vertdata[(vert_h-i-1)*vert_len + j/8] &
	      (128>>(j%8));

	  if (val)
	    bitdata[j*bit_len + i/8] = bitdata[j*bit_len + i/8] |
	      (128>>(i%8));
	}
      }

      /* create this character's bitmap ... */
      rotfont->per_char[ichar].glyph.bm =
	XCreatePixmap(dpy, root, bit_w, bit_h, 1);

      /* put the image into the bitmap ... */
      XPutImage(dpy, rotfont->per_char[ichar].glyph.bm,
		font_gc, I2, 0, 0, 0, 0, bit_w, bit_h);

      /* free the image and data ... */
      XDestroyImage(I1);
      XDestroyImage(I2);
      /*      free((char *)bitdata);  -- XDestroyImage does this
	      free((char *)vertdata);*/
    }

  }

  for (ichar = 0; ichar < min_char; ichar++)
    rotfont->per_char[ichar] = rotfont->per_char[int('?')];
  for (ichar = max_char+1; ichar < 256; ichar++)
    rotfont->per_char[ichar] = rotfont->per_char[int('?')];

  /* free pixmap and GC ... */
  XFreePixmap(dpy, canvas);
  XFreeGC(dpy, font_gc);

  return rotfont;
}

/* *** Free the resources associated with a rotated font *** */

static void XRotUnloadFont(Display *dpy, XRotFontStruct *rotfont)
{
  int ichar;

  if (rotfont->dir != 0) {
    /* loop through each character, freeing its pixmap ... */
    for (ichar = rotfont->min_char; ichar <= rotfont->max_char; ichar++)
      XFreePixmap(dpy, rotfont->per_char[ichar].glyph.bm);
  }
  /* rotfont should never be referenced again ... */
  free((char *)rotfont);
}

/* ---------------------------------------------------------------------- */

/* *** A front end to XRotPaintString : mimics XDrawString *** */

static void
XRotDrawString(Display *dpy, XRotFontStruct *rotfont, Drawable drawable,
	       GC gc, int x, int y, const char *str, int len)
{
  int i, xp, yp, dir, ichar;

  if (str == NULL || len<1) return;

  dir = rotfont->dir;

  /* a horizontal string is easy ... */
  if (dir == 0) {
    XSetFont(dpy, gc, rotfont->xfontstruct->fid);
    XDrawString(dpy, drawable, gc, x, y, str, len);
    return;
  }

  /* vertical or upside down ... */

  XSetFillStyle(dpy, gc, FillStippled);

  /* loop through each character in string ... */
  for (i = 0; i<len; i++) {
    ichar = ((unsigned char*)str)[i];

    /* suitable offset ... */
    if (dir == 1) {
      xp = x-rotfont->per_char[ichar].ascent;
      yp = y-rotfont->per_char[ichar].rbearing;
    }
    else if (dir == 2) {
      xp = x-rotfont->per_char[ichar].rbearing;
      yp = y-rotfont->per_char[ichar].descent+1;
    }
    else {
      xp = x-rotfont->per_char[ichar].descent+1;
      yp = y+rotfont->per_char[ichar].lbearing;
    }

    /* draw the glyph ... */
    XSetStipple(dpy, gc, rotfont->per_char[ichar].glyph.bm);

    XSetTSOrigin(dpy, gc, xp, yp);

    XFillRectangle(dpy, drawable, gc, xp, yp,
		   rotfont->per_char[ichar].glyph.bit_w,
		   rotfont->per_char[ichar].glyph.bit_h);

    /* advance position ... */
    if (dir == 1)
      y -= rotfont->per_char[ichar].width;
    else if (dir == 2)
      x -= rotfont->per_char[ichar].width;
    else
      y += rotfont->per_char[ichar].width;
  }
  XSetFillStyle(dpy, gc, FillSolid);
}

#ifndef FLWM
/* *** Return the width of a string *** */

static int XRotTextWidth(XRotFontStruct *rotfont, const char *str, int len)
{
  int i, width = 0, ichar;

  if (str == NULL) return 0;

  if (rotfont->dir == 0)
    width = XTextWidth(rotfont->xfontstruct, str, strlen(str));

  else
    for (i = 0; i<len; i++) {
      width += rotfont->per_char[((unsigned char*)str)[i]].width;
    }

  return width;
}
#endif

/* ---------------------------------------------------------------------- */

// the public functions use the fltk global variables for font & gc:

static XRotFontStruct* font;

static void setrotfont(int angle) {
  /* make angle positive ... */
  if (angle < 0) do angle += 360; while (angle < 0);
  /* get nearest vertical or horizontal direction ... */
  int dir = ((angle+45)/90)%4;
  if (font) {
    if (font->xfontstruct == fl_xfont && font->dir == dir) return;
    XRotUnloadFont(fl_display, font);
  }
  font = XRotLoadFont(fl_display, fl_xfont, dir);
}

void draw_rotated(const char* text, int n, int x, int y, int angle) {
  if (!text || !*text) return;
  setrotfont(angle);
  XRotDrawString(fl_display, font, fl_window, fl_gc, x, y, text, n);
}

#if !defined(FLWM) || FL_MAJOR_VERSION>=2
void draw_rotated(const char* text, int x, int y, int angle) {
  if (!text || !*text) return;
  draw_rotated(text, strlen(text), x, y, angle);
}
#endif

static void draw_rot90(const char* str, int n, int x, int y) {
  draw_rotated(str, n, y, -x, 90);
}
void draw_rotated90(
  const char* str, 	// the (multi-line) string
  int x, int y, int w, int h, 	// bounding box
  Fl_Align align) {
  if (!str || !*str) return;
  if (w && h && !fl_not_clipped(x, y, w, h)) return;
  if (align & FL_ALIGN_CLIP) fl_clip(x, y, w, h);
#if FL_MAJOR_VERSION>1
  setrotfont(90);
  int a = font->xfontstruct->ascent;
  int d = font->xfontstruct->descent;
  XRotDrawString(fl_display, font, fl_window, fl_gc,
		 x+(w+a-d+1)/2, y+h, str, strlen(str));
#else
  int a1 = align&(-16);
  if (align & FL_ALIGN_LEFT) a1 |= FL_ALIGN_TOP;
  if (align & FL_ALIGN_RIGHT) a1 |= FL_ALIGN_BOTTOM;
  if (align & FL_ALIGN_TOP) a1 |= FL_ALIGN_RIGHT;
  if (align & FL_ALIGN_BOTTOM) a1 |= FL_ALIGN_LEFT;
  fl_draw(str, -(y+h), x, h, w, (Fl_Align)a1, draw_rot90);
#endif
  if (align & FL_ALIGN_CLIP) fl_pop_clip();
}

#ifndef FLWM
static void draw_rot180(const char* str, int n, int x, int y) {
  draw_rotated(str, n, -x, -y, 180);
}
void draw_rotated180(
  const char* str, 	// the (multi-line) string
  int x, int y, int w, int h, 	// bounding box
  Fl_Align align) {
  int a1 = align&(-16);
  if (align & FL_ALIGN_LEFT) a1 |= FL_ALIGN_RIGHT;
  if (align & FL_ALIGN_RIGHT) a1 |= FL_ALIGN_LEFT;
  if (align & FL_ALIGN_TOP) a1 |= FL_ALIGN_BOTTOM;
  if (align & FL_ALIGN_BOTTOM) a1 |= FL_ALIGN_TOP;
  fl_draw(str, -(x+w), -(y+h), w, h, (Fl_Align)a1, draw_rot180);
}

static void draw_rot270(const char* str, int n, int x, int y) {
  draw_rotated(str, n, -y, x, 270);
}
void draw_rotated270(
  const char* str, 	// the (multi-line) string
  int x, int y, int w, int h, 	// bounding box
  Fl_Align align) {
  int a1 = align&(-16);
  if (align & FL_ALIGN_LEFT) a1 |= FL_ALIGN_BOTTOM;
  if (align & FL_ALIGN_RIGHT) a1 |= FL_ALIGN_TOP;
  if (align & FL_ALIGN_TOP) a1 |= FL_ALIGN_LEFT;
  if (align & FL_ALIGN_BOTTOM) a1 |= FL_ALIGN_RIGHT;
  fl_draw(str, y, -(x+w), h, w, (Fl_Align)a1, draw_rot270);
}
#endif

