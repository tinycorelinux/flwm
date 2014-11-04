// Desktop.C

#include "config.h"

#if DESKTOPS

#include "Frame.H"
#include "Desktop.H"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Desktop* Desktop::current_ = 0;
Desktop* Desktop::first = 0;

// return the highest desktop number:
int Desktop::max_number() {
  int n = 0;
  for (Desktop* d = first; d; d = d->next)
    if (d->number_ > n) n = d->number_;
  return n;
}

// return an empty slot number:
int Desktop::available_number() {
  int n = 1;
  for (Desktop* d = first; d;) {
    if (d->number_ == n) {n++; d = first;}
    else d = d->next;
  }
  return n;
}

// these are set by main.C:
Atom _win_workspace;
Atom _win_workspace_count;
Atom _win_workspace_names;
#ifndef __sgi
static Atom kwm_current_desktop;
#endif
extern Fl_Window* Root;

static int dont_send;
static void send_desktops() {
  if (dont_send) return;
  int n = Desktop::max_number();
  setProperty(fl_xid(Root), _win_workspace_count, XA_CARDINAL, n);
  char buffer[1025];
  char* p = buffer;
  for (int i = 1; i <= n; i++) {
    Desktop* d = Desktop::number(i);
    const char* name = d ? d->name() : "<deleted>";
    while (p < buffer+1024 && *name) *p++ = *name++;
    *p++ = 0;
    if (p >= buffer+1024) break;
  }
  XChangeProperty(fl_display, fl_xid(Root), _win_workspace_names, XA_STRING,
		  8, PropModeReplace, (unsigned char *)buffer, p-buffer-1);
}

Desktop::Desktop(const char* n, int num) {
  next = first;
  first = this;
  name_ = strdup(n);
  number_ = num;
  send_desktops();
}

Desktop::~Desktop() {
  // remove from list:
  for (Desktop** p = &first; *p; p = &((*p)->next))
    if (*p == this) {*p = next; break;}
  send_desktops();
  if (current_ == this || !first->next) current(first);
  // put any clients onto another desktop:
  for (Frame* c = Frame::first; c; c = c->next)
    if (c->desktop() == this) c->desktop(first);
  free((void*)name_);
}

void Desktop::name(const char* l) {
  free((void*)name_);
  name_ = strdup(l);
}

void Desktop::current(Desktop* n) {
  if (n == current_) return;
  current_ = n;
  for (Frame* c = Frame::first; c; c = c->next) {
    if (c->desktop() == n) {
      if (c->state() == OTHER_DESKTOP) c->state(NORMAL);
    } else if (c->desktop()) {
      if (c->state() == NORMAL) c->state(OTHER_DESKTOP);
    }
  }
  if (n && !dont_send) {
#ifndef __sgi
    setProperty(fl_xid(Root), kwm_current_desktop, kwm_current_desktop, n->number());
#endif
    setProperty(fl_xid(Root), _win_workspace, XA_CARDINAL, n->number()-1);
  }
}

// return desktop with given number, create it if necessary:
Desktop* Desktop::number(int n, int create) {
  if (!n) return 0;
  Desktop* d;
  for (d = first; d; d = d->next) if (d->number() == n) return d;
  if (create) {
    char buf[20]; sprintf(buf, "Desktop %d", n);
    d = new Desktop(buf,n);
  }
  return d;
}

// called at startup, read the list of desktops from the root
// window properties, or on failure make some default desktops.
void init_desktops() {
  dont_send = 1;
  int length;
  char* buffer =
    (char*)getProperty(fl_xid(Root), _win_workspace_names, XA_STRING, &length);
  if (buffer) {
    char* c = buffer;
    for (int i = 1; c < buffer+length; i++) {
      char* d = c; while (*d) d++;
      if (*c != '<') new Desktop(c,i);
      c = d+1;
    }
    XFree(buffer);
  }
  int current_num = 0;
  int p = getIntProperty(fl_xid(Root), _win_workspace, XA_CARDINAL, -1);
  if (p >= 0 && p < 25) current_num = p+1;
#ifndef __sgi
  // SGI's Xlib barfs when you try to do this XInternAtom!
  // Maybe somebody there does not like KDE?
  kwm_current_desktop = XInternAtom(fl_display, "KWM_CURRENT_DESKTOP", 0);
  if (!current_num) {
    p = getIntProperty(fl_xid(Root), kwm_current_desktop, kwm_current_desktop);
    if (p > 0 && p < 25) current_num = p;
  }
#endif
  if (!current_num) current_num = 1;
  Desktop::current(Desktop::number(current_num, 1));
  dont_send = 0;
}

#endif
