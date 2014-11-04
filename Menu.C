// Menu.cxx

#include "config.h"
#include "Frame.H"
#if DESKTOPS
#include "Desktop.H"
#endif
#include <FL/Fl_Box.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "FrameWindow.H"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

// it is possible for the window to be deleted or withdrawn while
// the menu is up.  This will detect that case (with reasonable
// reliability):
static int
window_deleted(Frame* c)
{
  return c->state() != NORMAL
    && c->state() != ICONIC
    && c->state() != OTHER_DESKTOP;
}

static void
frame_callback(Fl_Widget*, void*d)
{
  Frame* c = (Frame*)d;
  if (window_deleted(c)) return;
  c->raise();
  c->activate(2);
}

#if DESKTOPS
// raise it but also put it on the current desktop:
static void
move_frame_callback(Fl_Widget*, void*d)
{
  Frame* c = (Frame*)d;
  if (window_deleted(c)) return;
  c->desktop(Desktop::current());
  c->raise();
  c->activate(2);
}
#endif

#define SCREEN_DX 1	// offset to corner of contents area
#define SCREEN_W (MENU_ICON_W-2)	// size of area to draw contents in
#define SCREEN_H (MENU_ICON_H-2)	// size of area to draw contents in

#define	MAX_NESTING_DEPTH	32

extern Fl_Window* Root;

#if FL_MAJOR_VERSION < 2
static void
frame_label_draw(const Fl_Label* o, int X, int Y, int W, int H, Fl_Align align)
{
  Frame* f = (Frame*)(o->value);
  if (window_deleted(f)) return;
  fl_draw_box(FL_THIN_DOWN_BOX, X, Y, MENU_ICON_W, MENU_ICON_H, FL_GRAY);
  for (Frame* c = Frame::first; c; c = c->next) {
    if (c->state() != UNMAPPED && (c==f || c->is_transient_for(f))) {
      int x = ((c->x()-Root->x())*SCREEN_W+Root->w()/2)/Root->w();
      int w = (c->w()*SCREEN_W+Root->w()-1)/Root->w();
      if (w > SCREEN_W) w = SCREEN_W;
      if (w < 3) w = 3;
      if (x+w > SCREEN_W) x = SCREEN_W-w;
      if (x < 0) x = 0;
      int y = ((c->y()-Root->y())*SCREEN_H+Root->h()/2)/Root->h();
      int h = (c->h()*SCREEN_H+Root->h()-1)/Root->h();
      if (h > SCREEN_H) h = SCREEN_H;
      if (h < 3) h = 3;
      if (y+h > SCREEN_H) y = SCREEN_H-h;
      if (y < 0) y = 0;
      fl_color(FL_FOREGROUND_COLOR);
      if (c->state() == ICONIC)
	fl_rect(X+x+SCREEN_DX, Y+y+SCREEN_DX, w, h);
      else
	fl_rectf(X+x+SCREEN_DX, Y+y+SCREEN_DX, w, h);
    }
  }
  fl_font(o->font, o->size);
  fl_color((Fl_Color)o->color);
  const char* l = f->label(); if (!l) l = "unnamed";
  // double any ampersands to turn off the underscores:
  char buf[256];
  if (strchr(l,'&')) {
    char* t = buf;
    while (t < buf+254 && *l) {
      if (*l=='&') *t++ = *l;
      *t++ = *l++;
    }
    *t = 0;
    l = buf;
  }
  fl_draw(l, X+MENU_ICON_W+3, Y, W-MENU_ICON_W-3, H, align);
}

static void
frame_label_measure(const Fl_Label* o, int& W, int& H)
{
  Frame* f = (Frame*)(o->value);
  if (window_deleted(f)) {W = MENU_ICON_W+3; H = MENU_ICON_H; return;}
  const char* l = f->label(); if (!l) l = "unnamed";
  fl_font(o->font, o->size);
  fl_measure(l, W, H);
  W += MENU_ICON_W+3;
  if (W > MAX_MENU_WIDTH) W = MAX_MENU_WIDTH;
  if (H < MENU_ICON_H) H = MENU_ICON_H;
}

// This labeltype is used for non-frame items so the text can line
// up with the icons:

static void
label_draw(const Fl_Label* o, int X, int Y, int W, int H, Fl_Align align)
{
  fl_font(o->font, o->size);
  fl_color((Fl_Color)o->color);
  fl_draw(o->value, X+MENU_ICON_W+3, Y, W-MENU_ICON_W-3, H, align);
}

static void
label_measure(const Fl_Label* o, int& W, int& H)
{
  fl_font(o->font, o->size);
  fl_measure(o->value, W, H);
  W += MENU_ICON_W+3;
  if (W > MAX_MENU_WIDTH) W = MAX_MENU_WIDTH;
  if (H < MENU_ICON_H) H = MENU_ICON_H;
}

#define FRAME_LABEL FL_FREE_LABELTYPE
#define TEXT_LABEL Fl_Labeltype(FL_FREE_LABELTYPE+1)

#endif // FL_MAJOR_VERSION < 2

////////////////////////////////////////////////////////////////

static void
cancel_cb(Fl_Widget* w, void*)
{
  w->window()->hide();
}

#if DESKTOPS

static void
desktop_cb(Fl_Widget*, void* v)
{
  Desktop::current((Desktop*)v);
}

static void
delete_desktop_cb(Fl_Widget*, void* v)
{
  delete (Desktop*)v;
}

#if ASK_FOR_NEW_DESKTOP_NAME

static Fl_Input* new_desktop_input = 0;

static void
new_desktop_ok_cb(Fl_Widget* w, void*)
{
  w->window()->hide();
  Desktop::current(new Desktop(new_desktop_input->value(), Desktop::available_number()));
}

static void
new_desktop_cb(Fl_Widget*, void*)
{
  if (!new_desktop_input) {
    FrameWindow* w = new FrameWindow(190,90);
    new_desktop_input = new Fl_Input(10,30,170,25,"New desktop name:");
    new_desktop_input->align(FL_ALIGN_TOP_LEFT);
    new_desktop_input->labelfont(FL_BOLD);
    Fl_Return_Button* b = new Fl_Return_Button(100,60,80,20,"OK");
    b->callback(new_desktop_ok_cb);
    Fl_Button* b2 = new Fl_Button(10,60,80,20,"Cancel");
    b2->callback(cancel_cb);
    w->set_non_modal();
    w->end();
  }
  char buf[120];
  sprintf(buf, "Desktop %d", Desktop::available_number());
  new_desktop_input->value(buf);
  new_desktop_input->window()->hotspot(new_desktop_input);
  new_desktop_input->window()->show();
}

#else // !ASK_FOR_NEW_DESKTOP_NAME

static void
new_desktop_cb(Fl_Widget*, void*)
{
  char buf[120];
  int i = Desktop::available_number();
  sprintf(buf, "Desktop %d", i);
  Desktop::current(new Desktop(buf, i));
}

#endif

#endif
////////////////////////////////////////////////////////////////

static void
exit_cb(Fl_Widget*, void*)
{
  printf("exit_cb\n");
  Frame::save_protocol();
  exit(0);
}

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

static void
logout_cb(Fl_Widget*, void*)
{
  int pid=0;
  if (( pid=fork()) == 0) {
    execlp("exittc","exittc", NULL);
  }
}

////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#if XTERM_MENU_ITEM || WMX_MENU_ITEMS

static const char* xtermname = "xterm";

static void
spawn_cb(Fl_Widget*, void*n)
{
  char* name = (char*)n;
  // strange code thieved from 9wm to avoid leaving zombies
  if (fork() == 0) {
    if (fork() == 0) {
      close(ConnectionNumber(fl_display));
      if (name == xtermname) execlp(name, name, "-ut", (void*)0);
      else execl(name, name, (void*)0);
      fprintf(stderr, "flwm: can't run %s, %s\n", name, strerror(errno));
      XBell(fl_display, 70);
      exit(1);
    }
    exit(0);
  }
  wait((int *) 0);
}

#endif

static Fl_Menu_Item other_menu_items[] = {
#if XTERM_MENU_ITEM
  {"New xterm", 0, spawn_cb, (void*)xtermname, 0, 0, 0, MENU_FONT_SIZE},
#endif
#if DESKTOPS
  {"New desktop", 0, new_desktop_cb, 0, 0, 0, 0, MENU_FONT_SIZE},
#endif
  {"Exit", 0, logout_cb, 0, 0, 0, 0, MENU_FONT_SIZE},
  {0}};
#define num_other_items (sizeof(other_menu_items)/sizeof(Fl_Menu_Item))

// use this to fill in a menu location:
static void
init(Fl_Menu_Item& m, const char* data)
{
#ifdef HAVE_STYLES
  m.style = 0;
#endif
  m.label(data);
#if FL_MAJOR_VERSION > 2
  m.flags = fltk::RAW_LABEL;
#else
  m.flags = 0;
#endif
  m.labeltype(FL_NORMAL_LABEL);
  m.shortcut(0);
  m.labelfont(MENU_FONT_SLOT);
  m.labelsize(MENU_FONT_SIZE);
  m.labelcolor(FL_FOREGROUND_COLOR);
}

#if WMX_MENU_ITEMS

// wmxlist is an array of char* pointers (for efficient sorting purposes),
// which are stored in wmxbuffer (for memory efficiency and to avoid
// freeing and fragmentation)
static char** wmxlist = NULL;
static int wmxlistsize = 0;
// wmx commands are read from ~/.wmx,
// they are stored null-separated here:
static char* wmxbuffer = NULL;
static int wmxbufsize = 0;
static int num_wmx = 0;
// ML ----------------------
// static time_t wmx_time = 0;
time_t wmx_time = 0;	// ML: made global
// ------------------------ML
static int wmx_pathlen = 0;

static int
scan_wmx_dir (char *path, int bufindex, int nest)
{
  DIR* dir = opendir(path);
  struct stat st;
  int pathlen = strlen (path);
  if (dir) {
    struct dirent* ent;
    while ((ent=readdir(dir))) {
    if (ent->d_name[0] == '.')
        continue;
      strcpy(path+pathlen, ent->d_name);
      if (stat(path, &st) < 0) continue;
      int len = pathlen+strlen(ent->d_name);
	// worst-case alloc needs
      if (bufindex+len+nest+1 > wmxbufsize)
	wmxbuffer = (char*)realloc(wmxbuffer, (wmxbufsize+=1024));
      for (int i=0; i<nest; i++)
	wmxbuffer[bufindex++] = '/'; // extra slash marks menu titles
      if (S_ISDIR(st.st_mode) && (st.st_mode & 0555) && nest<MAX_NESTING_DEPTH){
	strcpy(wmxbuffer+bufindex, path);
        bufindex += len+1;
        strcat(path, "/");
        bufindex = scan_wmx_dir (path, bufindex, nest+1);
	num_wmx++;
      } else if (S_ISREG(st.st_mode) && (st.st_mode & 0111)) {
	// make sure it exists and is an executable file:
	strcpy(wmxbuffer+bufindex, path);
	bufindex += len+1;
	num_wmx++;
      }
    }
    closedir(dir);
  }
  return bufindex;
}

// comparison for qsort
//	We keep submenus together by noting that they're proper superstrings
static int
wmxCompare(const void *A, const void *B)
{
  char	*pA, *pB;
  pA = *(char **)A;
  pB = *(char **)B;

  pA += strspn(pA, "/");
  pB += strspn(pB, "/");

  // caseless compare
  while (*pA && *pB) {
    if (toupper(*pA) > toupper(*pB))
      return(1);
    if (toupper(*pA) < toupper(*pB))
      return(-1);
    pA++;
    pB++;
  }
  if (*pA)
    return(1);
  if (*pB)
    return(-1);
  return(0);
}

static void
load_wmx()
{
  const char* home=getenv("HOME"); if (!home) home = ".";
  char path[1024];
  strcpy(path, home);
  if (path[strlen(path)-1] != '/') strcat(path, "/");
  strcat(path, ".wmx/");
  struct stat st; if (stat(path, &st) < 0) return;
  if (st.st_mtime == wmx_time) return;
  wmx_time = st.st_mtime;
  num_wmx = 0;
  wmx_pathlen = strlen(path);
  scan_wmx_dir(path, 0, 0);

  // Build wmxlist
  if (num_wmx > wmxlistsize) {
    if (wmxlist)
      delete [] wmxlist;
    wmxlist = new char *[num_wmx];
    wmxlistsize = num_wmx;
  }
  for (int i=0; i<num_wmx; i++) {
    char* cmd = wmxbuffer;

    for (int j = 0; j < num_wmx; j++) {
      wmxlist[j] = cmd;
      cmd += strlen(cmd)+1;
    }
  }

  qsort(wmxlist, num_wmx, sizeof(char *), wmxCompare);
}

#endif

////////////////////////////////////////////////////////////////

int exit_flag; // set by the -x switch

static int is_active_frame(Frame* c) {
  for (Frame* a = Frame::activeFrame(); a; a = a->transient_for())
    if (a == c) return 1;
  return 0;
}

void
ShowTabMenu(int tab)
{

  static char beenhere;
  if (!beenhere) {
    beenhere = 1;
#if FL_MAJOR_VERSION < 2
    Fl::set_labeltype(FRAME_LABEL, frame_label_draw, frame_label_measure);
    Fl::set_labeltype(TEXT_LABEL, label_draw, label_measure);
#endif
    if (exit_flag) {
      Fl_Menu_Item* m = other_menu_items+num_other_items-2;
      m->label("Exit");
      m->callback(exit_cb);
    }
  }

  static Fl_Menu_Item* menu = 0;
  static int arraysize = 0;

#if DESKTOPS
  int one_desktop = !Desktop::first->next;
#endif

  // count up how many items are on the menu:

  int n = num_other_items;
#if WMX_MENU_ITEMS
  load_wmx();
  if (num_wmx) {
    n -= 1; // delete "new xterm"
    // add wmx items
    int	level = 0;
    for (int i=0; i<num_wmx; i++) {
      int nextlev = (i==num_wmx-1)?0:strspn(wmxlist[i+1], "/")-1;
      if (nextlev < level) {
	n += level-nextlev;
	level = nextlev;
      } else if (nextlev > level)
	level++;
      n++;
    }
  }
#endif

#if DESKTOPS
  // count number of items per desktop in these variables:
  int numsticky = 0;
  Desktop* d;
  for (d = Desktop::first; d; d = d->next) d->junk = 0;
#endif

  // every frame contributes 1 item:
  Frame* c;
  for (c = Frame::first; c; c = c->next) {
    if (c->state() == UNMAPPED || c->transient_for()) continue;
#if DESKTOPS
    if (!c->desktop()) {
      numsticky++;
    } else {
      c->desktop()->junk++;
    }
#endif
    n++;
  }

#if DESKTOPS
  if (!one_desktop) {
    // add the sticky "desktop":
    n += 2; if (!numsticky) n++;
    if (Desktop::current()) {
      n += numsticky;
      Desktop::current()->junk += numsticky;
    }
    // every desktop contributes menu title, null terminator,
    // and possible delete:
    for (d = Desktop::first; d; d = d->next) {
      n += 2; if (!d->junk) n++;
    }
  }
#endif

  if (n > arraysize) {
    delete[] menu;
    menu = new Fl_Menu_Item[arraysize = n];
  }

  // build the menu:
  n = 0;
  const Fl_Menu_Item* preset = 0;
  const Fl_Menu_Item* first_on_desk = 0;
#if DESKTOPS
  if (one_desktop) {
#endif
    for (c = Frame::first; c; c = c->next) {
      if (c->state() == UNMAPPED || c->transient_for()) continue;
#if FL_MAJOR_VERSION < 2
      init(menu[n],(char*)c);
      menu[n].labeltype(FRAME_LABEL);
#else
      init(menu[n],c->label());
#endif
      menu[n].callback(frame_callback, c);
      if (is_active_frame(c)) preset = menu+n;
      n++;
    }
    if (n > 0) first_on_desk = menu;
#if DESKTOPS
  } else for (d = Desktop::first; ; d = d->next) {
    // this loop adds the "sticky" desktop last, when d==0
    if (d == Desktop::current()) preset = menu+n;
    init(menu[n], d ? d->name() : "Sticky");
    menu[n].callback(desktop_cb, d);
    menu[n].flags = FL_SUBMENU;
    n++;
    if (d && !d->junk) {
      init(menu[n],"delete this desktop");
      menu[n].callback(delete_desktop_cb, d);
      n++;
    } else if (!d && !numsticky) {
      init(menu[n],"(empty)");
      menu[n].callback_ = 0;
      menu[n].deactivate();
      n++;
    } else {
      if (d == Desktop::current()) first_on_desk = menu+n;
      for (c = Frame::first; c; c = c->next) {
	if (c->state() == UNMAPPED || c->transient_for()) continue;
	if (c->desktop() == d || !c->desktop() && d == Desktop::current()) {
	  init(menu[n],(char*)c);
#if FL_MAJOR_VERSION < 2
	  init(menu[n],(char*)c);
	  menu[n].labeltype(FRAME_LABEL);
#else
	  init(menu[n],c->label());
#endif
	  menu[n].callback(d == Desktop::current() ?
			   frame_callback : move_frame_callback, c);
	  if (d == Desktop::current() && is_active_frame(c)) preset = menu+n;
	  n++;
	}
      }
    }
    menu[n].label(0); n++; // terminator for submenu
    if (!d) break;
  }
#endif

  // For ALT+Tab, move the selection forward or backward:
  if (tab > 0 && first_on_desk) {
    if (!preset)
      preset = first_on_desk;
    else {
      preset++;
      if (!preset->label() || preset->callback_ != frame_callback)
	preset = first_on_desk;
    }
  } else if (tab < 0 && first_on_desk) {
    if (preset && preset != first_on_desk)
      preset--;
    else {
      // go to end of menu
      preset = first_on_desk;
      while (preset[1].label() && preset[1].callback_ == frame_callback)
	preset++;
    }
  }

#if WMX_MENU_ITEMS
  // put wmx-style commands above that:
  if (num_wmx > 0) {
    char* cmd;
    int pathlen[MAX_NESTING_DEPTH];
    int level = 0;
    pathlen[0] = wmx_pathlen;
    for (int i = 0; i < num_wmx; i++) {
      cmd = wmxlist[i];
      cmd += strspn(cmd, "/")-1;
      init(menu[n], cmd+pathlen[level]);
#if FL_MAJOR_VERSION < 2
#if DESKTOPS
      if (one_desktop)
#endif
	if (!level)
	  menu[n].labeltype(TEXT_LABEL);
#endif
      int nextlev = (i==num_wmx-1)?0:strspn(wmxlist[i+1], "/")-1;
      if (nextlev < level) {
	menu[n].callback(spawn_cb, cmd);
	// Close 'em off
	for (; level>nextlev; level--)
	  init(menu[++n], 0);
      } else if (nextlev > level) {
	// This should be made a submenu
	pathlen[++level] = strlen(cmd)+1; // extra for next trailing /
	menu[n].flags = FL_SUBMENU;
	menu[n].callback((Fl_Callback*)0);
      } else {
	menu[n].callback(spawn_cb, cmd);
      }
      n++;
    }
  }

  // put the fixed menu items at the bottom:
#if XTERM_MENU_ITEM
  if (num_wmx) // if wmx commands, delete the built-in xterm item:
    memcpy(menu+n, other_menu_items+1, sizeof(other_menu_items)-sizeof(Fl_Menu_Item));
  else
#endif
#endif
    memcpy(menu+n, other_menu_items, sizeof(other_menu_items));
#if FL_MAJOR_VERSION < 2
#if DESKTOPS
  if (one_desktop)
#endif
    // fix the menus items so they are indented to align with window names:
    while (menu[n].label()) menu[n++].labeltype(TEXT_LABEL);
#endif

  const Fl_Menu_Item* picked =
    menu->popup(Fl::event_x(), Fl::event_y(), 0, preset);
#if FL_MAJOR_VERSION < 2
  if (picked && picked->callback()) picked->do_callback(0);
#endif
}

void ShowMenu() {ShowTabMenu(0);}
