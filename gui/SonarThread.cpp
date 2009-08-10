#include "SonarThread.hpp"
#include "PlotEvent.hpp"
#include "../sonar.hpp"
#include <iostream>

#include <stdlib.h> //for rand

#include <wx/event.h> // for wxxQueueEvent

using namespace std;

SonarThread::SonarThread( Frame* mf, sonar_mode m ) :
     wxThread(wxTHREAD_DETACHED), mainFrame( mf ), mode(m), lastCalibration(0),
     windowAvg(0.0/0.0), threshold(0.0/0.0){
  // The following gets phone home scheduling started.
  // Note that if we set lastPhoneHome to zero then it would phone home each
  // time the app is started.  Instead, we wait for a full PHONEHOME_INTERVAL.
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
  if( !(this->threshold > 0) ){ // initially threshold will be set to zero or NaN

    // set threshold to half of
    // current, assuming that program has just been launched so user is present.
    this->threshold = this->windowAvg / 2;
    cout << "set threshold to " << this->threshold << endl;
    updateGUIThreshold( this->threshold );
  }
  return true; // true for uninterrupted completion
}

bool SonarThread::scheduler( long log_start_time ){
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
    this->threshold = 0.0/0.0; // blank the threshold so it will be reset
    this->lastCalibration = currentTime;
  }
  return true; // true for uninterrupted completion
}

/*
#ifdef PLATFORM_WINDOWS
LRESULT wndProcCallback( int nCode, WPARAM wParam, LPARAM lParam ){
  CWPSTRUCT* msgData = (CWPSTRUCT*)lParam;

}
#endif

void SonarThread::setupSystemHooks(){
#ifdef PLATFORM_WINDOWS
  SetWindowsHookEx( WH_CALLWNDPROC, wndProcCallback, 0, 0 );
#else
  return;
#endif
}
*/

void SonarThread::poll(){
  this->threshold = (0.0/0.0); // blank threshold, so it won't be drawn

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
  // inhibit OS power managment
#ifdef PLATFORM_WINDOWS
  SetThreadExecutionState( ES_DISPLAY_REQUIRED | ES_CONTINUOUS );
#endif
  // TODO: inhibit screensaver as well

  // ignore previously saved threshold and recalibrate each time
  this->threshold = 0.0/0.0;

  // buffer duration is 10ms, but actually it just needs to be a multiple
  // of the ping_period.
  AudioBuf ping = tone( 0.01, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin power management loop at frequency of " 
       <<conf.ping_freq<<"Hz"<<endl;
  // initialize and start ping
  PaStream* strm = audio.nonblocking_play_loop( ping );
  AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
  SysInterface::log( "begin" );
  long log_start_time = get_log_start_time();

  // keep track of certain program events
  long lastSleep=0; // sleep means sonar-caused display sleep
  long lastTimeout=0; // timeout means timeout-caused display sleep
  bool sleeping=false; // indicates whether the sonar system caused a sleep
                       // note that timeout-sleeps do not set this flag
                       // so sonar readings continue even after timeout-sleep

  // test to see whether we should die
  while( !this->TestDestroy() ){
    // check scheduler to see if there are any periodic tasks to complete.
    if( !this->scheduler( log_start_time ) ) break;

    // Pause so that we don't poll idle_seconds() constantly
    SysInterface::sleep( SonarThread::SLEEP_LENGTH );

    //==== ACTIVE =============================================================
    // If user is active, reset window and test for false negatives.
    if( SysInterface::idle_seconds() < SonarThread::IDLE_TIME ){
      this->reset(); // throw out history
      sleeping=false; // OS will wake up monitor on user input

      // waking up too soon means that we just irritated the user
      long currentTime = SysInterface::current_time();
      if( currentTime - lastTimeout < SonarThread::FALSE_NEG ){
        cout << "False timeout detected." <<endl;
        SysInterface::log("false timeout");
        lastTimeout=0; // don't want to double-count false negatives
      }
      if( currentTime - lastSleep   < SonarThread::FALSE_NEG ){
        cout << "False sleep detected." <<endl;
        SysInterface::log("false sleep");
        lastSleep=0;   // don't want to double-count false negatives
        //-- THRESHOLD LOWERING
        this->threshold /= SonarThread::dynamicThreshFactor;
        updateGUIThreshold( this->threshold );
      }

      //==== INACTIVE ===========================================================
    }else if( !sleeping ){ // If inactive and sleeping, wait until awakened.
      // ---TIMEOUT---
      // if user has been idle a very long time, then simulate default PM action
      if( SysInterface::idle_seconds( ) > SonarThread::DISPLAY_TIMEOUT ){
        // if we've been tring to detect use for too long, then this probably
        // means that the threshold is too low.
        cout << "Display timeout." << endl;
        SysInterface::log( "timeout" );

        SysInterface::sleep_monitor( ); // sleep monitor
        lastTimeout = SysInterface::current_time( );
      }
      // ---SONAR---
      // if we have not collected enough consecutive readings to comprise a
      // single averaging window, then we can't yet do anything further.
      if( this->windowAvg > 0 ){

        // check to see that threshold has been set. If not, set it.
        if( !this->updateThreshold( ) ) break; // break if interrupted

        // if sonar reading below threshold
        if( this->windowAvg < this->threshold ){
          SysInterface::sleep_monitor( ); // sleep monitor
          lastSleep = SysInterface::current_time( );
          sleeping = true;
          this->reset();
        }
      }
      // run sonar, and store a new reading
      AudioDev::check_error( Pa_StartStream( strm ) ); // resume ping
      this->recordAndProcessAndUpdateGUI( );
      AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
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
  SysInterface::log( s );

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
  
  updateGUI( FEATURE(s), windowAvg, this->threshold );
}

// send a dummy keystroke to disable OS screensaver and power management
#ifdef PLATFORM_WINDOWS
void sendDummyInput(){
    INPUT input[2];
    ///memset(input, 0, sizeof(input));
    input[0].type = INPUT_KEYBOARD;

    input[0].ki.wVk = VK_NONAME; // character to send
    input[0].ki.dwFlags = 0;
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;

    input[1].ki.wVk = VK_NONAME; // character to send
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    input[1].ki.time = 0;
    input[1].ki.dwExtraInfo = 0;

    SendInput(2,input,sizeof(INPUT));
}
#endif

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


