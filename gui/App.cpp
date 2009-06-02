#include "App.hpp"

//====================== IMPLEMENTATIONS ====================================
// generate the main() entry point
IMPLEMENT_APP(App)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool App::OnInit(){
  this->frame = new Frame( _T("Hello World title"),600,600);
  frame->Show(true);
  SetTopWindow(frame);

  return true;
}

int App::OnExit(){
  return 0;
}
