// flwm: main.cpp
//
// CHANGES
//   20160402: update XkbKeycodeToKeysym() to use XKBlib.h; some
//     tests & blocks clarified (add parentheses); dentonlt
//   20190303: Added DoNotWarp variable.
//             Added program_version define. Rich
//
// Define "TEST" and it will compile to make a single fake window so
// you can test the window controls.
//#define TEST 1

#define FL_INTERNALS 1

#include "Frame.H"
#include <X11/Xproto.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FL/filename.H>
#include "config.h"
#ifdef SHOW_CLOCK
#include <time.h>
//#include <signal.h>	// ML
#endif

//ML
#include <FL/fl_ask.H>
#include <signal.h>

////////////////////////////////////////////////////////////////

static const char* program_name;
static int initializing;

#define program_version "Version 1.21"
int DoNotWarp=0;	// Used to override mouse pointer warping if environmental variable NOWARP exists.

static int xerror_handler(Display* d, XErrorEvent* e) {
  if (initializing && (e->request_code == X_ChangeWindowAttributes) &&
      e->error_code == BadAccess)
    Fl::fatal("Another window manager is running.  You must exit it before running %s %s.", program_name, program_version);
#ifndef DEBUG
  if (e->error_code == BadWindow) return 0;
  if (e->error_code == BadColor) return 0;
#endif
  char buf1[128], buf2[128];
  sprintf(buf1, "XRequest.%d", e->request_code);
  XGetErrorDatabaseText(d,"",buf1,buf1,buf2,128);
  XGetErrorText(d, e->error_code, buf1, 128);
  Fl::warning("%s: %s: %s 0x%lx", program_name, buf2, buf1, e->resourceid);
  return 0;
}

////////////////////////////////////////////////////////////////
// The Fl_Root class looks like a window to fltk but is actually the
// screen's root window.  This is done by using set_xid to "show" it
// rather than have fltk create the window.

class Fl_Root : public Fl_Window {
  int handle(int);
public:
  Fl_Root() : Fl_Window(0,0,Fl::w(),Fl::h()) {
  }
  void show() {
    if (!shown()) Fl_X::set_xid(this, RootWindow(fl_display, fl_screen));
  }
  void flush() {}
};
Fl_Window *Root;

extern void ShowMenu();
extern int Handle_Hotkey();
extern void Grab_Hotkeys();

int Fl_Root::handle(int e) {
  if (e == FL_PUSH) {
    ShowMenu();
    return 1;
  }
  return 0;
}

#if CLICK_RAISES || CLICK_TO_TYPE
extern void click_raise(Frame*);
#endif

// fltk calls this for any events it does not understand:
static int flwm_event_handler(int e) {

  if (Fl::event_key()==FL_Escape) {
    return 1;
  }

  if (!e) { // XEvent that fltk did not understand.
    XWindow window = fl_xevent->xany.window;
    // unfortunately most of the redirect events put the interesting
    // window id in a different place:
    switch (fl_xevent->type) {
    case CirculateNotify:
    case CirculateRequest:
    case ConfigureNotify:
    case ConfigureRequest:
    case CreateNotify:
    case DestroyNotify:
    case GravityNotify:
    case MapNotify:
    case MapRequest:
    case ReparentNotify:
    case UnmapNotify:
      window = fl_xevent->xmaprequest.window;
    }

    for (Frame* c = Frame::first; c; c = c->next) {
      if (c->window() == window || fl_xid(c) == window) {
#if CLICK_RAISES || CLICK_TO_TYPE
	  if (fl_xevent->type == ButtonPress) {click_raise(c); return 1;}
	  else
#endif
      return c->handle(fl_xevent);
      }
    } // end for each child window

    switch (fl_xevent->type) {
    case ButtonPress:
      printf("got a button press in main\n");
      return 0;
    case ConfigureRequest: {
      const XConfigureRequestEvent *e = &(fl_xevent->xconfigurerequest);
      XConfigureWindow(fl_display, e->window,
		       e->value_mask&~(CWSibling|CWStackMode),
		       (XWindowChanges*)&(e->x));
      return 1;}
    case MapRequest: {
      const XMapRequestEvent* e = &(fl_xevent->xmaprequest);
      (void)new Frame(e->window);
      return 1;}
    // this was needed for *some* earlier versions of fltk
    case KeyRelease:
      if (!Fl::grab()) return 0;
      Fl::e_keysym =
	XkbKeycodeToKeysym(fl_display, fl_xevent->xkey.keycode, 0, 0);
      goto KEYUP;
    } // end switch(fl_xevent->type)

  } // end if (!e)
  else if (e == FL_KEYUP) {
  KEYUP:
    if (!Fl::grab()) return 0;
    // when alt key released, pretend they hit enter & pick menu item
    if (Fl::event_key()==FL_Alt_L || Fl::event_key()==FL_Alt_R) {
      Fl::e_keysym = FL_Enter;
      return Fl::grab()->handle(FL_KEYBOARD);
    }
    return 0;
  } else if (e == FL_SHORTCUT || e == FL_KEYBOARD) {
#if FL_MAJOR_VERSION == 1 && FL_MINOR_VERSION == 0 && FL_PATCH_VERSION < 3
    // make the tab keys work in the menus in older fltk's:
    // (they do not cycle around however, so a new fltk is a good idea)
    if (Fl::grab()) {
      // make fltk's menus resond to tab + shift+tab:
      if (Fl::event_key() == FL_Tab) {
	if (Fl::event_state() & FL_SHIFT) goto J1;
	Fl::e_keysym = FL_Down;
      } else if (Fl::event_key() == 0xFE20) {
	J1: Fl::e_keysym = FL_Up;
      } else return 0;
      return Fl::grab()->handle(FL_KEYBOARD);
    }
#endif
    return Handle_Hotkey();
  }
  return 0;
}

#if DESKTOPS
extern void init_desktops();
extern Atom _win_workspace;
extern Atom _win_workspace_count;
extern Atom _win_workspace_names;
#endif

extern Atom _win_state;
extern Atom _win_hints;

#ifdef SHOW_CLOCK
int clock_period = 1;
int clock_oldmin = 61;
int clock_alarm_on = 0;
char clock_buf[80];

struct sigaction flwm_clock_alarm_start = {0,}, flwm_clock_alarm_stop = {0,};

void flwm_update_clock(void*) {
    time_t newtime;
    struct tm *tm_p;

    // get current time
    time(&newtime);
    tm_p = localtime(&newtime);

    // Update a window frame if necessary
    if (Frame::activeFrame() && tm_p->tm_min != clock_oldmin) {
	if (clock_oldmin != 61)
	    clock_period = 60;  // now that we're in sync, only update 1/minute
	clock_oldmin = tm_p->tm_min;
	strftime(clock_buf, 80, SHOW_CLOCK, tm_p);
	Frame::activeFrame()->redraw_clock();
    }
    // Now reschedule the timeout
    Fl::remove_timeout(flwm_update_clock);
    Fl::add_timeout(clock_period, flwm_update_clock);
}

void flwm_clock_alarm_on(int) {
    clock_alarm_on = 1;
    Frame::activeFrame()->redraw_clock();
}

void flwm_clock_alarm_off(int) {
    clock_alarm_on = 0;
    Frame::activeFrame()->redraw_clock();
}
#endif

static const char* cfg, *cbg;
static int cursor = FL_CURSOR_ARROW;

// ML ------------
extern time_t wmx_time;
void request_menu_refresh(int signum) {
	if (signum == SIGUSR2) {
		wmx_time = 42;	// arbitrary value so it won't match st_mtime fire/dir status field
	}
}
// -------------ML
static void initialize() {

  Display* d = fl_display;

#ifdef TEST
  XWindow w = XCreateSimpleWindow(d, RootWindow(d, fl_screen),
				 100, 100, 200, 300, 10,
				 BlackPixel(fl_display, 0),
//				 WhitePixel(fl_display, 0));
				 0x1234);
  Frame* frame = new Frame(w);
  frame->label("flwm test window");
  XSelectInput(d, w,
	       ExposureMask | StructureNotifyMask |
	       KeyPressMask | KeyReleaseMask | FocusChangeMask |
	       KeymapStateMask |
	       ButtonPressMask | ButtonReleaseMask |
	       EnterWindowMask | LeaveWindowMask /*|PointerMotionMask*/
	       );
#else

  Fl::add_handler(flwm_event_handler);

  // setting attributes on root window makes me the window manager:
  initializing = 1;
  XSelectInput(d, fl_xid(Root),
	       SubstructureRedirectMask | SubstructureNotifyMask |
	       ColormapChangeMask | PropertyChangeMask |
	       ButtonPressMask | ButtonReleaseMask |
	       EnterWindowMask | LeaveWindowMask |
	       KeyPressMask | KeyReleaseMask | KeymapStateMask);
  Root->cursor((Fl_Cursor)cursor, CURSOR_FG_SLOT, CURSOR_BG_SLOT);
  Fl::visible_focus(0);

#ifdef TITLE_FONT
  Fl::set_font(TITLE_FONT_SLOT, TITLE_FONT);
#endif
#ifdef MENU_FONT
  Fl::set_font(MENU_FONT_SLOT, MENU_FONT);
#endif
#ifdef ACTIVE_COLOR
  Fl::set_color(FL_SELECTION_COLOR, ACTIVE_COLOR<<8);
#endif

  // Gnome crap:
  // First create a window that can be watched to see if wm dies:
  Atom a = XInternAtom(d, "_WIN_SUPPORTING_WM_CHECK", False);
  XWindow win = XCreateSimpleWindow(d, fl_xid(Root), -200, -200, 5, 5, 0, 0, 0);
  CARD32 val = win;
  XChangeProperty(d, fl_xid(Root), a, XA_CARDINAL, 32, PropModeReplace, (uchar*)&val, 1);
  XChangeProperty(d, win, a, XA_CARDINAL, 32, PropModeReplace, (uchar*)&val, 1);
  // Next send a list of Gnome stuff we understand:
  a = XInternAtom(d, "_WIN_PROTOCOLS", 0);
  Atom list[10]; unsigned int i = 0;
//list[i++] = XInternAtom(d, "_WIN_LAYER", 0);
  list[i++] = _win_state = XInternAtom(d, "_WIN_STATE", 0);
  list[i++] = _win_hints = XInternAtom(d, "_WIN_HINTS", 0);
//list[i++] = XInternAtom(d, "_WIN_APP_STATE", 0);
//list[i++] = XInternAtom(d, "_WIN_EXPANDED_SIZE", 0);
//list[i++] = XInternAtom(d, "_WIN_ICONS", 0);
#if DESKTOPS
  list[i++] = _win_workspace = XInternAtom(d, "_WIN_WORKSPACE", 0);
  list[i++] = _win_workspace_count = XInternAtom(d, "_WIN_WORKSPACE_COUNT", 0);
  list[i++] = _win_workspace_names = XInternAtom(d, "_WIN_WORKSPACE_NAMES", 0);
#endif
//list[i++] = XInternAtom(d, "_WIN_FRAME_LIST", 0);
  XChangeProperty(d, fl_xid(Root), a, XA_ATOM, 32, PropModeReplace, (uchar*)list, i);

  Grab_Hotkeys();

#ifdef SHOW_CLOCK
  Fl::add_timeout(clock_period, flwm_update_clock);
  flwm_clock_alarm_start.sa_handler = &flwm_clock_alarm_on;
  flwm_clock_alarm_stop.sa_handler = &flwm_clock_alarm_off;
  sigaction(SIGALRM, &flwm_clock_alarm_start, NULL);
  sigaction(SIGCONT, &flwm_clock_alarm_stop, NULL);
#endif

  // ML -----------
  signal(SIGUSR2, request_menu_refresh);

  // ------------ML
  XSync(d, 0);
  initializing = 0;

#if DESKTOPS
  init_desktops();
#endif

  // find all the windows and create a Frame for each:
  unsigned int n;
  XWindow w1, w2, *wins;
  XWindowAttributes attr;
  XQueryTree(d, fl_xid(Root), &w1, &w2, &wins, &n);
  for (i = 0; i < n; ++i) {
    XGetWindowAttributes(d, wins[i], &attr);
    if (attr.override_redirect || !attr.map_state) continue;
    (void)new Frame(wins[i],&attr);
  }
  XFree((void *)wins);

#endif
}

////////////////////////////////////////////////////////////////

extern int exit_flag;
extern int max_w_switch;
extern int max_h_switch;

// consume a switch from argv.  Returns number of words eaten, 0 on error:
int arg(int argc, char **argv, int &i) {
  const char *s = argv[i];
  if (s[0] != '-') return 0;
  s++;

  // do single-word switches:
  if (!strcmp(s,"x")) {
    exit_flag = 1;
    i++;
    return 1;
  }

  // do switches with a value:
  const char *v = argv[i+1];
  if (i >= argc-1 || !v)
    return 0;	// all the rest need an argument, so if missing it is an error

  if (!strcmp(s, "cfg")) {
    cfg = v;
  } else if (!strcmp(s, "cbg")) {
    cbg = v;
  } else if (*s == 'c') {
    cursor = atoi(v);
  } else if (*s == 'v') {
    int visid = atoi(v);
    fl_open_display();
    XVisualInfo templt; int num;
    templt.visualid = visid;
    fl_visual = XGetVisualInfo(fl_display, VisualIDMask, &templt, &num);
    if (!fl_visual) Fl::fatal("No visual with id %d",visid);
    fl_colormap = XCreateColormap(fl_display, RootWindow(fl_display,fl_screen),
				  fl_visual->visual, AllocNone);
  } else if (*s == 'm') {
    max_w_switch = atoi(v);
    while (*v && *v++ != 'x');
    max_h_switch = atoi(v);
  } else
    return 0; // unrecognized
  // return the fact that we consumed 2 switches:
  i += 2;
  return 2;
}

static void color_setup(Fl_Color slot, const char* arg, ulong value) {
  if (arg) {
    XColor x;
    if (XParseColor(fl_display, fl_colormap, arg, &x))
      value = ((x.red>>8)<<24)|((x.green>>8)<<16)|((x.blue));
  }
  Fl::set_color(slot, value);
}

int main(int argc, char** argv) {
  program_name = fl_filename_name(argv[0]);
  int i; if (Fl::args(argc, argv, i, arg) < argc) Fl::error(
"%s\n\n"
"options are:\n"
" -d[isplay] host:#.#\tX display & screen to use\n"
" -v[isual] #\t\tvisual to use\n"
" -g[eometry] WxH+X+Y\tlimits windows to this area\n"
" -m[aximum] WxH\t\tsize of maximized windows\n"
" -x\t\t\tmenu says Exit instead of logout\n"
" -bg color\t\tFrame color\n"
" -fg color\t\tLabel color\n"
" -bg2 color\t\tText field color\n"
" -c[ursor] #\t\tCursor number for root\n"
" -cfg color\t\tCursor color\n"
" -cbg color\t\tCursor outline color", program_version
);

	if(getenv("NOWARP"))	// If environmental variable NOWARP exists (value does not matter)
		DoNotWarp=1;		// Then keep your hands off of my mouse pointer.

#ifndef FL_NORMAL_SIZE // detect new versions of fltk where this is a variable
  FL_NORMAL_SIZE = 12;
#endif
  fl_open_display();
  color_setup(CURSOR_FG_SLOT, cfg, CURSOR_FG_COLOR<<8);
  color_setup(CURSOR_BG_SLOT, cbg, CURSOR_BG_COLOR<<8);
  Fl::set_color(FL_SELECTION_COLOR,0,0,128);
  Fl_Root root;
  Root = &root;
  Root->show(argc,argv); // fools fltk into using -geometry to set the size
  XSetErrorHandler(xerror_handler);
  initialize();
  return Fl::run();
}
