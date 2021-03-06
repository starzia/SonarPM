#include "App.hpp"
#include "../SysInterface.hpp"
#include "../Config.hpp"
#include "../Logger.hpp"
#include <iostream>

using namespace std;

//====================== IMPLEMENTATIONS ====================================
BEGIN_EVENT_TABLE( App, wxApp )
EVT_QUERY_END_SESSION( App::onShutdown )
EVT_END_SESSION( App::onShutdown )
END_EVENT_TABLE()

// generate the main() entry point
IMPLEMENT_APP(App)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool App::OnInit(){

  // use standard command line handling:
  if ( !wxApp::OnInit() ) return false;

  // initialize portaudio
  Pa_Initialize();

  Config testConfigExists;
  bool firstTime = !testConfigExists.load();
  this->frame = new Frame( _T("SonarPM version " VERSION),600,300);
  SetTopWindow( this->frame );
  if( !firstTime ) this->frame->Show(false); // start minimized

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

/** default behavior of QueryEndSession is to call Close(false) on 
  * TopLevelWindow.  We don't want to bother prompting the user for 
  * minimize/close, so Close(true) */
void App::onShutdown( wxCloseEvent& event ){
  if( this->frame )
    this->frame->Close(true); // disallow veto
}
