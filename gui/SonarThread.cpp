#include "SonarThread.hpp"
#include "../sonar.hpp"
#include "../dsp.hpp"
#include "../audio.hpp"
#include <iostream>

#include <stdlib.h> //for rand

#include <wx/event.h> // for wxxQueueEvent

using namespace std;

// how to define the custom event
DEFINE_EVENT_TYPE(PLOT_EVENT)


SonarThread::SonarThread( Frame* mf ) : 
  wxThread(), mainFrame( mf ){}

void* SonarThread::Entry(){
  AudioDev my_audio = AudioDev();
  Config conf;
  conf.load( my_audio, SysInterface::config_dir()+CONFIG_FILENAME );
  
  while( 1 ){
    wxCommandEvent evt( PLOT_EVENT );

    evt.SetString(_T("this"));

    // Event::Clone() makes sure that the internal wxString
    // member is not shared by other wxString instances:
    mainFrame->GetEventHandler()->AddPendingEvent( evt );

    ///this->mainFrame->addPoint( rand() );
    SysInterface::sleep( 0.2);
  }
  /*
  // POLL
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it 
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* s = my_audio.nonblocking_play_loop( ping );
  while( 1 ){
    AudioBuf rec = my_audio.blocking_record( RECORDING_PERIOD );
    Statistics st = measure_stats( rec, conf.ping_freq );
    this->mainFrame->addPoint( st.delta );
    cout << st << endl;
  }
  */
  return 0;
}
