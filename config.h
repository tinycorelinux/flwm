/* flwm - config.h

   You can edit these symbols to change the behavior & appearance of flwm.
   Turning off features will make flwm smaller too!

   CHANGES
     20160410 - add WMX_DESK_METAKEYS as altenative to WMX_DESK_HOTKEYS;
       dentonlt
	 20160411 - add RESIZE_ANCHOR_TOPLEFT; dentonlt
	 20160506 - add EXTENDED_MOVEMENT_KEYS; dentonlt
*/

//ML Start---

// ML: My hotkeys (comment out to disable):
//	 CTRL+ALT+arrow keys: move window that direction 4 or more pixels
#define ML_HOTKEYS 1

// ML: Use environment variable to set titlebar color:
#define ML_TITLEBAR_COLOR 1

//-- End ML

// BEHAVIOR:

// Turn this on for click-to-type (rather than point-to-type).
// On: clicking on the window gives it focus
// Off: pointing at the window gives it the focus.
#define CLICK_TO_TYPE 1

// Most window managers consider this and click-to-type equivalent.
// On: clicking anywhere on window raises it
// Off: you have to click on the border to raise window
#define CLICK_RAISES CLICK_TO_TYPE

// For point-to-type, sticky focus means you don't lose the focus
// until you move the cursor to another window that wants focus.
// If this is off you lose focus as soon as the cursor goes outside
// the window (such as to the desktop):
// #define STICKY_FOCUS 1

// For point-to-type, after this many seconds the window is raised,
// nothing is done if this is not defined:
// #define AUTO_RAISE 0.5

// Perform "smart" autoplacement.
// New windows are put at positions where they cover as few existing windows
// as possible. A brute force algorithm is used, so it consumes quite a bit
// of CPU time.
#define SMART_PLACEMENT 1

// set this to zero to remove the multiple-desktop code.  This will
// make flwm about 20K smaller
#define DESKTOPS 1

// set this to zero for "new desktop" to just create one without asking
// for a name.  This saves 12K or so of fltk input field code:
#define ASK_FOR_NEW_DESKTOP_NAME 0

// wm2 has no close box, for good reasons.  Unfortunately too many programs
// assume there is one and have no other way to close a window.  For a more
// "pure" implementation set this to zero:
#define CLOSE_BOX 1

// set this to zero to remove the minimize/window-shade button:
#define MINIMIZE_BOX 1

// If this is false the minimize button only changes the width of the
// window, so it acts like a Mac window shade.  This was the original
// behavior, the new version makes it smaller vertically so it is just
// big enough to show the window title:
#define MINIMIZE_HEIGHT 1

// When using keystrokes to adjusting window width, flwm extends the
// window on its right side. By default, vertical changes stretch the
// window top (bottom left corner of window is anchored). To make flwm
// stretch downward instead (anchor top left corner), set this value:
// #define RESIZE_ANCHOR_TOPLEFT 1

// Read links from ~/.wmx to make menu items to run programs:
#define WMX_MENU_ITEMS 1

// Menu item to run a new xterm (if no wmx items found):
#define XTERM_MENU_ITEM 1

// Hotkeys (see Hotkeys.C for exactly what they do):
#define STANDARD_HOTKEYS 1 // alt+esc, alt+tab, alt+shift+tab
#define KWM_HOTKEYS 1	// ctrl+tab and ctrl+Fn for desktop switching
#define CDE_HOTKEYS defined(__sgi) // alt+fn do actions like raise/lower/close
#define WMX_HOTKEYS 1	// alt+up/down/enter/delete

// Extended Movement Keys: press ctrl+alt+shift+[arrow/enter] to send
// window to max left/right/up/down or center
// #define EXTENDED_MOVEMENT_KEYS 1

// enable one of these two for prev/next desktop switching. If both are
// set true here, only WMX_DESK_HOTKEYS will be implemented.
#define WMX_DESK_HOTKEYS 0 // alt+left/right (conflict with Netscape)
#define WMX_DESK_METAKEYS 0 // meta+left/right for prev/next desktop

#define DESKTOP_HOTKEYS 0 // alt+fn goes to desktop n

////////////////////////////////////////////////////////////////
// APPEARANCE:

// Color for active window title bar (also for selected menu items):
// If not defined, no active window title highlighting is done.
#if CLICK_TO_TYPE
#define ACTIVE_COLOR 0xF0F0F0
#endif
//#define ACTIVE_COLOR 0x000008

// thickness of the border edge on each side (includes XBORDER):
#define LEFT 3
#define RIGHT 4
#define TOP 3
#define BOTTOM 4

// font for titles (if not set, helvetica bold is used):
// If this name is specific enough the font size is ignored.
// #define TITLE_FONT "-*-helvetica-medium-r-normal--*"
#define TITLE_FONT_SIZE 12

// thickness of title bar (frame border thickness is added to it):
#define TITLE_WIDTH (TITLE_FONT_SIZE+1)
#ifdef TOPSIDE
#define TITLE_HEIGHT TITLE_WIDTH	//ML
#endif

// size & position of buttons (must fit in title bar):
#ifdef TOPSIDE
#define BUTTON_W 		TITLE_HEIGHT
#define BUTTON_H 		BUTTON_W
#define BUTTON_LEFT 	LEFT
#define BUTTON_TOP 		TOP
#define BUTTON_RIGHT 	RIGHT
#else
#define BUTTON_W 		TITLE_WIDTH
#define BUTTON_H 		BUTTON_W
#define BUTTON_LEFT 	LEFT
#define BUTTON_TOP 		TOP
#define BUTTON_BOTTOM 	BOTTOM
#endif

// how many pixels from edge for resize handle:
#define RESIZE_EDGE 5
// set this to zero to disable resizing by grabbing left/top edge:
//#define RESIZE_LEFT 1
#define RESIZE_TITLE_EDGE 1		// ML

// must drag window this far off screen to snap the border off the screen:
#define EDGE_SNAP 50
// must drag window this far off screen to actually move it off screen:
#define SCREEN_SNAP 100

// button decorations:
#define CLOSE_X 1	// windoze-style X in close button
#define CLOSE_HITTITE_LIGHTNING 0 // The ancient Hittite symbol for lightning
#define ICONIZE_BOX 1	// small box in iconize button
#define MINIMIZE_ARROW 1 // minimize button draws a <- rather than |

// default colors for cursor:
#ifdef __sgi
#define CURSOR_FG_COLOR 0xff0000
#else
#define CURSOR_FG_COLOR 0x000000
#endif
#define CURSOR_BG_COLOR 0xffffff

// "Clock in the title bar" code contributed by Kevin Quick
// <kquick@iphase.com>:

// Add a clock to the active window's title bar using specified
// strftime fmt Note: in keeping with the minimalistic, fast, and
// small philosophy of the flwm, the clock will only be updated
// once/minute so any display of seconds is frivolous.
//#define SHOW_CLOCK "%I:%M %p %Z"

// We also support the concept of a clock alarm.  The alarm is
// triggered by delivering SIGALRM to flwm and cleared by delivering
// SIGCONT to flwm.  When the alarm is active, the foreground and
// background colors of the clock display are determined by the
// following settings.  (The following are unused if SHOW_CLOCK is not
// defined).
#define ALARM_FG_COLOR 0x00ffff
#define ALARM_BG_COLOR 0xff0000

////////////////////////////////////////////////////////////////
// MENU APPEARANCE:

#define MAX_MENU_WIDTH 300

// size of the little pictures in the menu:
#define MENU_ICON_W 18
#define MENU_ICON_H 15

// font to use in menus (if not set helvetica is used):
#define MENU_FONT "-*-helvetica-medium-r-normal--*"
#define MENU_FONT_SIZE 12

////////////////////////////////////////////////////////////////
// You probably don't want to change any of these:

#ifdef TITLE_FONT
#define TITLE_FONT_SLOT FL_FREE_FONT
#else
#define TITLE_FONT_SLOT FL_BOLD
#endif

#ifdef MENU_FONT
#define MENU_FONT_SLOT Fl_Font(FL_FREE_FONT+1)
#else
#define MENU_FONT_SLOT FL_HELVETICA
#endif

#define CURSOR_BG_SLOT Fl_Color(30)
#define CURSOR_FG_SLOT Fl_Color(31)
