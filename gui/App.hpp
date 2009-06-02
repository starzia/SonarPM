#pragma once
#include <wx/wx.h>
#include "Frame.hpp"

//====================== FORWARD DECLARATIONS ===============================
// The HelloWorldApp class. This class shows a window
// containing a statusbar with the text "Hello World"
class App : public wxApp{
 public:
  virtual bool OnInit();
  virtual int OnExit();
private:
  Frame* frame;
};
 
DECLARE_APP(App)

//====================== IMPLEMENTATIONS ====================================
// generate the main() entry point
IMPLEMENT_APP(App)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool App::OnInit(){
  this->frame = new Frame( _T("Hello World title"),200,200);
  frame->Show(true);
  SetTopWindow(frame);

  return true;
}

int App::OnExit(){
  return 0;
}
