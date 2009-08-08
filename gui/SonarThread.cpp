#include "SonarThread.hpp"
#include "PlotEvent.hpp"
#include "../sonar.hpp"
#include <iostream>

#include <stdlib.h> //for rand

#include <wx/event.h> // for wxxQueueEvent

using namespace std;

SonarThread::SonarThread( Frame* mf, sonar_mode m ) :
     wxThread(wxTHREAD_DETACHED), mainFrame( mf ), mode(m), lastCalibration(0){
  // the following gets phone home scheduling started
  this->lastPhoneHome = SysInterface::current_time();
}

void* SonarThread::Entry(){
  cerr << "SonarThread entered" <<endl;
  if( !this->conf.load( SysInterface::config_dir()+CONFIG_FILENAME ) ){
   cerr << "fatal error: could not load config file" <<endl;
   return 0;
  }
  this->audio = AudioDev( conf.rec_dev, conf.play_dev );

  switch( this->mode ){
    case MODE_POWER_MANAGEMENT:
      this->power_management();
      break;
    case MODE_POLLING:
      this->poll();
      break;
    case MODE_FREQ_RESPONSE:
      log_freq_response( this->audio);
      break;
    case MODE_ECHO_TEST:
    default:
      break;
  }
  return 0;
  // thread is now terminated.
}

void SonarThread::OnExit(){
  this->reset(); // create gap in plot
  cerr << "SonarThread exited" <<endl;
  if( this->mode == MODE_POWER_MANAGEMENT ){
      SysInterface::log( "quit" );
  }
}

void SonarThread::updateGUIThreshold( float thresh ){
  cerr << "setting threshold to "<<thresh<<endl;
  // update gui
  PlotEvent evt = PlotEvent( PLOT_EVENT_THRESHOLD );
  evt.setVal( thresh, 0, 0 ); // last vals will be ignored
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );
}

void SonarThread::updateGUI( float echo_delta, float window_avg, float thresh ){
  // update gui
  PlotEvent evt = PlotEvent( PLOT_EVENT_POINT );
  evt.setVal( echo_delta, window_avg, thresh );
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );
}

void SonarThread::reset(){
  // check that we are not already in gap
  if( this->windowHistory.size() > 0 ){
    // create gap in plot
    PlotEvent evt = PlotEvent( PLOT_EVENT_GAP );
    this->mainFrame->GetEventHandler()->AddPendingEvent( evt );

    // clear sonar window
    this->windowHistory.resize( 0,0 );
  }
}

bool SonarThread::updateThreshold(){
  // check to see whether threshold has already been set, if not then set it
  if( !(conf.threshold > 0) ){ // initially threshold will be set to zero or NaN

    // set threshold to half of
    // current, assuming that program has just been launched so user is present.
    conf.threshold = this->windowAvg / 2;
    cout << "set threshold to " << conf.threshold << endl;
    updateGUIThreshold( conf.threshold );
    return true;
  }
}

bool SonarThread::scheduler( long start_time ){
  long currentTime = SysInterface::current_time();

  //-- PHONE HOME, if enough time has passed
  if( conf.allow_phone_home 
      && (currentTime - this->lastPhoneHome) > SonarThread::PHONEHOME_INTERVAL ){
    if( SysInterface::phone_home() ){
      // if phone home was successful, then record time
      this->lastPhoneHome = currentTime;
    }
  }

  // recalibrate, if enough time has passed
  if( currentTime - this->lastCalibration  > SonarThread::RECALIBRATION_INTERVAL ){
    this->conf.threshold = 0.0/0.0; // blank the threshold so it will be reset
    this->lastCalibration = currentTime;
  }
}

void SonarThread::poll(){
  this->conf.threshold = (0.0/0.0); // blank threshold, so it won't be drawn

  AudioBuf ping = tone( 0.01, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* strm = audio.nonblocking_play_loop( ping );

  // test to see whether we should die
  while( !this->TestDestroy() ){
    this->recordAndProcessAndUpdateGUI();
  }

  // clean up portaudio so that we can use it again later.
  audio.check_error( Pa_StopStream( strm ) );
  audio.check_error( Pa_CloseStream( strm ) );
}


void SonarThread::power_management(){
  // ignore previously saved threshold and recalibrate each time
  conf.threshold = 0.0/0.0;

  // buffer duration is 10ms, but actually it just needs to be a multiple
  // of the ping_period.
  AudioBuf ping = tone( 0.01, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin power management loop at frequency of " 
       <<conf.ping_freq<<"Hz"<<endl;
  // initialize and start ping
  PaStream* strm = audio.nonblocking_play_loop( ping );
  AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
  SysInterface::log( "begin" );
  long start_time = get_log_start_time();

  // test to see whether we should die
  while( !this->TestDestroy() ){
    // check scheduler to see if there are any periodic tasks to complete.
    if( !this->scheduler( start_time ) ) break;

    // Pause so that we don't poll idle_seconds() constantly
    SysInterface::sleep( SonarThread::SLEEP_LENGTH );
    // If user is active, reset window and skip rest of loop.
    if( SysInterface::idle_seconds() < SonarThread::IDLE_TIME ){
      this->reset();
      continue;
    }

    // run sonar, and store a new reading
    AudioDev::check_error( Pa_StartStream( strm ) ); // resume ping
    this->recordAndProcessAndUpdateGUI();
    AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping

    // if we have not collected enough consecutive readings to comprise a
    // single averaging window, then we can't yet do anything further.
    if( !( this->windowAvg > 0 ) ) continue;

    // check to see that threshold has been set. If not, set it.
    if( !this->updateThreshold() ) break; // break if interrupted

    // if sonar reading below thresh. and user is still idle ...
    if( this->windowAvg < conf.threshold
	&& SysInterface::idle_seconds() > SonarThread::IDLE_TIME ){
      // sleep monitor
      SysInterface::sleep_monitor();
      long sleep_timestamp = SysInterface::current_time();

      this->reset();
      if( !this->waitUntilActive() ) break; // break if interrupted
      // at this point, OS will have turned on monitor
      
      //-- THRESHOLD LOWERING
      // waking up too soon means that we've just irritated the user
      if( SysInterface::current_time() - sleep_timestamp 
            < SonarThread::FALSE_NEG ){
	cout << "False sleep detected." <<endl;
	SysInterface::log("false sleep");
	conf.threshold /= SonarThread::dynamicThreshFactor;
	///conf.write_config_file(); // config save changes
        updateGUIThreshold ( conf.threshold );
      }
    }
  }
  SysInterface::log( "interrupted" );
  // clean up portaudio so that we can use it again later.
  audio.check_error( Pa_CloseStream( strm ) );
}

void SonarThread::recordAndProcessAndUpdateGUI(){
  // record and process
  AudioBuf rec = audio.blocking_record( WINDOW_LENGTH );
  Statistics s = measure_stats( rec, conf.ping_freq );
  cout << s << endl;
  ///SysInterface::log( s );

  // update sliding window and GUI
  this->windowHistory.push_front( FEATURE(s) );
  unsigned int n = this->windowHistory.size();
  this->windowAvg = 0;

  int i;
  for( i=0; i<n; i++ ) windowAvg += windowHistory[i];
  windowAvg /= n;

  cerr << "window_avg (over "<<n<<"): " << windowAvg <<endl;

  // if window is incomplete, do not draw avg.
  if ( n < SonarThread::SLIDING_WINDOW ){
    windowAvg = 0.0/0.0;
  }else{
    windowHistory.pop_back();
  }
  
  updateGUI( FEATURE(s), windowAvg, conf.threshold );
}

bool SonarThread::waitUntilIdle(){
  while( SysInterface::idle_seconds() < IDLE_THRESH ){
    SysInterface::sleep( SLEEP_TIME ); //don't poll idle_seconds() constantly
    if( this->TestDestroy() ) return false; //test to see if thread was destroyed
  }
  SysInterface::log( "idle" );
  return true;
}

bool SonarThread::waitUntilActive(){
  duration_t prev_idle_time = SysInterface::idle_seconds();
  duration_t current_idle_time = SysInterface::idle_seconds();
  while( current_idle_time >= prev_idle_time ){
    SysInterface::sleep( SLEEP_TIME );
    prev_idle_time = current_idle_time;
    current_idle_time = SysInterface::idle_seconds();
    if( this->TestDestroy() ) return false; //test to see if thread was destroyed
  }
  SysInterface::log( "active" );
  return true;
}


//=========================================================================
EchoThread::EchoThread( wxDialog* diag,
                        unsigned int r_dev, unsigned int p_dev )
  : wxThread(wxTHREAD_DETACHED), 
    echoDialog(diag), play_dev(p_dev), rec_dev(r_dev){}

EchoThread::~EchoThread(){}

void* EchoThread::Entry(){
  AudioDev audio = AudioDev( this->rec_dev, this->play_dev );

  duration_t test_length = 3;
  cout<<"recording audio..."<<endl;
  AudioBuf buf = audio.blocking_record( test_length );
  cout<<"playing back the recording..."<<endl;

  audio.blocking_play( buf );
  // close status window when playback is done
  this->echoDialog->Close();
  return 0;
}


