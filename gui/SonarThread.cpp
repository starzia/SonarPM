#include "SonarThread.hpp"
#include "PlotEvent.hpp"
#include "../sonar.hpp"
#include <iostream>

#include <stdlib.h> //for rand

#include <wx/event.h> // for wxxQueueEvent

using namespace std;

SonarThread::SonarThread( Frame* mf, sonar_mode m ) :
                      wxThread(wxTHREAD_DETACHED), mainFrame( mf ), mode(m){}

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
  this->mainFrame->nullifyThread(); // clear parent's pointer to this thread
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

void SonarThread::poll(){
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it 
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* strm = audio.nonblocking_play_loop( ping );

  // test to see whether we should die
  while( !this->TestDestroy() ){
    this->recordAndProcessAndUpdateGUI();
/*
      AudioBuf rec = audio.blocking_record( SonarThread::WINDOW_LENGTH *
                                          SonarThread::SLIDING_WINDOW );
    if( this->TestDestroy() ) break;
    Statistics st = measure_stats( rec, conf.ping_freq );
    cout << st << endl;

    updateGUI( FEATURE(st), 0, 0 );
 */
  }

  // clean up portaudio so that we can use it again later.
  audio.check_error( Pa_StopStream( strm ) );
  audio.check_error( Pa_CloseStream( strm ) );
}


void SonarThread::power_management(){
  //-- INITIAL THRESHOLD SETTING
  // ignore previously saved threshold and recalibrate each time
  conf.threshold = 0;
  if( conf.threshold <= 0 ){ // initially threshold will be set to zero
    AudioBuf blip = tone(SonarThread::WINDOW_LENGTH*SonarThread::SLIDING_WINDOW,
                         conf.ping_freq );
    AudioBuf rec = audio.recordback( blip );
    Statistics blip_s = measure_stats( rec, conf.ping_freq );
    cout << "chose preliminary threshold of "<<FEATURE(blip_s)<<endl<<endl;
    conf.threshold = ( FEATURE(blip_s) )/2;
    ///conf.write_config_file(); // save new threshold
  }
  updateGUIThreshold( conf.threshold );


  // buffer duration is one second, but actually it just needs to be a multiple
  // of the ping_period.
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin power management loop at frequency of " 
       <<conf.ping_freq<<"Hz"<<endl;
  PaStream* strm;
  strm = audio.nonblocking_play_loop( ping ); // initialize and start ping
  AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
  SysInterface::log( "begin" );
  long start_time = get_log_start_time();

  // test to see whether we should die
  while( !this->TestDestroy() ){
    if( !this->waitUntilIdle() ) break; // break if interrupted
    AudioDev::check_error( Pa_StartStream( strm ) ); // resume ping

    //-- THRESHOLD RAISING
    if( SysInterface::idle_seconds() > IDLE_SAFETYNET ){
      // if we've been tring to detect use for too long, then this probably
      // means that the threshold is too low.
      cout << "False attention detected." <<endl;
      SysInterface::log("false attention");
      conf.threshold *= DYNAMIC_THRESH_FACTOR;
      ///conf.write_config_file(); // config save changes
      updateGUIThreshold( conf.threshold );
    }

    this->recordAndProcessAndUpdateGUI();
    AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping

    // if sonar reading below thresh. and user is still idle ...
    if( this->windowHistory[0] < conf.threshold
	&& SysInterface::idle_seconds() > IDLE_THRESH ){
      // sleep monitor
      SysInterface::sleep_monitor();
      long sleep_timestamp = SysInterface::current_time();
      if( !this->waitUntilActive() ) break; // break if interrupted
      // at this point, OS will have turned on monitor
      
      //-- THRESHOLD LOWERING
      // waking up too soon means that we've just irritated the user
      if( SysInterface::current_time() - sleep_timestamp < IDLE_THRESH ){
	cout << "False sleep detected." <<endl;
	SysInterface::log("false sleep");
	conf.threshold /= DYNAMIC_THRESH_FACTOR;
	///conf.write_config_file(); // config save changes
        updateGUIThreshold ( conf.threshold );
      }
    }
    //-- PHONE HOME, if enough time has passed
    if( conf.allow_phone_home &&
	(SysInterface::current_time()-start_time) > PHONEHOME_TIME ){
      if( SysInterface::phone_home() ){
	// if phone home was successful, then disable future phonehome attempts
	conf.disable_phone_home();
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
  unsigned int n = this->windowHistory.size();
  float window_avg;
  if( n == 0 ){
    window_avg = FEATURE(s);
  }else if( n < SonarThread::SLIDING_WINDOW ){
    window_avg = ( (n-1) * this->windowHistory[0] + FEATURE(s) ) / n;
  }else{
    window_avg = ( n * this->windowHistory[0] +
                   FEATURE(s) - this->windowHistory[n-1] ) / n;
    this->windowHistory.pop_back();
  }
  this->windowHistory.push_front( window_avg );
  updateGUI( FEATURE(s), window_avg, conf.threshold );
  cerr << "window_avg (over "<<n<<"): " << window_avg <<endl;
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


