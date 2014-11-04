// FrameWindow.H

// X does not echo back the window-map events (it probably should when
// override_redirect is off).  Unfortunately this means you have to use
// this subclass if you want a "normal" fltk window, it will force a
// Frame to be created and destroy it upon hide.

// Warning: modal() does not work!  Don't turn it on as it screws up the
// interface with the window borders.  You can use set_non_modal() to
// disable the iconize box but the window manager must be written to
// not be modal.

#ifndef FrameWindow_H
#define FrameWindow_H

#include <FL/Fl_Window.H>
class Frame;

class FrameWindow : public Fl_Window {
  Frame* frame;
public:
  void hide();
  void show();
  int handle(int);
  FrameWindow(int X, int Y, int W, int H, const char* L = 0) :
    Fl_Window(X,Y,W,H,L) {}
  FrameWindow(int W, int H, const char* L = 0) :
    Fl_Window(W,H,L) {}
};

#endif
