// Rotated text drawing with X.

// Original code:
// Copyright (c) 1992 Alan Richardson (mppa3@uk.ac.sussex.syma) */
//
// Modifications for fltk:
// Copyright (c) 1997 Bill Spitzak (spitzak@d2.com)

#ifndef Rotated_H
#define Rotated_H

void draw_rotated(const char* text, int n, int x, int y, int angle);
void draw_rotated(const char* text, int x, int y, int angle);
void draw_rotated90(const char*, int x, int y, int w, int h, Fl_Align);
void draw_rotated270(const char*, int x, int y, int w, int h, Fl_Align);
void draw_rotated180(const char*, int x, int y, int w, int h, Fl_Align);

#endif 
