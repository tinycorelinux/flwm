// Frame.C
//
// CHANGES
// 20090927: Some modifications by Michael A. Losh, tagged "ML" below,
//   or bracketed by TOPSIDE manifest constant
// 20160402: some tests clarified (more parentheses ...); dentonlt

#define FL_INTERNALS 1

#include "config.h"
#include "Frame.H"
#include "Desktop.H"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <FL/fl_draw.H>
#include "Rotated.H"

static Atom wm_state = 0;
static Atom wm_change_state;
static Atom wm_protocols;
static Atom wm_delete_window;
static Atom wm_take_focus;
static Atom wm_save_yourself;
static Atom wm_colormap_windows;
static Atom _motif_wm_hints;
static Atom kwm_win_decoration;
#if DESKTOPS
static Atom kwm_win_desktop;
static Atom kwm_win_sticky;
#endif
//static Atom wm_client_leader;
static Atom _wm_quit_app;

// these are set by initialize in main.C:
Atom _win_hints;
Atom _win_state;
#if DESKTOPS
extern Atom _win_workspace;
#endif

#ifdef SHOW_CLOCK
extern char clock_buf[];
extern int clock_alarm_on;
#endif

static const int XEventMask =
ExposureMask|StructureNotifyMask
|KeyPressMask|KeyReleaseMask|KeymapStateMask|FocusChangeMask
|ButtonPressMask|ButtonReleaseMask
|EnterWindowMask|LeaveWindowMask
|PointerMotionMask|SubstructureRedirectMask|SubstructureNotifyMask;

extern Fl_Window* Root;

Frame* Frame::active_;
Frame* Frame::first;

static inline int max(int a, int b) {return a > b ? a : b;}
static inline int min(int a, int b) {return a < b ? a : b;}

////////////////////////////////////////////////////////////////
// The constructor is by far the most complex part, as it collects
// all the scattered pieces of information about the window that
// X has and uses them to initialize the structure, position the
// window, and then finally create it.

int dont_set_event_mask = 0; // used by FrameWindow

// "existing" is a pointer to an XWindowAttributes structure that is
// passed for an already-existing window when the window manager is
// starting up.  If so we don't want to alter the state, size, or
// position.  If null than this is a MapRequest of a new window.
Frame::Frame(XWindow window, XWindowAttributes* existing) :
  Fl_Window(0,0),
  window_(window),
  state_flags_(0),
  flags_(0),
  transient_for_xid(None),
  transient_for_(0),
  revert_to(active_),
  colormapWinCount(0),
  close_button(BUTTON_LEFT,BUTTON_TOP,BUTTON_W,BUTTON_H,"X"),
  iconize_button(BUTTON_LEFT,BUTTON_TOP,BUTTON_W,BUTTON_H,"i"),
  max_h_button(BUTTON_LEFT,BUTTON_TOP+3*BUTTON_H,BUTTON_W,BUTTON_H,"h"),
  max_w_button(BUTTON_LEFT,BUTTON_TOP+BUTTON_H,BUTTON_W,BUTTON_H,"w"),
  min_w_button(BUTTON_LEFT,BUTTON_TOP+2*BUTTON_H,BUTTON_W,BUTTON_H,"W")
{
  close_button.callback(button_cb_static);
  iconize_button.callback(button_cb_static);
  max_h_button.type(FL_TOGGLE_BUTTON);
  max_h_button.callback(button_cb_static);
  max_w_button.type(FL_TOGGLE_BUTTON);
  max_w_button.callback(button_cb_static);
  min_w_button.type(FL_TOGGLE_BUTTON);
  min_w_button.callback(button_cb_static);
  end();
  box(FL_NO_BOX); // relies on background color erasing interior
  // ML -------------------------------
#ifdef ML_TITLEBAR_COLOR
  char * titlebar_color_str = getenv("FLWM_TITLEBAR_COLOR");
  int r = 0x90, g =  0x90, b = 0x90;
  int fields = 0;
  if (titlebar_color_str) {
	  fields = sscanf(titlebar_color_str, "%02X:%02X:%02X", &r, &g, &b);
  }
  if (titlebar_color_str && (fields == 3)) {
	  Fl::set_color(FL_BACKGROUND2_COLOR, r, g, b);
  }
  else {
	  Fl::set_color(FL_BACKGROUND2_COLOR, 0x90, 0x90, 0x90);
  }
#else
  Fl::set_color(FL_BACKGROUND2_COLOR, 0x90, 0x90, 0x90);
#endif
  // --------------------------------ML
  labelcolor(FL_FOREGROUND_COLOR);
  next = first;
  first = this;

  // do this asap so we don't miss any events...
  if (!dont_set_event_mask)
    XSelectInput(fl_display, window_,
		 ColormapChangeMask | PropertyChangeMask | FocusChangeMask
		 );

  if (!wm_state) {
    // allocate all the atoms if this is the first time
    wm_state		= XInternAtom(fl_display, "WM_STATE",		0);
    wm_change_state	= XInternAtom(fl_display, "WM_CHANGE_STATE",	0);
    wm_protocols	= XInternAtom(fl_display, "WM_PROTOCOLS",	0);
    wm_delete_window	= XInternAtom(fl_display, "WM_DELETE_WINDOW",	0);
    wm_take_focus	= XInternAtom(fl_display, "WM_TAKE_FOCUS",	0);
    wm_save_yourself	= XInternAtom(fl_display, "WM_SAVE_YOURSELF",	0);
    wm_colormap_windows	= XInternAtom(fl_display, "WM_COLORMAP_WINDOWS",0);
    _motif_wm_hints	= XInternAtom(fl_display, "_MOTIF_WM_HINTS",	0);
    kwm_win_decoration	= XInternAtom(fl_display, "KWM_WIN_DECORATION",	0);
#if DESKTOPS
    kwm_win_desktop	= XInternAtom(fl_display, "KWM_WIN_DESKTOP",	0);
    kwm_win_sticky	= XInternAtom(fl_display, "KWM_WIN_STICKY",	0);
#endif
//  wm_client_leader	= XInternAtom(fl_display, "WM_CLIENT_LEADER",	0);
    _wm_quit_app	= XInternAtom(fl_display, "_WM_QUIT_APP",	0);
  }

#ifdef TOPSIDE
  label_x = label_h = label_w = 0;
#else
  label_y = label_h = label_w = 0;
#endif
  getLabel();
  // getIconLabel();

  {XWindowAttributes attr;
  if (existing) attr = *existing;
  else {
    // put in some legal values in case XGetWindowAttributes fails:
    attr.x = attr.y = 0; attr.width = attr.height = 100;
    attr.colormap = fl_colormap;
    attr.border_width = 0;
    XGetWindowAttributes(fl_display, window, &attr);
  }
  left = top = dwidth = dheight = 0; // pretend border is zero-width for now
  app_border_width = attr.border_width;
  x(attr.x+app_border_width); restore_x = x();
  y(attr.y+app_border_width); restore_y = y();
  w(attr.width); restore_w = w();
  h(attr.height); restore_h = h();
  colormap = attr.colormap;}

  getColormaps();

  //group_ = 0;
  {XWMHints* hints = XGetWMHints(fl_display, window_);
  if (hints) {
    if ((hints->flags & InputHint) && !hints->input) set_flag(NO_FOCUS);
    //if (hints && hints->flags&WindowGroupHint) group_ = hints->window_group;
  }
  switch (getIntProperty(wm_state, wm_state, 0)) {
  case NormalState:
    state_ = NORMAL; break;
  case IconicState:
    state_ = ICONIC; break;
  // X also defines obsolete values ZoomState and InactiveState
  default:
    if ((hints && (hints->flags&StateHint)) && (hints->initial_state==IconicState))
      state_ = ICONIC;
    else
      state_ = NORMAL;
  }
  if (hints) XFree(hints);}
  // Maya sets this, seems to mean the same as group:
  // if (!group_) group_ = getIntProperty(wm_client_leader, XA_WINDOW);

  XGetTransientForHint(fl_display, window_, &transient_for_xid);

  getProtocols();

  getMotifHints();

  // get Gnome hints:
  int p = getIntProperty(_win_hints, XA_CARDINAL);
  if (p&1) set_flag(NO_FOCUS);		// WIN_HINTS_SKIP_FOCUS
  // if (p&2)				// WIN_HINTS_SKIP_WINLIST
  // if (p&4)				// WIN_HINTS_SKIP_TASKBAR
  // if (p&8) ...			// WIN_HINTS_GROUP_TRANSIENT
  if (p&16) set_flag(CLICK_TO_FOCUS);	// WIN_HINTS_FOCUS_ON_CLICK

  // get KDE hints:
  p = getIntProperty(kwm_win_decoration, kwm_win_decoration, 1);
  if (!(p&3)) set_flag(NO_BORDER);
  else if (p & 2) set_flag(THIN_BORDER);
  if (p & 256) set_flag(NO_FOCUS);

  fix_transient_for();

  if (transient_for()) {
    if (state_ == NORMAL) state_ = transient_for()->state_;
#if DESKTOPS
    desktop_ = transient_for()->desktop_;
#endif
  }
#if DESKTOPS
  // see if anybody thinks window is "sticky:"
  else if ((getIntProperty(_win_state, XA_CARDINAL) & 1) // WIN_STATE_STICKY
      || getIntProperty(kwm_win_sticky, kwm_win_sticky)) {
    desktop_ = 0;
  } else {
    // get the desktop from either Gnome or KDE (Gnome takes precedence):
    p = getIntProperty(_win_workspace, XA_CARDINAL, -1) + 1; // Gnome desktop
    if (p <= 0) p = getIntProperty(kwm_win_desktop, kwm_win_desktop);
    if ((p > 0) && (p < 25))
      desktop_ = Desktop::number(p, 1);
    else
      desktop_ = Desktop::current();
  }
  if (desktop_ && (desktop_ != Desktop::current()))
    if (state_ == NORMAL) state_ = OTHER_DESKTOP;
#endif

  int autoplace = getSizes();
  // some Motif programs assumme this will force the size to conform :-(
  if (w() < min_w || h() < min_h) {
    if (w() < min_w) w(min_w);
    if (h() < min_h) h(min_h);
    XResizeWindow(fl_display, window_, w(), h());
  }

  // try to detect programs that think "transient_for" means "no border":
  if (transient_for_xid && !label() && !flag(NO_BORDER)) {
    set_flag(THIN_BORDER);
  }
  updateBorder();
  show_hide_buttons();

  if (autoplace && !existing && !(transient_for() && (x() || y()))) {
    place_window();
  }

  // move window so contents and border are visible:
  x(force_x_onscreen(x(), w()));
  y(force_y_onscreen(y(), h()));

  // guess some values for the "restore" fields, if already maximized:
  if (max_w_button.value()) {
    restore_w = min_w + ((w()-dwidth-min_w)/2/inc_w) * inc_w;
    restore_x = x()+left + (w()-dwidth-restore_w)/2;
  }
  if (max_h_button.value()) {
    restore_h = min_h + ((h()-dheight-min_h)/2/inc_h) * inc_h;
    restore_y = y()+top + (h()-dheight-restore_h)/2;
  }

  const int mask = CWBorderPixel | CWColormap | CWEventMask | CWBitGravity
    | CWBackPixel | CWOverrideRedirect;
  XSetWindowAttributes sattr;
  sattr.event_mask = XEventMask;
  sattr.colormap = fl_colormap;
  sattr.border_pixel = fl_xpixel(FL_GRAY0);
  sattr.bit_gravity = NorthWestGravity;
  sattr.override_redirect = 1;
  sattr.background_pixel = fl_xpixel(FL_GRAY);
  Fl_X::set_xid(this, XCreateWindow(fl_display,
				    RootWindow(fl_display,fl_screen),
			     x(), y(), w(), h(), 0,
			     fl_visual->depth,
			     InputOutput,
			     fl_visual->visual,
			     mask, &sattr));

  setStateProperty();

  if (!dont_set_event_mask) XAddToSaveSet(fl_display, window_);
  if (existing) set_state_flag(IGNORE_UNMAP);
  XReparentWindow(fl_display, window_, fl_xid(this), left, top);
  XSetWindowBorderWidth(fl_display, window_, 0);
  if (state_ == NORMAL) XMapWindow(fl_display, window_);
  sendConfigureNotify(); // many apps expect this even if window size unchanged

#if CLICK_RAISES || CLICK_TO_TYPE
  if (!dont_set_event_mask)
    XGrabButton(fl_display, AnyButton, AnyModifier, window, False,
		ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
#endif

  if (state_ == NORMAL) {
    XMapWindow(fl_display, fl_xid(this));
    if (!existing) activate_if_transient();
  }
  set_visible();
}

#if SMART_PLACEMENT
// Helper functions for "smart" window placement.
int overlap1(int p1, int l1, int p2, int l2) {
  int ret = 0;
  if(p1 <= p2 && p2 <= p1 + l1) {
    ret = min(p1 + l1 - p2, l2);
  } else if (p2 <= p1 && p1 <= p2 + l2) {
    ret = min(p2 + l2 - p1, l1);
  }
  return ret;
}

int overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
  return (overlap1(x1, w1, x2, w2) * overlap1(y1, h1, y2, h2));
}

// Compute the overlap with existing windows.
// For normal windows the overlapping area is taken into account plus a
// constant value for every overlapping window.
// The active window counts twice.
// For iconic windows half the overlapping area is taken into account.
int getOverlap(int x, int y, int w, int h, Frame *first, Frame *self) {
  int ret = 0;
  short state;
  for (Frame* f = first; f; f = f->next) {
    if (f != self) {
      state = f->state();
      if (state == NORMAL || state == ICONIC) {
	int o = overlap(x, y, w, h, f->x(), f->y(), f->w(), f->h());
	if (state == NORMAL) {
	  ret = ret + o + (o>0?40000:0) + (o * f->active());
	} else if (state == ICONIC) {
	  ret = ret + o/2;
	}
      }
    }
  }
  return ret;
}

// autoplacement (brute force version for now)
void Frame::place_window() {
  int min_overlap = -1;
  int tmp_x, tmp_y, tmp_o;
  int best_x = 0;
  int best_y = 0;
  int _w = w();
  int _h = h();
  int max_x = Root->x() + Root->w();
  int max_y = Root->y() + Root->h();

  Frame *f1 = Frame::first;
  for(int i=0;; i++) {
    if (i==0) {
      tmp_x = 0;
    } else if (i==1) {
      tmp_x = max_x - _w;
    } else {
      if (f1 == this) {
	f1 = f1->next;
      }
      if (!f1) {
	break;
      }
      tmp_x = f1->x() + f1->w();
      f1 = f1->next;
    }
    Frame *f2 = Frame::first;
    for(int j=0;; j++) {
      if (j==0) {
	tmp_y = 0;
      } else if (j==1) {
	tmp_y = max_y - _h;
      } else {
	if (f2 == this) {
	  f2 = f2->next;
	}
	if (!f2) {
	  break;
	}
	tmp_y = f2->y() + f2->h();
	f2 = f2->next;
      }

      if ((tmp_x + _w <= max_x) && (tmp_y + _h <= max_y)) {
	tmp_o = getOverlap(tmp_x, tmp_y, _w, _h, Frame::first, this);
	if(tmp_o < min_overlap || min_overlap < 0) {
	  best_x = tmp_x;
	  best_y = tmp_y;
	  min_overlap = tmp_o;
	  if (min_overlap == 0) {
	    break;
	  }
	}
      }
    }
    if (min_overlap == 0) {
      break;
    }
  }
  x(best_x);
  y(best_y);
}

#else

// autoplacement (stupid version for now)
void Frame::place_window() {
  x(Root->x()+(Root->w()-w())/2);
  y(Root->y()+(Root->h()-h())/2);
  // move it until it does not hide any existing windows:
  const int delta = TITLE_WIDTH+LEFT;
    for (Frame* f = next; f; f = f->next) {
      if (f->x()+delta > x() && f->y()+delta > y() &&
	  f->x()+f->w()-delta < x()+w() && f->y()+f->h()-delta < y()+h()) {
	x(max(x(),f->x()+delta));
	y(max(y(),f->y()+delta));
	f = this;
      }
    }
}
#endif

// modify the passed X & W to a legal horizontal window position
int Frame::force_x_onscreen(int X, int W) {
  // force all except the black border on-screen:
  X = min(X, Root->x()+Root->w()+1-W);
  X = max(X, Root->x()-1);
  // force the contents on-screen:
  X = min(X, Root->x()+Root->w()-W+dwidth-left);
  if (W-dwidth > Root->w() || h()-dheight > Root->h())
    // windows bigger than the screen need title bar so they can move
    X = max(X, Root->x()-LEFT);
  else
    X = max(X, Root->x()-left);
  return X;
}

// modify the passed Y & H to a legal vertical window position:
int Frame::force_y_onscreen(int Y, int H) {
  // force border (except black edge) to be on-screen:
  Y = min(Y, Root->y()+Root->h()+1-H);
  Y = max(Y, Root->y()-1);
  // force contents to be on-screen:
  Y = min(Y, Root->y()+Root->h()-H+dheight-top);
  Y = max(Y, Root->y()-top);
  return Y;
}

////////////////////////////////////////////////////////////////
// destructor
// The destructor is called on DestroyNotify, so I don't have to do anything
// to the contained window, which is already been destroyed.

Frame::~Frame() {

  // It is possible for the frame to be destroyed while the menu is
  // popped-up, and the menu will still contain a pointer to it.  To
  // fix this the menu checks the state_ location for a legal and
  // non-withdrawn state value before doing anything.  This should
  // be reliable unless something reallocates the memory and writes
  // a legal state value to this location:
  state_ = UNMAPPED;

  // remove any pointers to this:
  Frame** cp; for (cp = &first; *cp; cp = &((*cp)->next))
    if (*cp == this) {*cp = next; break;}
  for (Frame* f = first; f; f = f->next) {
    if (f->transient_for_ == this) f->transient_for_ = transient_for_;
    if (f->revert_to == this) f->revert_to = revert_to;
  }
  throw_focus(1);

  if (colormapWinCount) {
    XFree((char *)colormapWindows);
    delete[] window_Colormaps;
  }
  //if (iconlabel()) XFree((char*)iconlabel());
  if (label())     XFree((char*)label());
}

////////////////////////////////////////////////////////////////

void Frame::getLabel(int del) {
  char* old = (char*)label();
  char* nu = del ? 0 : (char*)getProperty(XA_WM_NAME);
  if (nu) {
    // since many window managers print a default label when none is
    // given, many programs send spaces to make a blank label.  Detect
    // this and make it really be blank:
    char* c = nu; while (*c == ' ') c++;
    if (!*c) {XFree(nu); nu = 0;}
  }
  if (old) {
    if (nu && !strcmp(old,nu)) {XFree(nu); return;}
    XFree(old);
  } else {
    if (!nu) return;
  }
#ifdef TOPSIDE
  Fl_Widget::label(nu);
  if (nu) {
    fl_font(TITLE_FONT_SLOT, TITLE_FONT_SIZE);
    label_h = int(fl_size())+6;
  } else
    label_h = 0;
  if (shown())// && label_w > 3 && top > 3)
    XClearArea(fl_display, fl_xid(this), label_x, BUTTON_TOP, label_w, label_h, 1);
#else
  Fl_Widget::label(nu);
  if (nu) {
    fl_font(TITLE_FONT_SLOT, TITLE_FONT_SIZE);
    label_w = int(fl_width(nu))+6;
  } else
    label_w = 0;
  if (shown() && label_h > 3 && left > 3)
    XClearArea(fl_display, fl_xid(this), 1, label_y+3, left-1, label_h-3, 1);
#endif
}

////////////////////////////////////////////////////////////////

int Frame::getGnomeState(int &) {
// values for _WIN_STATE property are from Gnome WM compliance docs:
#define WIN_STATE_STICKY          (1<<0) /*everyone knows sticky*/
#define WIN_STATE_MINIMIZED       (1<<1) /*Reserved - definition is unclear*/
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /*window in maximized V state*/
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /*window in maximized H state*/
#define WIN_STATE_HIDDEN          (1<<4) /*not on taskbar but window visible*/
#define WIN_STATE_SHADED          (1<<5) /*shaded (MacOS / Afterstep style)*/
#define WIN_STATE_HID_WORKSPACE   (1<<6) /*not on current desktop*/
#define WIN_STATE_HID_TRANSIENT   (1<<7) /*owner of transient is hidden*/
#define WIN_STATE_FIXED_POSITION  (1<<8) /*window is fixed in position even*/
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /*ignore for auto arranging*/
  // nyi
  return 0;
}

////////////////////////////////////////////////////////////////

// Read the sizeHints, and try to remove the vast number of mistakes
// that some applications seem to do writing them.
// Returns true if autoplace should be done.

int Frame::getSizes() {

  XSizeHints sizeHints;
  long junk;
  if (!XGetWMNormalHints(fl_display, window_, &sizeHints, &junk))
    sizeHints.flags = 0;

  // get the increment, use 1 if none or illegal values:
  if (sizeHints.flags & PResizeInc) {
    inc_w = sizeHints.width_inc; if (inc_w < 1) inc_w = 1;
    inc_h = sizeHints.height_inc; if (inc_h < 1) inc_h = 1;
  } else {
    inc_w = inc_h = 1;
  }

  // get the current size of the window:
  int W = w()-dwidth;
  int H = h()-dheight;
  // I try a lot of places to get a good minimum size value.  Lots of
  // programs set illegal or junk values, so getting this correct is
  // difficult:
  min_w = W;
  min_h = H;

  // guess a value for minimum size in case it is not set anywhere:
  min_w = min(min_w, 4*BUTTON_H);
  min_w = ((min_w+inc_w-1)/inc_w) * inc_w;
  min_h = min(min_h, 4*BUTTON_H);
  min_h = ((min_h+inc_h-1)/inc_h) * inc_h;
  // some programs put the minimum size here:
  if (sizeHints.flags & PBaseSize) {
    junk = sizeHints.base_width; if (junk > 0) min_w = junk;
    junk = sizeHints.base_height; if (junk > 0) min_h = junk;
  }
  // finally, try the actual place the minimum size should be:
  if (sizeHints.flags & PMinSize) {
    junk = sizeHints.min_width; if (junk > 0) min_w = junk;
    junk = sizeHints.min_height; if (junk > 0) min_h = junk;
  }

  max_w = max_h = 0; // default maximum size is "infinity"
  if (sizeHints.flags & PMaxSize) {
    // Though not defined by ICCCM standard, I interpret any maximum
    // size that is less than the minimum to mean "infinity".  This
    // allows the maximum to be set in one direction only:
    junk = sizeHints.max_width;
    if (junk >= min_w && junk <= W) max_w = junk;
    junk = sizeHints.max_height;
    if (junk >= min_h && junk <= H) max_h = junk;
  }

  // set the maximize buttons according to current size:
  max_w_button.value(W == maximize_width());
  max_h_button.value(H == maximize_height());

  // Currently only 1x1 aspect works:
  if (sizeHints.flags & PAspect
      && sizeHints.min_aspect.x == sizeHints.min_aspect.y)
    set_flag(KEEP_ASPECT);

  // another fix for gimp, which sets PPosition to 0,0:
  if (x() <= 0 && y() <= 0) sizeHints.flags &= ~PPosition;

  return !(sizeHints.flags & (USPosition|PPosition));
}

int max_w_switch;
// return width of contents when maximize button pressed:
int Frame::maximize_width() {
  int W = max_w_switch; if (!W) W = Root->w();
#ifdef TOPSIDE
  return ((W-min_w)/inc_w) * inc_w + min_w;
#else
  return ((W-TITLE_WIDTH-min_w)/inc_w) * inc_w + min_w;
#endif
}

int max_h_switch;
int Frame::maximize_height() {
  int H = max_h_switch; if (!H) H = Root->h();
#ifdef TOPSIDE
  return ((H-TITLE_HEIGHT-min_h)/inc_h) * inc_h + min_h;
#else
  return ((H-min_h)/inc_h) * inc_h + min_h;
#endif
}

////////////////////////////////////////////////////////////////

void Frame::getProtocols() {
  int n; Atom* p = (Atom*)getProperty(wm_protocols, XA_ATOM, &n);
  if (p) {
    clear_flag(DELETE_WINDOW_PROTOCOL|TAKE_FOCUS_PROTOCOL|QUIT_PROTOCOL);
    for (int i = 0; i < n; ++i) {
      if (p[i] == wm_delete_window) {
	set_flag(DELETE_WINDOW_PROTOCOL);
      } else if (p[i] == wm_take_focus) {
	set_flag(TAKE_FOCUS_PROTOCOL);
      } else if (p[i] == wm_save_yourself) {
	set_flag(SAVE_PROTOCOL);
      } else if (p[i] == _wm_quit_app) {
	set_flag(QUIT_PROTOCOL);
      }
    }
  }
  XFree((char*)p);
}

////////////////////////////////////////////////////////////////

int Frame::getMotifHints() {
  long* prop = (long*)getProperty(_motif_wm_hints, _motif_wm_hints);
  if (!prop) return 0;

  // see /usr/include/X11/Xm/MwmUtil.h for meaning of these bits...
  // prop[0] = flags (what props are specified)
  // prop[1] = functions (all, resize, move, minimize, maximize, close, quit)
  // prop[2] = decorations (all, border, resize, title, menu, minimize,
  //                        maximize)
  // prop[3] = input_mode (modeless, primary application modal, system modal,
  //                       full application modal)
  // prop[4] = status (tear-off window)

  // Fill in the default value for missing fields:
  if (!(prop[0]&1)) prop[1] = 1;
  if (!(prop[0]&2)) prop[2] = 1;

  // The low bit means "turn the marked items off", invert this.
  // Transient windows already have size & iconize buttons turned off:
  if (prop[1]&1) prop[1] = ~prop[1] & (transient_for_xid ? ~0x58 : -1);
  if (prop[2]&1) prop[2] = ~prop[2] & (transient_for_xid ? ~0x60 : -1);

  int old_flags = flags();

  // see if they are trying to turn off border:
  if (!(prop[2])) set_flag(NO_BORDER); else clear_flag(NO_BORDER);

  // see if they are trying to turn off title & close box:
  if (!(prop[2]&0x18)) set_flag(THIN_BORDER); else clear_flag(THIN_BORDER);

  // some Motif programs use this to disable resize :-(
  // and some programs change this after the window is shown (*&%$#%)
  if (!(prop[1]&2) || !(prop[2]&4))
    set_flag(NO_RESIZE); else clear_flag(NO_RESIZE);

  // and some use this to disable the Close function.  The commented
  // out test is it trying to turn off the mwm menu button: it appears
  // programs that do that still expect Alt+F4 to close them, so I
  // leave the close on then:
  if (!(prop[1]&0x20) /*|| !(prop[2]&0x10)*/)
    set_flag(NO_CLOSE); else clear_flag(NO_CLOSE);

  // see if they set "input hint" to non-zero:
  // prop[3] should be nonzero but the only example of this I have
  // found is Netscape 3.0 and it sets it to zero...
  if (!shown() && (prop[0]&4) /*&& prop[3]*/) set_flag(::MODAL);

  // see if it is forcing the iconize button back on.  This makes
  // transient_for act like group instead...
  if ((prop[1]&0x8) || (prop[2]&0x20)) set_flag(ICONIZE);

  // Silly 'ol Amazon paint ignores WM_DELETE_WINDOW and expects to
  // get the SGI-specific "_WM_QUIT_APP".  It indicates this by trying
  // to turn off the close box. SIGH!!!
  if (flag(QUIT_PROTOCOL) && !(prop[1]&0x20))
    clear_flag(DELETE_WINDOW_PROTOCOL);

  XFree((char*)prop);
  return (flags() ^ old_flags);
}

////////////////////////////////////////////////////////////////

void Frame::getColormaps(void) {
  if (colormapWinCount) {
    XFree((char *)colormapWindows);
    delete[] window_Colormaps;
  }
  int n;
  XWindow* cw = (XWindow*)getProperty(wm_colormap_windows, XA_WINDOW, &n);
  if (cw) {
    colormapWinCount = n;
    colormapWindows = cw;
    window_Colormaps = new Colormap[n];
    for (int i = 0; i < n; ++i) {
      if (cw[i] == window_) {
	window_Colormaps[i] = colormap;
      } else {
	XWindowAttributes attr;
	XSelectInput(fl_display, cw[i], ColormapChangeMask);
	XGetWindowAttributes(fl_display, cw[i], &attr);
	window_Colormaps[i] = attr.colormap;
      }
    }
  } else {
    colormapWinCount = 0;
  }
}

void Frame::installColormap() const {
  for (int i = colormapWinCount; i--;)
    if (colormapWindows[i] != window_ && window_Colormaps[i])
      XInstallColormap(fl_display, window_Colormaps[i]);
  if (colormap)
    XInstallColormap(fl_display, colormap);
}

////////////////////////////////////////////////////////////////

// figure out transient_for(), based on the windows that exist, the
// transient_for and group attributes, etc:
void Frame::fix_transient_for() {
  Frame* p = 0;
  if (transient_for_xid && !flag(ICONIZE)) {
    for (Frame* f = first; f; f = f->next) {
      if (f != this && f->window_ == transient_for_xid) {p = f; break;}
    }
    // loops are illegal:
    for (Frame* q = p; q; q = q->transient_for_) if (q == this) {p = 0; break;}
  }
  transient_for_ = p;
}

int Frame::is_transient_for(const Frame* f) const {
  if (f)
    for (Frame* p = transient_for(); p; p = p->transient_for())
      if (p == f) return 1;
  return 0;
}

// When a program maps or raises a window, this is called.  It guesses
// if this window is in fact a modal window for the currently active
// window and if so transfers the active state to this:
// This also activates new main windows automatically
int Frame::activate_if_transient() {
  if (!Fl::pushed())
    if (!transient_for() || is_transient_for(active_)) return activate(1);
  return 0;
}

////////////////////////////////////////////////////////////////

int Frame::activate(int warp) {
  // see if a modal & newer window is up:
  for (Frame* c = first; c && c != this; c = c->next)
    if (c->flag(::MODAL) && c->transient_for() == this)
      if (c->activate(warp)) return 1;
  // ignore invisible windows:
  if (state() != NORMAL || w() <= dwidth) return 0;
  // always put in the colormap:
  installColormap();
  // move the pointer if desired:
  // (note that moving the pointer is pretty much required for point-to-type
  // unless you know the pointer is already in the window):
  if (!warp || Fl::event_state() & (FL_BUTTON1|FL_BUTTON2|FL_BUTTON3)) {
    ;
  } else if (warp==2) {
    // warp to point at title:
#ifdef TOPSIDE
    XWarpPointer(fl_display, None, fl_xid(this), 0,0,0,0, left/2+1,
                 min(label_x+label_w/2+1, label_h/2));
#else
    XWarpPointer(fl_display, None, fl_xid(this), 0,0,0,0, left/2+1,
                 min(label_y+label_w/2+1,h()/2));
#endif
  } else {
    warp_pointer();
  }
  // skip windows that don't want focus:
  if (flag(NO_FOCUS)) return 0;
  // set this even if we think it already has it, this seems to fix
  // bugs with Motif popups:
  XSetInputFocus(fl_display, window_, RevertToPointerRoot, fl_event_time);
  if (active_ != this) {
    if (active_) active_->deactivate();
    active_ = this;
//#ifdef ACTIVE_COLOR
//    XSetWindowAttributes a;
//    a.background_pixel = fl_xpixel(FL_SELECTION_COLOR);
//    XChangeWindowAttributes(fl_display, fl_xid(this), CWBackPixel, &a);
//    labelcolor(fl_contrast(FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR)); // ML changed color comparison
//    XClearArea(fl_display, fl_xid(this), 2, 2, w()-4, h()-4, 1);
//#else
    XSetWindowAttributes a;
    a.background_pixel = fl_xpixel(FL_BACKGROUND2_COLOR);
    XChangeWindowAttributes(fl_display, fl_xid(this), CWBackPixel, &a);
    labelcolor(fl_contrast(FL_FOREGROUND_COLOR, FL_BACKGROUND2_COLOR));  // ML changed color comparison
    XClearArea(fl_display, fl_xid(this), 2, 2, w()-4, h()-4, 1);
#ifdef SHOW_CLOCK
    redraw();
#endif
//#endif
    if (flag(TAKE_FOCUS_PROTOCOL))
      sendMessage(wm_protocols, wm_take_focus);
  }
  return 1;
}

// this private function should only be called by constructor and if
// the window is active():
void Frame::deactivate() {
#ifdef ACTIVE_COLOR
    XSetWindowAttributes a;
    a.background_pixel = fl_xpixel(FL_GRAY);
    XChangeWindowAttributes(fl_display, fl_xid(this), CWBackPixel, &a);
    labelcolor(FL_FOREGROUND_COLOR);
    XClearArea(fl_display, fl_xid(this), 2, 2, w()-4, h()-4, 1);
#else
#ifdef SHOW_CLOCK
    redraw();
#endif
#endif
}

#if CLICK_RAISES || CLICK_TO_TYPE
// After the XGrabButton, the main loop will get the mouse clicks, and
// it will call here when it gets them:
void click_raise(Frame* f) {
  f->activate();
#if CLICK_RAISES
  if (fl_xevent->xbutton.button <= 1) f->raise();
#endif
  XAllowEvents(fl_display, ReplayPointer, CurrentTime);
}
#endif

// get rid of the focus by giving it to somebody, if possible:
void Frame::throw_focus(int destructor) {
  if (!active()) return;
  if (!destructor) deactivate();
  active_ = 0;
  if (revert_to && revert_to->activate()) return;
  for (Frame* f = first; f; f = f->next)
    if (f != this && f->activate()) return;
}

////////////////////////////////////////////////////////////////

// change the state of the window (this is a private function and
// it ignores the transient-for or desktop information):

void Frame::state(short newstate) {
  short oldstate = state();
  if (newstate == oldstate) return;
  state_ = newstate;
  switch (newstate) {
  case UNMAPPED:
    throw_focus();
    XUnmapWindow(fl_display, fl_xid(this));
    //set_state_flag(IGNORE_UNMAP);
    //XUnmapWindow(fl_display, window_);
    XRemoveFromSaveSet(fl_display, window_);
    break;
  case NORMAL:
    if (oldstate == UNMAPPED) XAddToSaveSet(fl_display, window_);
    if (w() > dwidth) XMapWindow(fl_display, window_);
    XMapWindow(fl_display, fl_xid(this));
    clear_state_flag(IGNORE_UNMAP);
    break;
  default:
    if (oldstate == UNMAPPED) {
      XAddToSaveSet(fl_display, window_);
    } else if (oldstate == NORMAL) {
      throw_focus();
      XUnmapWindow(fl_display, fl_xid(this));
      //set_state_flag(IGNORE_UNMAP);
      //XUnmapWindow(fl_display, window_);
    } else {
      return; // don't setStateProperty IconicState multiple times
    }
    break;
  }
  setStateProperty();
}

void Frame::setStateProperty() const {
  long data[2];
  switch (state()) {
  case UNMAPPED :
    data[0] = WithdrawnState; break;
  case NORMAL :
  case OTHER_DESKTOP :
    data[0] = NormalState; break;
  default :
    data[0] = IconicState; break;
  }
  data[1] = (long)None;
  XChangeProperty(fl_display, window_, wm_state, wm_state,
		  32, PropModeReplace, (unsigned char *)data, 2);
}

////////////////////////////////////////////////////////////////
// Public state modifiers that move all transient_for(this) children
// with the frame and do the desktops right:

void Frame::raise() {
  Frame* newtop = 0;
  Frame* previous = 0;
  int previous_state = state_;
  Frame** p;
  // Find all the transient-for windows and this one, and raise them,
  // preserving stacking order:
  for (p = &first; *p;) {
    Frame* f = *p;
    if ((f == this) || (f->is_transient_for(this) && (f->state() != UNMAPPED))) {
      *p = f->next; // remove it from list
      if (previous) {
	XWindowChanges w;
	w.sibling = fl_xid(previous);
	w.stack_mode = Below;
	XConfigureWindow(fl_display, fl_xid(f), CWSibling|CWStackMode, &w);
	previous->next = f;
      } else {
	XRaiseWindow(fl_display, fl_xid(f));
	newtop = f;
      }
#if DESKTOPS
      if (f->desktop_ && f->desktop_ != Desktop::current())
       f->state(OTHER_DESKTOP);
      else
#endif
	f->state(NORMAL);
      previous = f;
    } else {
      p = &((*p)->next);
    }
  }
  previous->next = first;
  first = newtop;
#if DESKTOPS
  if (!transient_for() && desktop_ && desktop_ != Desktop::current()) {
    // for main windows we also must move to the current desktop
    desktop(Desktop::current());
  }
#endif
  if (previous_state != NORMAL && newtop->state_==NORMAL)
    newtop->activate_if_transient();
}

void Frame::lower() {
  Frame* t = transient_for(); if (t) t->lower();
  if (!next || next == t) return; // already on bottom
  // pull it out of the list:
  Frame** p = &first;
  for (; *p != this; p = &((*p)->next)) {}
  *p = next;
  // find end of list:
  Frame* f = next; while (f->next != t) f = f->next;
  // insert it after that:
  f->next = this; next = t;
  // and move the X window:
  XWindowChanges w;
  w.sibling = fl_xid(f);
  w.stack_mode = Below;
  XConfigureWindow(fl_display, fl_xid(this), CWSibling|CWStackMode, &w);
}

void Frame::iconize() {
  for (Frame* c = first; c; c = c->next) {
    if ((c == this) || (c->is_transient_for(this) && (c->state() != UNMAPPED)))
      c->state(ICONIC);
  }
}

#if DESKTOPS
void Frame::desktop(Desktop* d) {
  if (d == desktop_) return;
  // Put all the relatives onto the desktop as well:
  for (Frame* c = first; c; c = c->next) {
    if (c == this || c->is_transient_for(this)) {
      c->desktop_ = d;
      c->setProperty(_win_state, XA_CARDINAL, !d);
      c->setProperty(kwm_win_sticky, kwm_win_sticky, !d);
      if (d) {
	c->setProperty(kwm_win_desktop, kwm_win_desktop, d->number());
	c->setProperty(_win_workspace, XA_CARDINAL, d->number()-1);
      }
      if (!d || d == Desktop::current()) {
	if (c->state() == OTHER_DESKTOP) c->state(NORMAL);
      } else {
	if (c->state() == NORMAL) c->state(OTHER_DESKTOP);
      }
    }
  }
}
#endif

////////////////////////////////////////////////////////////////

// Resize and/or move the window.  The size is given for the frame, not
// the contents.  This also sets the buttons on/off as needed:

void Frame::set_size(int nx, int ny, int nw, int nh, int warp) {
  int dx = nx-x(); x(nx);
  int dy = ny-y(); y(ny);
  if (!dx && !dy && nw == w() && nh == h()) return;
  int unmap = 0;
  int remap = 0;
  // use XClearArea to cause correct damage events:
  if (nw != w()) {
    max_w_button.value(nw-dwidth == maximize_width());
    min_w_button.value(nw <= dwidth);
    if (nw <= dwidth) {
      unmap = 1;
    } else {
      if (w() <= dwidth) remap = 1;
    }
    int minw = (nw < w()) ? nw : w();
    XClearArea(fl_display, fl_xid(this), minw-RIGHT, 0, RIGHT, nh, 1);
    w(nw);
#ifdef TOPSIDE
	show_hide_buttons();
#endif
  }
  if (nh != h()) {
    max_h_button.value(nh-dheight == maximize_height());
    int minh = (nh < h()) ? nh : h();
    XClearArea(fl_display, fl_xid(this), 0, minh-BOTTOM, w(), BOTTOM, 1);
    // see if label or close box moved, erase the minimum area:
//     int old_label_y = label_y;
//     int old_label_h = label_h;
    h(nh);
#if 1 //def SHOW_CLOCK
#ifdef TOPSIDE
    //int t = label_x + 3; // we have to clear the entire label area
	XClearArea(fl_display,fl_xid(this), label_x, BUTTON_TOP, label_x + label_w,
				BUTTON_H, 1);  // ML
#else
	show_hide_buttons();
    //int t = label_y + 3; // we have to clear the entire label area
	XClearArea(fl_display,fl_xid(this), 1, label_y, left-1, nh-label_y, 1);  // ML
#endif
#else
    int t = nh;
    if (label_y != old_label_y) {
      t = label_y; if (old_label_y < t) t = old_label_y;
    } else if (label_y+label_h != old_label_y+old_label_h) {
      t = label_y+label_h;
      if (old_label_y+old_label_h < t) t = old_label_y+old_label_h;
    }
#endif
//ML    if (t < nh && left>LEFT)
//ML      XClearArea(fl_display,fl_xid(this), 1, t, left-1, nh-t, 1);
  }
  // for maximize button move the cursor first if window gets smaller
  if (warp == 1 && (dx || dy))
    XWarpPointer(fl_display, None,None,0,0,0,0, dx, dy);
  // for configure request, move the cursor first
  if (warp == 2 && active() && !Fl::pushed()) warp_pointer();
  XMoveResizeWindow(fl_display, fl_xid(this), nx, ny, nw, nh);
  if (nw <= dwidth) {
    if (unmap) {
      set_state_flag(IGNORE_UNMAP);
      XUnmapWindow(fl_display, window_);
    }
  } else {
    XResizeWindow(fl_display, window_, nw-dwidth, nh-dheight);
    if (remap) {
      XMapWindow(fl_display, window_);
#if CLICK_TO_TYPE
      if (active()) activate();
#else
      activate();
#endif
    }
  }
  // for maximize button move the cursor second if window gets bigger:
  if (warp == 3 && (dx || dy))
    XWarpPointer(fl_display, None,None,0,0,0,0, dx, dy);
  if (nw > dwidth) sendConfigureNotify();
  XSync(fl_display,0);
}

void Frame::sendConfigureNotify() const {
  XConfigureEvent ce;
  ce.type   = ConfigureNotify;
  ce.event  = window_;
  ce.window = window_;
  ce.x = x()+left-app_border_width;
  ce.y = y()+top-app_border_width;
  ce.width  = w()-dwidth;
  ce.height = h()-dheight;
  ce.border_width = app_border_width;
  ce.above = None;
  ce.override_redirect = 0;
  XSendEvent(fl_display, window_, False, StructureNotifyMask, (XEvent*)&ce);
}

// move the pointer inside the window:
void Frame::warp_pointer() {
  int X,Y; Fl::get_mouse(X,Y);
  X -= x();
  int Xi = X;
  if (X <= 0) X = left/2+1;
  if (X >= w()) X = w()-(RIGHT/2+1);
  Y -= y();
  int Yi = Y;
  if (Y < 0) Y = TOP/2+1;
  if (Y >= h()) Y = h()-(BOTTOM/2+1);
  if (X != Xi || Y != Yi)
    XWarpPointer(fl_display, None, fl_xid(this), 0,0,0,0, X, Y);
}

// Resize the frame to match the current border type:
void Frame::updateBorder() {
  int nx = x()+left;
  int ny = y()+top;
  int nw = w()-dwidth;
  int nh = h()-dheight;
  if (flag(NO_BORDER)) {
    left = top = dwidth = dheight = 0;
  } else {
#ifdef TOPSIDE
    left = LEFT;
    dwidth = left+RIGHT;
    top = flag(THIN_BORDER) ? TOP : TOP+TITLE_HEIGHT;
    dheight = top+BOTTOM;
#else
    left = flag(THIN_BORDER) ? LEFT : LEFT+TITLE_WIDTH;
    dwidth = left+RIGHT;
    top = TOP;
    dheight = TOP+BOTTOM;
#endif
  }
  nx -= left;
  ny -= top;
  nw += dwidth;
  nh += dheight;
  if (x()==nx && y()==ny && w()==nw && h()==nh) return;
  x(nx); y(ny); w(nw); h(nh);
  if (!shown()) return; // this is so constructor can call this
  // try to make the contents not move while the border changes around it:
  XSetWindowAttributes a;
  a.win_gravity = StaticGravity;
  XChangeWindowAttributes(fl_display, window_, CWWinGravity, &a);
  XMoveResizeWindow(fl_display, fl_xid(this), nx, ny, nw, nh);
  a.win_gravity = NorthWestGravity;
  XChangeWindowAttributes(fl_display, window_, CWWinGravity, &a);
  // fix the window position if the X server didn't do the gravity:
  XMoveWindow(fl_display, window_, left, top);
}

// position and show the buttons according to current border, size,
// and other state information:
#ifdef TOPSIDE
 // FIRST, the TOPSIDE VERSION of the function
void Frame::show_hide_buttons() {
  if (flag(THIN_BORDER|NO_BORDER)) {
    iconize_button.hide();
    max_w_button.hide();
    min_w_button.hide();
    max_h_button.hide();
    close_button.hide();
    return;
  }
  int bx = BUTTON_LEFT;

// RS: ICONIZE BUTTON IS TOP LEFT --
  if (transient_for()) {
	// don't show iconize button for "transient" (e.g. dialog box) windows
    iconize_button.hide();
  } else {
    iconize_button.show();
    iconize_button.position(BUTTON_LEFT, BUTTON_TOP);
  }
//  ML: OTHER BUTTONS ARE PLACED IN UPPER RIGHT
  bx = w() - BUTTON_RIGHT - BUTTON_W;

// RS: FIRST PLACE CLOSE BUTTON FARTHEST INTO UPPER RIGHT
#if CLOSE_BOX
  if (flag(NO_CLOSE)) {
#endif
    close_button.hide();
#if CLOSE_BOX
  } else {
    close_button.show();
    close_button.position(bx, BUTTON_TOP);
	bx -= BUTTON_W;
  }
#endif
  if (transient_for()) {
	// don't show resize and iconize buttons for "transient" (e.g. dialog box) windows
    max_w_button.hide();
    max_h_button.hide();
  } else {
    max_h_button.position(bx, BUTTON_TOP);
    max_h_button.show();
	bx -= BUTTON_W;
    max_w_button.position(bx, BUTTON_TOP);
    max_w_button.show();
	bx -= BUTTON_W;
  }

  if (!transient_for()) {
    iconize_button.position(bx, BUTTON_TOP);
    iconize_button.show();
	bx -= BUTTON_W;
  }
  else {
    iconize_button.hide();

  }
  if (label_x != bx && shown())
//ML Buttons look garbled after expanding, so let's just clear the whole area
    XClearArea(fl_display,fl_xid(this), LEFT, TOP, w() - LEFT, TITLE_HEIGHT, 1);
  label_x = BUTTON_LEFT + left;
  label_w = bx - label_x;

}

#else // have a left-side titlebar version

void Frame::show_hide_buttons() {
  if (flag(THIN_BORDER|NO_BORDER)) {
    iconize_button.hide();
    max_w_button.hide();
    min_w_button.hide();
    max_h_button.hide();
    close_button.hide();
    return;
  }
  int by = BUTTON_TOP;

// ML CLOSE BUTTON IS NOW TOP LEFT --
#if CLOSE_BOX
  if (flag(NO_CLOSE)) {
#endif
    close_button.hide();
#if CLOSE_BOX
  } else {
    close_button.show();
    close_button.position(BUTTON_LEFT,by);
    by += BUTTON_H;
  }
#endif
  if (!transient_for()) {
    min_w_button.position(BUTTON_LEFT,by);
    min_w_button.show();
    by += BUTTON_H;
  }
  else {
    min_w_button.hide();
  }
  if ((min_h == max_h) || flag(KEEP_ASPECT|NO_RESIZE) ||
      (!max_h_button.value() && ((by+label_w+2*BUTTON_H) > (h()-BUTTON_BOTTOM)))) {
    max_h_button.hide();
  } else {
    max_h_button.position(BUTTON_LEFT,by);
    max_h_button.show();
    by += BUTTON_H;
  }
  if ((min_w == max_w) || flag(KEEP_ASPECT|NO_RESIZE) ||
      (!max_w_button.value() && ((by+label_w+2*BUTTON_H > h()-BUTTON_BOTTOM)))) {
    max_w_button.hide();
  } else {
    max_w_button.position(BUTTON_LEFT,by);
    max_w_button.show();
    by += BUTTON_H;
  }
  if (label_y != by && shown())
//ML    XClearArea(fl_display,fl_xid(this), 1, by, left-1, label_h+label_y-by, 1);
//ML Buttons look garbled after expanding, so let's just clear the whole area
    XClearArea(fl_display,fl_xid(this), 1, 1, left-1, h()-1, 1);
  label_y = by;

  //ML MOVED ICONIZE BUTTON TO BOTTOM --
  if (by+BUTTON_H > h()-BUTTON_BOTTOM || transient_for()) {
    label_h = h()-BOTTOM-by;
    iconize_button.hide();
  } else {
    iconize_button.show();
    iconize_button.position(BUTTON_LEFT,h()-(BUTTON_BOTTOM+BUTTON_H));
    label_h = iconize_button.y()-by;
  }
// -- END ML

}
#endif

// make sure fltk does not try to set the window size:
void Frame::resize(int, int, int, int) {}
// For fltk2.0:
void Frame::layout() {
}

////////////////////////////////////////////////////////////////

void Frame::close() {
  if (flag(DELETE_WINDOW_PROTOCOL))
    sendMessage(wm_protocols, wm_delete_window);
  else if (flag(QUIT_PROTOCOL))
    sendMessage(wm_protocols, _wm_quit_app);
  else
    kill();
}

void Frame::kill() {
  XKillClient(fl_display, window_);
}

// this is called when window manager exits:
void Frame::save_protocol() {
  Frame* f;
  for (f = first; f; f = f->next) if (f->flag(SAVE_PROTOCOL)) {
    f->set_state_flag(SAVE_PROTOCOL_WAIT);
    f->sendMessage(wm_protocols, wm_save_yourself);
  }
  double t = 10.0; // number of seconds to wait before giving up
  while (t > 0.0) {
    for (f = first; ; f = f->next) {
      if (!f) return;
      if (f->flag(SAVE_PROTOCOL) && f->state_flags_&SAVE_PROTOCOL_WAIT) break;
    }
    t = Fl::wait(t);
  }
}

////////////////////////////////////////////////////////////////
// Drawing code:

#ifdef TOPSIDE
void Frame::draw() {
  if (flag(NO_BORDER)) return;
  //ML--------------------- Paint opaque titlebar background
	  labelcolor(fl_contrast(FL_FOREGROUND_COLOR, FL_BACKGROUND2_COLOR));
	  if (active()) {
	 	 fl_rectf(2, 2, w() - 4, h()-4,
		   fl_color_average(FL_BACKGROUND2_COLOR, FL_WHITE, 0.6)
//        (ACTIVE_COLOR >> 16) & 0xFF, (ACTIVE_COLOR >> 8) & 0xFF, ACTIVE_COLOR & 0xFF
			);
      }
      else {
        fl_rectf(2, 2, w() - 4, h()-4,
		FL_BACKGROUND2_COLOR
		//FL_GRAY
					);
      }
	  // ------------------ML

  if (damage() != FL_DAMAGE_CHILD) {

#ifdef ACTIVE_COLOR
    fl_frame2(active() ? "AAAAJJWW" : "AAAAJJWWNNTT",0,0,w(),h());
    if (active()) {
      fl_color(FL_GRAY_RAMP+('N'-'A'));
      fl_xyline(2, h()-3, w()-3, 2);
    }
#else
    fl_frame("AAAAWWJJTTNN",0,0,w(),h());
#endif
    if (!flag(THIN_BORDER) && label_h > 3) {
      fl_color(labelcolor());
      fl_font(TITLE_FONT_SLOT, TITLE_FONT_SIZE);
      fl_draw(label(), label_x, BUTTON_TOP, label_w, BUTTON_H,
		      Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_CLIP), 0, 0);
    }
  }
  if (!flag(THIN_BORDER)) Fl_Window::draw();
}
#else
void Frame::draw() {
  if (flag(NO_BORDER)) return;
  //ML--------------------- Paint opaque titlebar background
	  labelcolor(fl_contrast(FL_FOREGROUND_COLOR, FL_BACKGROUND2_COLOR));
	  if (active()) {
	 	 fl_rectf(2, 2, w() - 4, h()-4,
		 fl_color_average(FL_BACKGROUND2_COLOR, FL_WHITE, 0.6) ); // ML
        //(ACTIVE_COLOR >> 16) & 0xFF, (ACTIVE_COLOR >> 8) & 0xFF, ACTIVE_COLOR & 0xFF);
      }
      else {
        fl_rectf(2, 2, w() - 4, h()-4, FL_BACKGROUND2_COLOR); // ML
      }
	  // ------------------ML

  if (damage() != FL_DAMAGE_CHILD) {
#ifdef ACTIVE_COLOR
    fl_frame2(active() ? "AAAAJJWW" : "AAAAJJWWNNTT",0,0,w(),h());
    if (active()) {
      fl_color(FL_GRAY_RAMP+('N'-'A'));
      fl_xyline(2, h()-3, w()-3, 2);
    }
#else
    fl_frame("AAAAWWJJTTNN",0,0,w(),h());
#endif
    if (!flag(THIN_BORDER) && label_h > 3) {
#ifdef SHOW_CLOCK
      if (active()) {
	  int clkw = int(fl_width(clock_buf));
	  if (clock_alarm_on) {
	      fl_font(TITLE_FONT_SLOT, TITLE_FONT_SIZE);
	      fl_rectf(LEFT-1, label_y + label_h - 3 - clkw, TITLE_WIDTH, clkw,
		       (ALARM_BG_COLOR>>16)&0xff,
		       (ALARM_BG_COLOR>>8)&0xff,
		       ALARM_BG_COLOR&0xff);
	      fl_color((ALARM_FG_COLOR>>16)&0xff,
		       (ALARM_FG_COLOR>>8)&0xff,
		       ALARM_FG_COLOR&0xff);
	  } else
	      fl_font(MENU_FONT_SLOT, TITLE_FONT_SIZE);
	  // This might overlay the label if the label is long enough
	  // and the window height is short enough.  For now, we'll
	  // assume this is not enough of a problem to be concerned
	  // about.
	  draw_rotated90(clock_buf, 1, label_y+3, left-1, label_h-6,
			 Fl_Align(FL_ALIGN_BOTTOM|FL_ALIGN_CLIP));
      } else
	  // Only show the clock on the active frame.
	  XClearArea(fl_display, fl_xid(this), 1, label_y+3,
		     left-1, label_h-3, 0);
#endif
      fl_color(labelcolor());
      fl_font(TITLE_FONT_SLOT, TITLE_FONT_SIZE);
      draw_rotated90(label(), 1, label_y+3, left-1, label_h-3,
		     Fl_Align(FL_ALIGN_TOP|FL_ALIGN_CLIP));
    }
  }
  if (!flag(THIN_BORDER)) Fl_Window::draw();
}
#endif

#ifdef SHOW_CLOCK
void Frame::redraw_clock() {
    double clkw = fl_width(clock_buf);
    XClearArea(fl_display, fl_xid(this),
	       1, label_y+label_h-3-(int)clkw,
	       left-1, (int)clkw, 1);
}
#endif

void FrameButton::draw() {
  const int x = this->x();
  const int y = this->y();
  Fl_Widget::draw_box(value() ? FL_DOWN_FRAME : FL_UP_FRAME,
					  value() ? fl_darker(FL_BACKGROUND2_COLOR)
							  : fl_color_average(FL_BACKGROUND2_COLOR, FL_WHITE, 0.6)); // ML
//  Fl_Widget::draw_box(value() ? FL_DOWN_FRAME : FL_UP_FRAME, FL_GRAY);
  fl_color(parent()->labelcolor());
  switch (label()[0]) {
  case 'W':
    fl_rect(x+(h()-7)/2,y+3,2,h()-6);
    return;
  case 'w':
    fl_rect(x+2,y+(h()-7)/2,w()-4,7);
    return;
  case 'h':
    fl_rect(x+(h()-7)/2,y+2,7,h()-4);
    return;
  case 'X':
#if CLOSE_X
    fl_line(x+2,y+3,x+w()-5,y+h()-4);
    fl_line(x+3,y+3,x+w()-4,y+h()-4);
    fl_line(x+2,y+h()-4,x+w()-5,y+3);
    fl_line(x+3,y+h()-4,x+w()-4,y+3);
#endif
#if CLOSE_HITTITE_LIGHTNING
    fl_arc(x+3,y+3,w()-6,h()-6,0,360);
    fl_line(x+7,y+3, x+7,y+11);
#endif
    return;
  case 'i':
#if ICONIZE_BOX
    fl_line (x+2, y+(h()-4), x+w()-4, y+h()-4);
#endif
    return;
  }
}

////////////////////////////////////////////////////////////////
// User interface code:

// this is called when user clicks the buttons:
void Frame::button_cb(Fl_Button* b) {
  switch (b->label()[0]) {
  case 'W':	// minimize button
    if (b->value()) {
      if (!max_w_button.value()) {
        restore_x = x()+left;
	restore_y = y()+top;
#if MINIMIZE_HEIGHT
	restore_w=w()-dwidth;
	restore_h = h()-dwidth;
#endif
      }
#if MINIMIZE_HEIGHT
#ifdef TOPSIDE
      set_size(x(), y(), dwidth-1, 350,1);// <-- crude hack for now
#else
      set_size(x(), y(), dwidth-1,
	       min(h(),min(350,label_w+3*BUTTON_H+BUTTON_TOP+BUTTON_BOTTOM)),
	       1);
#endif // not topside
#else
      set_size(x(), y(), dwidth-1, h(), 1);
#endif
    } else {
#if MINIMIZE_HEIGHT
      set_size(x(), y(), restore_w+dwidth, restore_h+dwidth, 1);
#else
      set_size(x(), y(), restore_w+dwidth, h(), 1);
#endif
    }
    show_hide_buttons();
    break;
  case 'w':	// max-width button
    if (b->value()) {
      if (!min_w_button.value()) {restore_x=x()+left; restore_w=w()-dwidth;}
      int W = maximize_width()+dwidth;
      int X = force_x_onscreen(x() + (w()-W)/2, W);
      set_size(X, y(), W, h(), 3);
    } else {
      set_size(restore_x-left, y(), restore_w+dwidth, h(), 1);
    }
    show_hide_buttons();
    break;
  case 'h':	// max-height button
    if (b->value()) {
      restore_y = y()+top;
      restore_h = h()-dwidth;
      int H = maximize_height()+dheight;
      int Y = force_y_onscreen(y() + (h()-H)/2, H);
      set_size(x(), Y, w(), H, 3);
    } else {
      set_size(x(), restore_y-top, w(), restore_h+dwidth, 1);
    }
    break;
  case 'X':
    close();
    break;
  default: // iconize button
    iconize();
    break;
  }
}

// static callback for fltk:
void Frame::button_cb_static(Fl_Widget* w, void*) {
  ((Frame*)(w->parent()))->button_cb((Fl_Button*)w);
}

// This method figures out what way the mouse will resize the window.
// It is used to set the cursor and to actually control what you grab.
// If the window cannot be resized in some direction this should not
// return that direction.
int Frame::mouse_location() {
  int x = Fl::event_x();
  int y = Fl::event_y();
  int r = 0;
  if (flag(NO_RESIZE)) return 0;
  if (min_h != max_h) {
    if (y < RESIZE_EDGE) r |= FL_ALIGN_TOP;
    else if (y >= h()-RESIZE_EDGE) r |= FL_ALIGN_BOTTOM;
  }
  if (min_w != max_w) {
#if RESIZE_LEFT
    if (x < RESIZE_EDGE) r |= FL_ALIGN_LEFT;
#else
    if (x < RESIZE_EDGE && r) r |= FL_ALIGN_LEFT;
#endif
    else if (x >= w()-RESIZE_EDGE) r |= FL_ALIGN_RIGHT;
  }
  return r;
}

// set the cursor correctly for a return value from mouse_location():
void Frame::set_cursor(int r) {
//  Fl_Cursor c = r ? FL_CURSOR_ARROW : FL_CURSOR_MOVE;
  Fl_Cursor c = r ? FL_CURSOR_ARROW : FL_CURSOR_ARROW;
  switch (r) {
  case FL_ALIGN_TOP:
  case FL_ALIGN_BOTTOM:
    c = FL_CURSOR_NS;
    break;
  case FL_ALIGN_LEFT:
  case FL_ALIGN_RIGHT:
    c = FL_CURSOR_WE;
    break;
  case FL_ALIGN_LEFT|FL_ALIGN_TOP:
  case FL_ALIGN_RIGHT|FL_ALIGN_BOTTOM:
    c = FL_CURSOR_NWSE;
    break;
  case FL_ALIGN_LEFT|FL_ALIGN_BOTTOM:
  case FL_ALIGN_RIGHT|FL_ALIGN_TOP:
    c = FL_CURSOR_NESW;
    break;
  }
//#if FL_MAJOR_VERSION>1
  cursor(c);
//#else
//  static Frame* previous_frame;
//  static Fl_Cursor previous_cursor;
//  if (this != previous_frame || c != previous_cursor) {
//    previous_frame = this;
//    previous_cursor = c;
//    cursor(c, CURSOR_FG_SLOT, CURSOR_BG_SLOT);
//  }
//#endif
}

#ifdef AUTO_RAISE
// timeout callback to cause autoraise:
void auto_raise(void*) {
  if (Frame::activeFrame() && !Fl::grab() && !Fl::pushed())
    Frame::activeFrame()->raise();
}
#endif

extern void ShowMenu();

// If cursor is in the contents of a window this is set to that window.
// This is only used to force the cursor to an arrow even though X keeps
// sending mysterious erroneous move events:
static Frame* cursor_inside = 0;

// Handle an fltk event.
int Frame::handle(int e) {
  static int what, dx, dy, ix, iy, iw, ih;
  // see if child widget handles event:
  if (Fl_Group::handle(e) && e != FL_ENTER && e != FL_MOVE) {
    if (e == FL_PUSH) set_cursor(-1);
    return 1;
  }
  switch (e) {

  case FL_SHOW:
  case FL_HIDE:
    return 0; // prevent fltk from messing things up

  case FL_ENTER:
#if !CLICK_TO_TYPE
    if (Fl::pushed() || Fl::grab()) return 1;
    if (activate()) {
#ifdef AUTO_RAISE
      Fl::remove_timeout(auto_raise);
      Fl::add_timeout(AUTO_RAISE, auto_raise);
#endif
    }
#endif
    goto GET_CROSSINGS;

  case 0:
  GET_CROSSINGS:
    // set cursor_inside to true when the mouse is inside a window
    // set it false when mouse is on a frame or outside a window.
    // fltk mangles the X enter/leave events, we need the original ones:

    switch (fl_xevent->type) {
    case EnterNotify:

      // see if cursor skipped over frame and directly to interior:
      if (fl_xevent->xcrossing.detail == NotifyVirtual ||
	  fl_xevent->xcrossing.detail == NotifyNonlinearVirtual)
	cursor_inside = this;

      else {
	// cursor is now pointing at frame:
	cursor_inside = 0;
      }

      // fall through to FL_MOVE:
      break;

    case LeaveNotify:
      if (fl_xevent->xcrossing.detail == NotifyInferior) {
        // cursor moved from frame to interior
        cursor_inside = this;
        set_cursor(-1);
        return 1;
      }
      return 1;

    default:
      return 0; // other X event we don't understand
    }
    case FL_MOVE:
    if (Fl::belowmouse() != this || cursor_inside == this)
      set_cursor(-1);
    else
      set_cursor(mouse_location());
    return 1;

  case FL_PUSH:
    if (Fl::event_button() > 2) {
      set_cursor(-1);
      ShowMenu();
      return 1;
    }
    ix = x(); iy = y(); iw = w(); ih = h();
    if (!max_w_button.value() && !min_w_button.value()) {
      restore_x = ix+left; restore_w = iw-dwidth;
    }
#if MINIMIZE_HEIGHT
    if (!min_w_button.value())
#endif
    if (!max_h_button.value()) {
      restore_y = iy+top; restore_h = ih-dwidth;
    }
    what = mouse_location();
    if (Fl::event_button() > 1) what = 0; // middle button does drag
    dx = Fl::event_x_root()-ix;
    if (what & FL_ALIGN_RIGHT) dx -= iw;
    dy = Fl::event_y_root()-iy;
    if (what & FL_ALIGN_BOTTOM) dy -= ih;
    set_cursor(what);
    return 1;
  case FL_DRAG:
    if (Fl::event_is_click()) return 1; // don't drag yet
  case FL_RELEASE:
    if (Fl::event_is_click()) {
      if (Fl::grab()) return 1;
#if CLICK_TO_TYPE
      if (activate()) {
	if (Fl::event_button() <= 1) raise();
	return 1;
      }
#endif
      if (Fl::event_button() > 1) lower(); else raise();
    } else if (!what) {
      int nx = Fl::event_x_root()-dx;
      int W = Root->x()+Root->w();
      if (nx+iw > W && nx+iw < W+SCREEN_SNAP) {
	int t = W+1-iw;
	if (iw >= Root->w() || x() > t || nx+iw >= W+EDGE_SNAP)
	  t = W+(dwidth-left)-iw;
	if (t >= x() && t < nx) nx = t;
      }
      int X = Root->x();
      if (nx < X && nx > X-SCREEN_SNAP) {
	int t = X-1;
	if (iw >= Root->w() || x() < t || nx <= X-EDGE_SNAP) t = X-BUTTON_LEFT;
	if (t <= x() && t > nx) nx = t;
      }
      int ny = Fl::event_y_root()-dy;
      int H = Root->y()+Root->h();
      if (ny+ih > H && ny+ih < H+SCREEN_SNAP) {
	int t = H+1-ih;
	if (ih >= Root->h() || y() > t || ny+ih >= H+EDGE_SNAP)
	  t = H+(dheight-top)-ih;
	if (t >= y() && t < ny) ny = t;
      }
      int Y = Root->y();
      if (ny < Y && ny > Y-SCREEN_SNAP) {
	int t = Y-1;
	if (ih >= H || y() < t || ny <= Y-EDGE_SNAP) t = Y-top;
	if (t <= y() && t > ny) ny = t;
      }
      set_size(nx, ny, iw, ih);
    } else {
      int nx = ix;
      int ny = iy;
      int nw = iw;
      int nh = ih;
      if (what & FL_ALIGN_RIGHT)
	nw = Fl::event_x_root()-dx-nx;
      else if (what & FL_ALIGN_LEFT)
	nw = ix+iw-(Fl::event_x_root()-dx);
      else {nx = x(); nw = w();}
      if (what & FL_ALIGN_BOTTOM)
	nh = Fl::event_y_root()-dy-ny;
      else if (what & FL_ALIGN_TOP)
	nh = iy+ih-(Fl::event_y_root()-dy);
      else {ny = y(); nh = h();}
      if (flag(KEEP_ASPECT)) {
	if ((nw-dwidth > nh-dwidth)
	    && (what&(FL_ALIGN_LEFT|FL_ALIGN_RIGHT)
	    || !(what&(FL_ALIGN_TOP|FL_ALIGN_BOTTOM))))
	  nh = nw-dwidth+dheight;
	else
	  nw = nh-dheight+dwidth;
      }
      int MINW = min_w+dwidth;
      if (nw <= dwidth && dwidth > TITLE_WIDTH) {
	nw = dwidth-1;
#if MINIMIZE_HEIGHT
	restore_h = nh;
#endif
      } else {
	if (inc_w > 1) nw = ((nw-MINW+inc_w/2)/inc_w)*inc_w+MINW;
	if (nw < MINW) nw = MINW;
	else if (max_w && nw > max_w+dwidth) nw = max_w+dwidth;
      }
      int MINH = min_h+dheight;
      const int MINH_B = BUTTON_H+BUTTON_TOP+BOTTOM;
      if (MINH_B > MINH) MINH = MINH_B;
      if (inc_h > 1) nh = ((nh-MINH+inc_h/2)/inc_h)*inc_h+MINH;
      if (nh < MINH) nh = MINH;
      else if (max_h && nh > max_h+dheight) nh = max_h+dheight;
      if (what & FL_ALIGN_LEFT) nx = ix+iw-nw;
      if (what & FL_ALIGN_TOP) ny = iy+ih-nh;
      set_size(nx,ny,nw,nh);
    }
    return 1;
  }
  return 0;
}

// Handle events that fltk did not recognize (mostly ones directed
// at the desktop):

int Frame::handle(const XEvent* ei) {

  switch (ei->type) {

  case ConfigureRequest: {
    const XConfigureRequestEvent* e = &(ei->xconfigurerequest);
    unsigned long mask = e->value_mask;
    if (mask & CWBorderWidth) app_border_width = e->border_width;
    // Try to detect if the application is really trying to move the
    // window, or is simply echoing it's postion, possibly with some
    // variation (such as echoing the parent window position), and
    // dont' move it in that case:
    int X = (mask & CWX && e->x != x()) ? e->x+app_border_width-left : x();
    int Y = (mask & CWY && e->y != y()) ? e->y+app_border_width-top : y();
    int W = (mask & CWWidth) ? e->width+dwidth : w();
    int H = (mask & CWHeight) ? e->height+dheight : h();
    // Generally we want to obey any application positioning of the
    // window, except when it appears the app is trying to position
    // the window "at the edge".
    if (!(mask & CWX) || (X >= -2*left && X < 0)) X = force_x_onscreen(X,W);
    if (!(mask & CWY) || (Y >= -2*top && Y < 0)) Y = force_y_onscreen(Y,H);
    // Fix Rick Sayre's program that resizes it's windows bigger than the
    // maximum size:
    if (W > max_w+dwidth) max_w = 0;
    if (H > max_h+dheight) max_h = 0;
    set_size(X, Y, W, H, 2);
    if (e->value_mask & CWStackMode && e->detail == Above && state()==NORMAL)
      raise();
    return 1;}

  case MapRequest: {
    //const XMapRequestEvent* e = &(ei->xmaprequest);
    raise();
    return 1;}

  case UnmapNotify: {
    const XUnmapEvent* e = &(ei->xunmap);
    if (e->window == window_ && !e->from_configure) {
      if (state_flags_&IGNORE_UNMAP) clear_state_flag(IGNORE_UNMAP);
      else state(UNMAPPED);
    }
    return 1;}

  case DestroyNotify: {
    //const XDestroyWindowEvent* e = &(ei->xdestroywindow);
    delete this;
    return 1;}

  case ReparentNotify: {
    const XReparentEvent* e = &(ei->xreparent);
    if (e->parent==fl_xid(this)) return 1; // echo
    if (e->parent==fl_xid(Root)) return 1; // app is trying to tear-off again?
    delete this; // guess they are trying to paste tear-off thing back?
    return 1;}

  case ClientMessage: {
    const XClientMessageEvent* e = &(ei->xclient);
    if (e->message_type == wm_change_state && e->format == 32) {
      if (e->data.l[0] == NormalState) raise();
      else if (e->data.l[0] == IconicState) iconize();
    } else
      // we may want to ignore _WIN_LAYER from xmms?
      Fl::warning("flwm: unexpected XClientMessageEvent, type 0x%lx, "
	      "window 0x%lx\n", e->message_type, e->window);
    return 1;}

  case ColormapNotify: {
    const XColormapEvent* e = &(ei->xcolormap);
    if (e->c_new) {  // this field is called "new" in the old C++-unaware Xlib
      colormap = e->colormap;
      if (active()) installColormap();
    }
    return 1;}

  case PropertyNotify: {
    const XPropertyEvent* e = &(ei->xproperty);
    Atom a = e->atom;

    // case XA_WM_ICON_NAME: (do something similar to name)
    if (a == XA_WM_NAME) {
      getLabel(e->state == PropertyDelete);

    } else if (a == wm_state) {
      // it's not clear if I really need to look at this.  Need to make
      // sure it is not seeing the state echoed by the application by
      // checking for it being different...
      switch (getIntProperty(wm_state, wm_state, state())) {
      case IconicState:
	if (state() == NORMAL || state() == OTHER_DESKTOP) iconize(); break;
      case NormalState:
	if (state() != NORMAL && state() != OTHER_DESKTOP) raise(); break;
      }

    } else if (a == wm_colormap_windows) {
      getColormaps();
      if (active()) installColormap();

    } else if (a == _motif_wm_hints) {
      // some #%&%$# SGI Motif programs change this after mapping the window!
      // :-( :=( :-( :=( :-( :=( :-( :=( :-( :=( :-( :=(
      if (getMotifHints()) { // returns true if any flags changed
	fix_transient_for();
	updateBorder();
	show_hide_buttons();
      }

    } else if (a == wm_protocols) {
      getProtocols();
      // get Motif hints since they may do something with QUIT:
      getMotifHints();

    } else if (a == XA_WM_NORMAL_HINTS || a == XA_WM_SIZE_HINTS) {
      getSizes();
      show_hide_buttons();

    } else if (a == XA_WM_TRANSIENT_FOR) {
      XGetTransientForHint(fl_display, window_, &transient_for_xid);
      fix_transient_for();
      show_hide_buttons();

    } else if (a == XA_WM_COMMAND) {
      clear_state_flag(SAVE_PROTOCOL_WAIT);

    }
    return 1;}

  }
  return 0;
}

////////////////////////////////////////////////////////////////
// X utility routines:

void* Frame::getProperty(Atom a, Atom type, int* np) const {
  return ::getProperty(window_, a, type, np);
}

void* getProperty(XWindow w, Atom a, Atom type, int* np) {
  Atom realType;
  int format;
  unsigned long n, extra;
  int status;
  uchar* prop;
  status = XGetWindowProperty(fl_display, w,
			      a, 0L, 256L, False, type, &realType,
			      &format, &n, &extra, &prop);
  if (status != Success) return 0;
  if (!prop) return 0;
  if (!n) {XFree(prop); return 0;}
  if (np) *np = (int)n;
  return (void*)prop;
}

int Frame::getIntProperty(Atom a, Atom type, int deflt) const {
  return ::getIntProperty(window_, a, type, deflt);
}

int getIntProperty(XWindow w, Atom a, Atom type, int deflt) {
  void* prop = getProperty(w, a, type);
  if (!prop) return deflt;
  int r = int(*(long*)prop);
  XFree(prop);
  return r;
}

void setProperty(XWindow w, Atom a, Atom type, int v) {
  long prop = v;
  XChangeProperty(fl_display, w, a, type, 32, PropModeReplace, (uchar*)&prop,1);
}

void Frame::setProperty(Atom a, Atom type, int v) const {
  ::setProperty(window_, a, type, v);
}

void Frame::sendMessage(Atom a, Atom l) const {
  XEvent ev;
  long mask;
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = window_;
  ev.xclient.message_type = a;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = long(l);
  ev.xclient.data.l[1] = long(fl_event_time);
  mask = 0L;
  XSendEvent(fl_display, window_, False, mask, &ev);
}
