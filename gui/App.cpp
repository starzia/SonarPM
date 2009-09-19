#include "App.hpp"
#include "../SysInterface.hpp"
#include "../Config.hpp"
#include "../Logger.hpp"
#include <iostream>

using namespace std;

//====================== IMPLEMENTATIONS ====================================
// generate the main() entry point
IMPLEMENT_APP(App)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool App::OnInit(){

  // use standard command line handling:
  if ( !wxApp::OnInit() ) return false;

  // initialize portaudio
  Pa_Initialize();

  this->frame = new Frame( _T("SonarPM version " VERSION),600,300);
  this->frame->Show(false); // start minimized
  SetTopWindow( this->frame );

  // TODO: note that term_handler will print a "quit" msg in the log even if 
  // sonar power management had not been started.
  SysInterface::register_term_handler();

  return true;
}

int App::OnExit(){
  cerr<<"OnExit"<<endl;
  // if sonar thread did not close properly, then ensure that log has quit msg
/* TODO: this crashes in windows, fix it.
  cerr << (void*)Logger::log_basic<<endl;
  Logger::log_basic( "end" );
*/
  // close portaudio
  Pa_Terminate();
  return 0;
}
