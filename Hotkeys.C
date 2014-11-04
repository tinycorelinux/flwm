// Hotkeys.C

// If you want to change what the hotkeys are, see the table at the bottom!

#include "config.h"
#include "Frame.H"
#include "Desktop.H"
#include <stdio.h>

extern void ShowMenu();
extern void ShowTabMenu(int tab);

#if STANDARD_HOTKEYS

static void NextWindow() { // Alt+Tab
  ShowTabMenu(1);
}

static void PreviousWindow() { // Alt+Shift+Tab
  ShowTabMenu(-1);
}

#endif

#if DESKTOPS

static void NextDesk() {
  if (Desktop::current()) {
    Desktop::current(Desktop::current()->next?
		     Desktop::current()->next:Desktop::first);
  } else {
    Desktop::current(Desktop::first);
  }
}

static void PreviousDesk() {
  Desktop* search=Desktop::first;
  while (search->next && search->next!=Desktop::current()){
    search=search->next;
  }
  Desktop::current(search);
}

// warning: this assummes it is bound to Fn key:
static void DeskNumber() {
  Desktop::current(Desktop::number(Fl::event_key()-FL_F, 1));
}
#endif

#if WMX_HOTKEYS || CDE_HOTKEYS

static void Raise() { // Alt+Up
  Frame* f = Frame::activeFrame();
  if (f) f->raise();
}

static void Lower() { // Alt+Down
  Frame* f = Frame::activeFrame();
  if (f) f->lower();
}

static void Iconize() { // Alt+F1
  Frame* f = Frame::activeFrame();
  if (f) f->iconize();
  else ShowMenu(); // so they can deiconize stuff
}

static void Close() { // Alt+Delete
  Frame* f = Frame::activeFrame();
  if (f) f->close();
}

#endif

#ifdef ML_HOTKEYS
static void MoveFrame(int xbump, int ybump) {
 int xincr, yincr, nx, ny, xspace, yspace;
  Frame* f = Frame::activeFrame();
  if (f) {
	  xspace = Fl::w() - f->w();
	  xincr = xspace / 20;
	  if (xincr < 4) {
		  xincr = 4;
	  }
	  nx = f->x() + (xbump * xincr);
	  if (nx < 0) nx = 0;
	  if (nx > xspace) nx = xspace;

	  yspace = Fl::h() - f->h();
	  yincr = yspace / 20;
	  if (yincr < 4) {
		  yincr = 4;
	  }
	  ny = f->y() + (ybump * yincr);
	  if (ny < 0) ny = 0;
	  if (ny > yspace) ny = yspace;

	  f->set_size(nx, ny, f->w(), f->h());
  }
}

static void MoveLeft(void) { // Ctrl+Alt+Left
	MoveFrame(-1, 0);
}

static void MoveUp(void) { // Ctrl+Alt+Up
	MoveFrame(0, -1);
}

static void MoveRight(void) { // Ctrl+Alt+Right
	MoveFrame(+1, 0);
}

static void MoveDown(void) { // Ctrl+Alt+Down
	MoveFrame(0, +1);
}
static void GrowFrame(int wbump, int hbump) {
  int wincr, hincr, nx, ny, nw, nh, xspace, yspace;
  Frame* f = Frame::activeFrame();
  if (f) {
	  int minw = 8 * BUTTON_W;
	  int minh = 4 * BUTTON_H;
	  nx = f->x();
	  ny = f->y();
	  nw = f->w();
	  nh = f->h();
	  if (wbump != 0 && f->w() >= minw) {
		  nw +=  wbump * 32;
		  if (nw < minw) nw = minw;
		  if (nw > Fl::w()) nw = Fl::w();
		  if (nx + nw > Fl::w())
			nx = Fl::w() - nw;
	  }

	  if (hbump != 0 && f->h() >= minh) {
		  nh += hbump * 32;
		  if (nh < minh) nh = minh;
		  if (nh > Fl::h()) nh = Fl::h();
		  ny = f->y();
		  if (ny + nh > Fl::h())
			ny = Fl::h() - nh;
		  else
		    ny = f->y() + f->h() - nh;
	  }

	  f->set_size(nx, ny, nw, nh);
  }
}

static void GrowBigger(void) { // Ctrl+Alt+Plus
	GrowFrame(+1, +1);
}

static void GrowSmaller(void) { // Ctrl+Alt+Minus
	GrowFrame(-1, -1);
}

static void GrowWider(void) { // Ctrl+Alt+Gt.Than
	GrowFrame(+1, 0);
}

static void GrowNarrower(void) { // Ctrl+Alt+LessThan
	GrowFrame(-1, 0);
}

static void GrowTaller(void) { // Ctrl+Alt+PageUp
	GrowFrame(0, +1);
}

static void GrowShorter(void) { // Ctrl+Alt+PageDn
	GrowFrame(0, -1);
}

static void ToggleVertMax(void) {// Ctrl+Alt+V
	static int nonmax_h = Fl::h() - 64;
	static int nonmax_y = 32;
	Frame* f = Frame::activeFrame();

	if (f->h() < Fl::h() - 16) {
		nonmax_h = f->h();
		nonmax_y = f->y();
		f->set_size(f->x(), 0, f->w(), Fl::h());
	}
	else {
		f->set_size(f->x(), nonmax_y, f->w(), nonmax_h);
	}
}

static void ToggleHorzMax(void) {// Ctrl+Alt+H
	static int nonmax_w = Fl::w() - 64;
	static int nonmax_x = 32;
	Frame* f = Frame::activeFrame();

	if (f->w() < Fl::w() - 16) {
		nonmax_w = f->w();
		nonmax_x = f->x();
		f->set_size(0, f->y(), Fl::w(), f->h());
	}
	else {
		f->set_size(nonmax_x, f->y(), nonmax_w, f->h());
	}
}

static void ToggleWinMax(void) {// Ctrl+Alt+M
	Frame* f = Frame::activeFrame();
	int is_hmax = -1;
	int is_vmax = -1;
	if (f->w() > Fl::w() - 16) is_hmax = 1;
	if (f->h() > Fl::h() - 16) is_vmax = 1;

	if ((is_hmax * is_vmax) > 0 || is_hmax > 0) ToggleVertMax();
	if ((is_hmax * is_vmax) > 0 || is_vmax > 0) ToggleHorzMax();
}
#endif

////////////////////////////////////////////////////////////////

static struct {int key; void (*func)();} keybindings[] = {
#if STANDARD_HOTKEYS || MINIMUM_HOTKEYS
  // these are very common and tend not to conflict, due to Windoze:
  {FL_ALT+FL_Escape,	ShowMenu},
  {FL_ALT+FL_Menu,	ShowMenu},
#endif
#if STANDARD_HOTKEYS
  {FL_ALT+FL_Tab,	NextWindow},
  {FL_ALT+FL_SHIFT+FL_Tab,PreviousWindow},
  {FL_ALT+FL_SHIFT+0xfe20,PreviousWindow}, // XK_ISO_Left_Tab
#endif
#if KWM_HOTKEYS && DESKTOPS // KWM uses these to switch desktops
//   {FL_CTRL+FL_Tab,	NextDesk},
//   {FL_CTRL+FL_SHIFT+FL_Tab,PreviousDesk},
//   {FL_CTRL+FL_SHIFT+0xfe20,PreviousDesk}, // XK_ISO_Left_Tab
  {FL_CTRL+FL_F+1,	DeskNumber},
  {FL_CTRL+FL_F+2,	DeskNumber},
  {FL_CTRL+FL_F+3,	DeskNumber},
  {FL_CTRL+FL_F+4,	DeskNumber},
  {FL_CTRL+FL_F+5,	DeskNumber},
  {FL_CTRL+FL_F+6,	DeskNumber},
  {FL_CTRL+FL_F+7,	DeskNumber},
  {FL_CTRL+FL_F+8,	DeskNumber},
  {FL_CTRL+FL_F+9,	DeskNumber},
  {FL_CTRL+FL_F+10,	DeskNumber},
  {FL_CTRL+FL_F+11,	DeskNumber},
  {FL_CTRL+FL_F+12,	DeskNumber},
#endif
#if WMX_HOTKEYS
  // wmx also sets all these, they seem pretty useful:
  {FL_ALT+FL_Up,	Raise},
  {FL_ALT+FL_Down,	Lower},
//{FL_ALT+FL_Enter,	Iconize},
  {FL_ALT+FL_F+1,	Iconize},
  {FL_ALT+FL_Delete,	Close},
//{FL_ALT+FL_Page_Up,	ToggleMaxH},
//{FL_ALT+FL_Page_Down, ToggleMaxW},
#endif
#if WMX_DESK_HOTKEYS && DESKTOPS
  // these wmx keys are not set by default as they break NetScape:
  {FL_ALT+FL_Left,	PreviousDesk},
  {FL_ALT+FL_Right,	NextDesk},
#endif
#if CDE_HOTKEYS
  // CDE hotkeys (or at least what SGI's 4DWM uses):
//{FL_ALT+FL_F+1,	Raise}, // used above to iconize
//{FL_ALT+FL_F+2,	unknown}, // KWM uses this to run a typed-in command
  {FL_ALT+FL_F+3,	Lower},
  {FL_ALT+FL_F+4,	Close}, // this matches KWM
//{FL_ALT+FL_F+5,	Restore}, // useless because no icons visible
//{FL_ALT+FL_F+6,	unknown}, // ?
//{FL_ALT+FL_F+7,	Move}, // grabs the window for movement
//{FL_ALT+FL_F+8,	Resize}, // grabs the window for resizing
  {FL_ALT+FL_F+9,	Iconize},
//{FL_ALT+FL_F+10,	Maximize},
//{FL_ALT+FL_F+11,	unknown}, // ?
  {FL_ALT+FL_F+12,	Close}, // actually does "quit"
#else
#if DESKTOPS && DESKTOP_HOTKEYS
  // seem to be common to Linux window managers
  {FL_ALT+FL_F+1,	DeskNumber},
  {FL_ALT+FL_F+2,	DeskNumber},
  {FL_ALT+FL_F+3,	DeskNumber},
  {FL_ALT+FL_F+4,	DeskNumber},
  {FL_ALT+FL_F+5,	DeskNumber},
  {FL_ALT+FL_F+6,	DeskNumber},
  {FL_ALT+FL_F+7,	DeskNumber},
  {FL_ALT+FL_F+8,	DeskNumber},
  {FL_ALT+FL_F+9,	DeskNumber},
  {FL_ALT+FL_F+10,	DeskNumber},
  {FL_ALT+FL_F+11,	DeskNumber},
  {FL_ALT+FL_F+12,	DeskNumber},
#endif
#endif
#ifdef ML_HOTKEYS
  {FL_CTRL+FL_ALT+FL_Left,		MoveLeft},
  {FL_CTRL+FL_ALT+FL_Up,		MoveUp},
  {FL_CTRL+FL_ALT+FL_Right,	MoveRight},
  {FL_CTRL+FL_ALT+FL_Down,	MoveDown},
  {FL_CTRL+FL_ALT+'=',	GrowBigger},		// = is also + key
  {FL_CTRL+FL_ALT+'-',	GrowSmaller},
  {FL_CTRL+FL_ALT+'.',	GrowWider},		// . is also > key
  {FL_CTRL+FL_ALT+',',	GrowNarrower}, // , is also < key
  {FL_CTRL+FL_ALT+FL_Page_Up,	GrowTaller},
  {FL_CTRL+FL_ALT+FL_Page_Down,	GrowShorter},
  {FL_CTRL+FL_ALT+'t',	GrowTaller},
  {FL_CTRL+FL_ALT+'s',	GrowShorter},
  {FL_CTRL+FL_ALT+'v',	ToggleVertMax},
  {FL_CTRL+FL_ALT+'h',	ToggleHorzMax},
  {FL_CTRL+FL_ALT+'m',	ToggleWinMax},
#endif
  {0}};

#if FL_MAJOR_VERSION > 1
// Define missing function, this should get put in fltk2.0:
namespace fltk {
int test_shortcut(int shortcut) {
  if (!shortcut) return 0;

  int shift = Fl::event_state();
  // see if any required shift flags are off:
  if ((shortcut&shift) != (shortcut&0x7fff0000)) return 0;
  // record shift flags that are wrong:
  int mismatch = (shortcut^shift)&0x7fff0000;
  // these three must always be correct:
  if (mismatch&(FL_META|FL_ALT|FL_CTRL)) return 0;

  int key = shortcut & 0xffff;

  // if shift is also correct, check for exactly equal keysyms:
  if (!(mismatch&(FL_SHIFT)) && unsigned(key) == Fl::event_key()) return 1;

  // try matching ascii, ignore shift:
  if (key == event_text()[0]) return 1;

  // kludge so that Ctrl+'_' works (as opposed to Ctrl+'^_'):
  if ((shift&FL_CTRL) && key >= 0x3f && key <= 0x5F
      && event_text()[0]==(key^0x40)) return 1;
  return 0;
}
}
#endif

int Handle_Hotkey() {
  for (int i = 0; keybindings[i].key; i++) {
    if (Fl::test_shortcut(keybindings[i].key) ||
	(keybindings[i].key & 0xFFFF) == FL_Delete
	&& Fl::event_key() == FL_BackSpace// fltk bug?
	) {
      keybindings[i].func();
      return 1;
    }
  }
  return 0;
}

extern Fl_Window* Root;

void Grab_Hotkeys() {
  XWindow root = fl_xid(Root);
  for (int i = 0; keybindings[i].key; i++) {
    int k = keybindings[i].key;
    int keycode = XKeysymToKeycode(fl_display, k & 0xFFFF);
    if (!keycode) continue;
    // Silly X!  we need to ignore caps lock & numlock keys by grabbing
    // all the combinations:
    XGrabKey(fl_display, keycode, k>>16,     root, 0, 1, 1);
    XGrabKey(fl_display, keycode, (k>>16)|2, root, 0, 1, 1); // CapsLock
    XGrabKey(fl_display, keycode, (k>>16)|16, root, 0, 1, 1); // NumLock
    XGrabKey(fl_display, keycode, (k>>16)|18, root, 0, 1, 1); // both
  }
}

