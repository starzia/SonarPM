#include "App.hpp"
#include "../sonar.hpp"

//====================== IMPLEMENTATIONS ====================================
// generate the main() entry point
IMPLEMENT_APP(App)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool App::OnInit(){
  // use standard command line handling:
  if ( !wxApp::OnInit() ) return false;

  this->frame = new Frame( _T("Hello World title"),600,600);
  frame->Show(true);
  //SetTopWindow( this->frame );

  // pop up config window
  this->confFrame = new ConfigFrame( _T("Configuration"),500,500);
  this->confFrame->Show(true);
  SetTopWindow( this->confFrame );

  return true;
}

int App::OnExit(){
  return 0;
}
