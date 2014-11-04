// Frame.H

// Each X window being managed by fltk has one of these

#ifndef Frame_H
#define Frame_H

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/x.H>
#if FL_MAJOR_VERSION<2
# define XWindow Window
#endif

// The state is an enumeration of reasons why the window may be invisible.
// Only if it is NORMAL is the window visible.
enum {
  UNMAPPED	= 0,	// unmap command from app (X calls this WithdrawnState)
  NORMAL	= 1,	// window is visible
//SHADED	= 2,	// acts like NORMAL
  ICONIC	= 3,	// hidden/iconized
  OTHER_DESKTOP	= 4	// normal but on another desktop
};

// values for flags:
// The flags are constant and are turned on by information learned
// from the Gnome, KDE, and/or Motif window manager hints.  Flwm will
// ignore attempts to change these hints after the window is mapped.
enum {
  NO_FOCUS		= 0x0001, // does not take focus
  CLICK_TO_FOCUS	= 0x0002, // must click on window to give it focus
  NO_BORDER		= 0x0004, // raw window with no border
  THIN_BORDER		= 0x0008, // just resize border
  NO_RESIZE		= 0x0010, // don't resize even if sizehints say its ok
  NO_CLOSE		= 0x0040, // don't put a close box on it
  TAKE_FOCUS_PROTOCOL	= 0x0080, // send junk when giving window focus
  DELETE_WINDOW_PROTOCOL= 0x0100, // close box sends a message
  KEEP_ASPECT		= 0x0200, // aspect ratio from sizeHints
  MODAL			= 0x0400, // grabs focus from transient_for window
  ICONIZE		= 0x0800, // transient_for_ actually means group :-(
  QUIT_PROTOCOL		= 0x1000, // Irix 4DWM "quit" menu item
  SAVE_PROTOCOL		= 0x2000  // "WM_SAVE_YOURSELF" stuff
};

// values for state_flags:
// These change over time
enum {
  IGNORE_UNMAP		= 0x01,	// we did something that echos an UnmapNotify
  SAVE_PROTOCOL_WAIT	= 0x02
};

class FrameButton : public Fl_Button {
  void draw();
public:
  FrameButton(int X, int Y, int W, int H, const char* L=0)
    : Fl_Button(X,Y,W,H,L) {}
};

class Desktop;

class Frame : public Fl_Window {

  XWindow window_;

  short state_;		// X server state: iconic, withdrawn, normal
  short state_flags_;	// above state flags
  void set_state_flag(short i) {state_flags_ |= i;}
  void clear_state_flag(short i) {state_flags_&=~i;}

  int flags_;		// above constant flags
  void set_flag(int i) {flags_ |= i;}
  void clear_flag(int i) {flags_&=~i;}

  int restore_x, restore_w; // saved size when min/max width is set
  int restore_y, restore_h; // saved size when max height is set
  int min_w, max_w, inc_w; // size range and increment
  int min_h, max_h, inc_h; // size range and increment
  int app_border_width;	// value of border_width application tried to set

  int left, top, dwidth, dheight; // current thickness of border
  int label_y, label_h; // location of label
  int label_w;		    // measured width of printed label

  XWindow transient_for_xid; // value from X
  Frame* transient_for_; // the frame for that xid, if found

  Frame* revert_to;	// probably the xterm this was run from

  Colormap colormap;	// this window's colormap
  int colormapWinCount; // list of other windows to install colormaps for
  XWindow *colormapWindows;
  Colormap *window_Colormaps; // their colormaps

  Desktop* desktop_;

  FrameButton close_button;
  FrameButton iconize_button;
  FrameButton max_h_button;
  FrameButton max_w_button;
  FrameButton min_w_button;

  int maximize_width();
  int maximize_height();
  int force_x_onscreen(int X, int W);
  int force_y_onscreen(int Y, int H);
  void place_window();

  void sendMessage(Atom, Atom) const;
  void sendConfigureNotify() const;
  void setStateProperty() const;

  void* getProperty(Atom, Atom = AnyPropertyType, int* length = 0) const;
  int  getIntProperty(Atom, Atom = AnyPropertyType, int deflt = 0) const;
  void setProperty(Atom, Atom, int) const;
  void getLabel(int del = 0);
  void getColormaps();
  int  getSizes();
  int  getGnomeState(int&);
  void getProtocols();
  int  getMotifHints();
  void updateBorder();
  void fix_transient_for(); // called when transient_for_xid changes

  void installColormap() const;

  void resize(int,int,int,int);
  void layout();
  void show_hide_buttons();

  int handle(int);	// handle fltk events
  void set_cursor(int);
  int mouse_location();

  void draw();

  static Frame* active_;
  static void button_cb_static(Fl_Widget*, void*);
  void button_cb(Fl_Button*);

  void deactivate();
  int activate_if_transient();
  void _desktop(Desktop*);

  int border() const {return !(flags_&NO_BORDER);}
  int flags() const {return flags_;}
  int flag(int i) const {return flags_&i;}
  void throw_focus(int destructor = 0);
  void warp_pointer();

public:

  void set_size(int,int,int,int, int warp=0);  // ML: made public

  int handle(const XEvent*);

  static Frame* first;
  Frame* next;		// stacking order, top to bottom

  Frame(XWindow, XWindowAttributes* = 0);
  ~Frame();

  XWindow window() const {return window_;}
  Frame* transient_for() const {return transient_for_;}
  int is_transient_for(const Frame*) const;

  Desktop* desktop() const {return desktop_;}
  void desktop(Desktop*);

  void raise(); // also does map
  void lower();
  void iconize();
  void close();
  void kill();
  int activate(int warp = 0); // returns true if it actually sets active state

  short state() const {return state_;}
  void state(short); // don't call this unless you know what you are doing!

  int active() const {return active_==this;}
  static Frame* activeFrame() {return active_;}

  static void save_protocol(); // called when window manager exits

  // The following should be conditionally defined based on the
  // SHOW_CLOCK definition in config.h but that definition is not
  // available at the time we are evaluating this; it does no harm
  // to be present even if not SHOW_CLOCK.
  void redraw_clock();

};

// handy wrappers for those ugly X routines:
void* getProperty(XWindow, Atom, Atom = AnyPropertyType, int* length = 0);
int getIntProperty(XWindow, Atom, Atom = AnyPropertyType, int deflt = 0);
void setProperty(XWindow, Atom, Atom, int);

#endif
