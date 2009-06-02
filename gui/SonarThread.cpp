#include "SonarThread.hpp"
#include "../sonar.hpp"
#include "../dsp.hpp"
#include "../audio.hpp"
#include <iostream>
using namespace std;

SonarThread::SonarThread( Frame* mf ) : 
  wxThread(), mainFrame( mf ){}

void* SonarThread::Entry(){
  AudioDev my_audio = AudioDev();
  Config conf;
  conf.load( my_audio, SysInterface::config_dir()+CONFIG_FILENAME );

  // POLL
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it 
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* s = my_audio.nonblocking_play_loop( ping );
  while( 1 ){
    AudioBuf rec = my_audio.blocking_record( RECORDING_PERIOD );
    Statistics st = measure_stats( rec, conf.ping_freq );
    this->mainFrame->addPoint( st.delta );
    cout << s << endl;
  }
  return 0;
}
