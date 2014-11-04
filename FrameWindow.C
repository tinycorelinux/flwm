// FrameWindow.C

// X does not echo back the window-map events (it probably should when
// override_redirect is off).  Unfortunately this means you have to use
// this subclass if you want a "normal" fltk window, it will force a
// Frame to be created and destroy it upon hide.

// Warning: modal() does not work!  Don't turn it on as it screws up the
// interface with the window borders.  You can use set_non_modal() to
// disable the iconize box but the window manager must be written to
// not be modal.

#include <FL/Fl.H>
#include "FrameWindow.H"
#include "Frame.H"

extern int dont_set_event_mask;

void FrameWindow::show() {
  if (shown()) {Fl_Window::show(); return;}
  Fl_Window::show();
  dont_set_event_mask = 1;
  frame = new Frame(fl_xid(this));
  dont_set_event_mask = 0;
}

void FrameWindow::hide() {
  if (shown()) {
    Fl_Window::hide();
    delete frame;
  }
}

int FrameWindow::handle(int e) {
  if (Fl_Window::handle(e)) return 1;
  // make Esc close the window:
  if (e == FL_SHORTCUT && Fl::event_key()==FL_Escape) {
    do_callback();
    return 1;
  }
  return 0;
}
