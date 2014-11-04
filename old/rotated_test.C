// Test the xvertext routines for rotated text

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_draw.H>

////////////////////////////////////////////////////////////////

#include "Rotated.H"

class RotText : public Fl_Box {
  void draw();
public:
  RotText(int X, int Y, int W, int H, const char* L = 0) :
    Fl_Box(X,Y,W,H,L) {}
};

void RotText::draw() {
  draw_box();
  fl_color(FL_BLACK);
  fl_font(labelfont(), labelsize());
  draw_rotated90(label(), x(), y(), w(), h(), align());
}

////////////////////////////////////////////////////////////////

Fl_Toggle_Button *leftb,*rightb,*topb,*bottomb,*insideb,*clipb,*wrapb;
RotText *text;
Fl_Input *input;
Fl_Hor_Value_Slider *fonts;
Fl_Hor_Value_Slider *sizes;
Fl_Double_Window *window;

void button_cb(Fl_Widget *,void *) {
  int i = 0;
  if (leftb->value()) i |= FL_ALIGN_LEFT;
  if (rightb->value()) i |= FL_ALIGN_RIGHT;
  if (topb->value()) i |= FL_ALIGN_TOP;
  if (bottomb->value()) i |= FL_ALIGN_BOTTOM;
  if (insideb->value()) i |= FL_ALIGN_INSIDE;
  if (clipb->value()) i |= FL_ALIGN_CLIP;
  if (wrapb->value()) i |= FL_ALIGN_WRAP;
  text->align(i);
  window->redraw();
}

void font_cb(Fl_Widget *,void *) {
  text->labelfont(int(fonts->value()));
  window->redraw();
}

void size_cb(Fl_Widget *,void *) {
  text->labelsize(int(sizes->value()));
  window->redraw();
}

void input_cb(Fl_Widget *,void *) {
  text->label(input->value());
  window->redraw();
}

int main(int argc, char **argv) {
  window = new Fl_Double_Window(400,400);

  input = new Fl_Input(50,0,350,25);
  input->static_value("The quick brown fox jumped over the lazy dog.");
  input->when(FL_WHEN_CHANGED);
  input->callback(input_cb);

  sizes= new Fl_Hor_Value_Slider(50,25,350,25,"Size:");
  sizes->align(FL_ALIGN_LEFT);
  sizes->bounds(1,64);
  sizes->step(1);
  sizes->value(14);
  sizes->callback(size_cb);

  fonts=new Fl_Hor_Value_Slider(50,50,350,25,"Font:");
  fonts->align(FL_ALIGN_LEFT);
  fonts->bounds(0,15);
  fonts->step(1);
  fonts->value(0);
  fonts->callback(font_cb);

  Fl_Group *g = new Fl_Group(0,0,0,0);
  leftb = new Fl_Toggle_Button(50,75,50,25,"left");
  leftb->callback(button_cb);
  rightb = new Fl_Toggle_Button(100,75,50,25,"right");
  rightb->callback(button_cb);
  topb = new Fl_Toggle_Button(150,75,50,25,"top");
  topb->callback(button_cb);
  bottomb = new Fl_Toggle_Button(200,75,50,25,"bottom");
  bottomb->callback(button_cb);
  insideb = new Fl_Toggle_Button(250,75,50,25,"inside");
  insideb->callback(button_cb);
  wrapb = new Fl_Toggle_Button(300,75,50,25,"wrap");
  wrapb->callback(button_cb);
  clipb = new Fl_Toggle_Button(350,75,50,25,"clip");
  clipb->callback(button_cb);
  g->resizable(insideb);
  g->forms_end();

  text= new RotText(100,225,200,100,input->value());
  text->box(FL_FRAME_BOX);
  text->align(FL_ALIGN_CENTER);
  window->resizable(text);
  window->forms_end();
  window->show(argc,argv);
  return Fl::run();
}

//
// End of "$Id: rotated_test.C,v 1.1 2000/01/18 01:05:49 spitzak Exp $".
//
