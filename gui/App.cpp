#include "App.hpp"
#include "../sonar.hpp"

//====================== IMPLEMENTATIONS ====================================
// generate the main() entry point
IMPLEMENT_APP(App)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool App::OnInit(){
  // use standard command line handling:
  if ( !wxApp::OnInit() ) return false;

  // initialize portaudio
  Pa_Initialize();

  this->frame = new Frame( _T("Hello World title"),400,400);
  frame->Show(true);
  //SetTopWindow( this->frame );

  return true;
}

int App::OnExit(){
  // close portaudio
  Pa_Terminate();
  return 0;
}
