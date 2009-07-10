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

  this->frame = new Frame( _T("Sonar status"),400,400);
  frame->Show(true);
  SetTopWindow( this->frame );

  // TODO: note that term_handler will print a "quit" msg in the log even if 
  // sonar power management had not been started.
  SysInterface::register_term_handler();

  return true;
}

int App::OnExit(){
  // if sonar thread did not close properly, then ensure that log has quit msg
  if( this->frame )
    if( this->frame->sThread )
      SysInterface::log( "quit" );

  // close portaudio
  Pa_Terminate();
  return 0;
}
