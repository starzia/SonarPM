#include "SonarThread.hpp"
#include "SonarEvent.hpp"
#include "../SysInterface.hpp"
#include <iostream>
#include <sstream>

#include <wx/event.h> // for wxxQueueEvent
#ifdef PLATFORM_WINDOWS
#include <PowrProf.h>
#endif

//=========== CONSTANTS ===========
const duration_t SonarThread::WINDOW_LENGTH = (0.5);
const unsigned int SonarThread::SLIDING_WINDOW = (10);
const duration_t SonarThread::IDLE_TIME = (1);
const duration_t SonarThread::SLEEP_LENGTH = (0.2);
const duration_t SonarThread::FALSE_NEG = (5);
const float SonarThread::DYN_THRESH_FACTOR = 0.8;
const duration_t SonarThread::RECALIBRATION_INTERVAL = 3600;
const duration_t SonarThread::DISPLAY_TIMEOUT = (600); // ten minutes
const frequency SonarThread::DEFAULT_PING_FREQ = (22000);
const duration_t SonarThread::DUMMY_INPUT_INTERVAL = (55);
const float SonarThread::ACTIVE_GAIN = 2;

using namespace std;

SonarThread::SonarThread( Frame* mf, sonar_mode m ) :
     wxThread(wxTHREAD_DETACHED), mainFrame( mf ), mode(m),
     threshold(NAN), windowAvg(NAN), lastCalibration(0) {
  lastUserInputTime = lastDummyInputTime = SysInterface::current_time();
}

void* SonarThread::Entry(){
  cerr << "SonarThread entered" <<endl;

  //====== CONFIGURATION ======
  if( !this->conf.load() ){
   cerr << "fatal error: could not load config file" <<endl;
   return 0;
  }
  this->audio = AudioDev( conf.rec_dev, conf.play_dev );
  // link config file to logger object
  this->logger.setConfig( &(this->conf) );
  //===========================

  switch( this->mode ){
    case MODE_POWER_MANAGEMENT:
      this->power_management();
      break;
    case MODE_POLLING:
      this->poll();
      break;
    case MODE_FREQ_RESPONSE:
      this->logger.log_freq_response( this->audio);
      break;
    case MODE_ECHO_TEST:
    default:
      break;
  }
  // signal completion to frame
  SonarEvent evt = SonarEvent( SONAR_DONE );
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );
  return 0;
  // thread is now terminated.
}

void SonarThread::OnExit(){
  this->reset(); // create gap in plot
  cerr << "SonarThread exited" <<endl;
  if( this->mode == MODE_POWER_MANAGEMENT ){
    this->logger.log( "end" );
  }
}

void SonarThread::setThreshold( float thresh ){
  cout << "setting threshold to "<<thresh<<endl;
  this->threshold = thresh;
  // update gui
  SonarEvent evt = SonarEvent( PLOT_EVENT_THRESHOLD );
  evt.setVal( thresh, 0, 0 ); // last vals will be ignored
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );
  // log new value
  ostringstream log_msg;
  log_msg << "threshold " << thresh;
  this->logger.log( log_msg.str() );
}

void SonarThread::updateGUI( float echo_delta, float window_avg, float thresh,
                             bool setStatus = true ){
  // update plot
  SonarEvent evt = SonarEvent( PLOT_EVENT_POINT );
  evt.setVal( echo_delta, window_avg, thresh );
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );

  if( setStatus ){
    // set status text
    SonarEvent evt2 = SonarEvent( STATUS_MSG );
    ostringstream ss;
    ss << "Last reading: " << echo_delta
       << ",  ten-point average: " << window_avg;
    evt2.setMsg( ss.str() );
    this->mainFrame->GetEventHandler()->AddPendingEvent( evt2 );
  }
}

void SonarThread::setDisplayTimeout(){
#ifdef PLATFORM_WINDOWS
  // inhibit OS power managment, since we will be simulating it
  // (actually, this inhibits only some policies; we still need
  //  to periodically send keystrokes to keep screensaver off)
  SetThreadExecutionState( ES_DISPLAY_REQUIRED | ES_CONTINUOUS );

  GLOBAL_POWER_POLICY gpp;
  POWER_POLICY pp;
  if( GetCurrentPowerPolicies( &gpp, &pp ) ){
    this->displayTimeout = pp.user.VideoTimeoutDc;
  }else{
    this->logger.log( "displayTimeoutFailed" );
    this->displayTimeout = SonarThread::DISPLAY_TIMEOUT;
  }
#else
  this->displayTimeout = SonarThread::DISPLAY_TIMEOUT;
#endif
  // log new setting
  ostringstream log_msg;
  log_msg << "displayTimeout " << this->displayTimeout;
  this->logger.log( log_msg.str() );
  cout << log_msg.str() << endl;
}

void SonarThread::reset(){
  // check that we are not already in gap
  if( this->windowHistory.size() > 0 ){
    // create gap in plot
    SonarEvent evt = SonarEvent( PLOT_EVENT_GAP );
    this->mainFrame->GetEventHandler()->AddPendingEvent( evt );

    // clear sonar window
    this->windowHistory.resize( 0,0 );
  }
}

/** updateThreshold() sets the threshold if it is not already set.
 * It collects sonar readings of the user in the active state by continuously
 * recording and discarding readings that do not coincide with user input.
 * Then it simply sets the threshold to half of the average active reading.
 * @return false if this fcn was interrupted by thread deletion
 *         true if threshold was successfully set.
 */
bool SonarThread::updateThreshold(){
  vector<float> activeReadings;
  cout << "updating threshold using active readings..." << endl;

  // set status text
  SonarEvent evt = SonarEvent( STATUS_MSG );
  evt.setMsg( "Calibrating...  Please move the mouse for a few seconds." );
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );

  while( activeReadings.size() < SonarThread::SLIDING_WINDOW ){
    if( this->TestDestroy() ) return false; // test to see whether we should die
      
    // take sonar reading
    AudioBuf rec = audio.blocking_record( SonarThread::WINDOW_LENGTH );
    Statistics s = measure_stats( rec, conf.ping_freq );

    // if active
    if( this->true_idle_seconds() < SonarThread::IDLE_TIME ){
      activeReadings.push_back( FEATURE(s) );
    }
    // plot readings, but don't update status text
    updateGUI( FEATURE(s), NAN, NAN, false );
  }
  // set threshold to 1/ACTIVE_GAIN * average of active sonar readings
  float newThreshold = 0;
  unsigned int i;
  for( i=0; i<activeReadings.size(); i++ ) newThreshold += activeReadings[i];
  newThreshold /= activeReadings.size() * SonarThread::ACTIVE_GAIN;
  this->setThreshold( newThreshold );

  // make a gap in the plot to separate training data
  SonarEvent evt2 = SonarEvent( PLOT_EVENT_GAP );
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt2 );
  return true;
}


bool SonarThread::scheduler( long log_start_time ){
  long currentTime = SysInterface::current_time();

  // recalibrate, if enough time has passed
  if( currentTime - this->lastCalibration  > SonarThread::RECALIBRATION_INTERVAL ){
    this->threshold = NAN; // blank the threshold so it will be reset
    this->lastCalibration = currentTime;
  }
  // generate dummy keyboard input if enough idle time has passed
  if( SysInterface::idle_seconds() > SonarThread::DUMMY_INPUT_INTERVAL ){
    this->sendDummyInput();
    this->lastDummyInputTime = currentTime;
  }
  return true; // return true for uninterrupted completion
}

bool SonarThread::resumePing(){
  PaError ret=0;
  // start ping, if necessary
  if( AudioDev::check_error( Pa_IsStreamStopped( this->pingStrm ) ) ){
    ret = AudioDev::check_error( Pa_StartStream( this->pingStrm ) ); // resume ping
    SysInterface::sleep(1); // wait for ping to get started
    //TODO: figure out some better solution than this long delay.
  }
  return (ret == paNoError);
}

bool SonarThread::pausePing(){
  PaError ret=0;
  // stop ping, if necessary
  if( AudioDev::check_error( Pa_IsStreamActive( this->pingStrm ) ) )
    AudioDev::check_error( Pa_StopStream( this->pingStrm ) ); // stop ping
  return (ret == paNoError);
}

void SonarThread::poll(){
  AudioBuf ping = tone( 0.01, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  this->pingStrm = audio.nonblocking_play_loop( ping );

  // test to see whether we should die
  while( !this->TestDestroy() ){
    this->recordAndProcessAndUpdateGUI();
  }

  // clean up portaudio so that we can use it again later.
  audio.check_error( Pa_StopStream( this->pingStrm ) );
  audio.check_error( Pa_CloseStream( this->pingStrm ) );
}


void SonarThread::power_management(){
  // buffer duration is 10ms, but actually it just needs to be a multiple
  // of the ping_period.
  AudioBuf ping = tone( 0.1, conf.ping_freq, 0,0 );
  cout << "Begin power management loop at frequency of " 
       <<conf.ping_freq<<"Hz"<<endl;
  // initialize and start ping
  this->pingStrm = audio.nonblocking_play_loop( ping );
  this->pausePing(); // pause to force resumePing() to be called before recording.
  this->logger.log( "begin" );
  long log_start_time = this->logger.get_log_start_time();
  this->setDisplayTimeout();

  // keep track of certain program events
  long lastSleep=0; // sleep means sonar-caused display sleep
  long lastTimeout=0; // timeout means timeout-caused display sleep
  bool sleeping=false; // indicates whether the sonar system caused a sleep
                       // note that timeout-sleeps do not set this flag
                       // so sonar readings continue even after timeout-sleep

  // test to see whether we should die
  while( !this->TestDestroy() ){
    // Pause so that we don't poll idle_seconds() constantly
    SysInterface::sleep( SonarThread::SLEEP_LENGTH );

    //==== ACTIVE =============================================================
    // If user is active, reset window and test for false negatives.
    if( this->true_idle_seconds() < SonarThread::IDLE_TIME ){
      this->reset(); // throw out history
      sleeping=false; // OS will wake up monitor on user input

      // check scheduler to see if there are any periodic tasks to complete.
      if( !this->scheduler( log_start_time ) ) break;

      // check to see that threshold has been set.  If not, set it.
      if( !(this->threshold > 0) && !sleeping ){
        this->resumePing();
        if( !this->updateThreshold() ) break; // break if it was interrupted
      }

      // waking up too soon means that we just irritated the user
      long currentTime = SysInterface::current_time();
      this->lastUserInputTime = currentTime;
      if( currentTime - lastTimeout < SonarThread::FALSE_NEG ){
        cout << "False timeout detected." <<endl;
        this->logger.log("false timeout");
        lastTimeout=0; // don't want to double-count false negatives
      }
      if( currentTime - lastSleep   < SonarThread::FALSE_NEG ){
        cout << "False sleep detected." <<endl;
        this->logger.log("false sleep");
        lastSleep=0;   // don't want to double-count false negatives
        //-- THRESHOLD LOWERING
        this->setThreshold( this->threshold*SonarThread::DYN_THRESH_FACTOR );
      }
      this->pausePing();

    //==== INACTIVE ===========================================================
    }else if( !sleeping ){ // If inactive and sleeping, wait until awakened.
      // ---SONAR READING---
      this->resumePing();
      // run sonar, and store a new reading
      this->recordAndProcessAndUpdateGUI( );

      // ---TIMEOUT POLICY---
      // if user has been idle a very long time, then simulate default PM action
      if( this->true_idle_seconds( ) > this->displayTimeout ){
        // if we've been tring to detect use for too long, then this probably
        // means that the threshold is too low.
        cout << "Display timeout." << endl;
        this->logger.log( "sleep timeout" );
        SysInterface::sleep_monitor( ); // sleep monitor
        lastTimeout = SysInterface::current_time( );
      }
      // ---SONAR POLICY---
      // if we have not collected enough consecutive readings to comprise a
      // single averaging window, then we can't yet do anything further.
      if( this->windowAvg > 0 ){
        // if sonar reading below threshold
        if( this->windowAvg < this->threshold ){
          this->logger.log( "sleep sonar" );
          SysInterface::sleep_monitor( ); // sleep monitor
          lastSleep = SysInterface::current_time( );
          sleeping = true;
          this->pausePing();
          this->reset();
        }
      }
    }
  }
  this->logger.log( "end" );
  // clean up portaudio so that we can use it again later.
  this->pausePing();
  AudioDev::check_error( Pa_CloseStream( this->pingStrm ) );
}

void SonarThread::recordAndProcessAndUpdateGUI(){
  // record and process
  AudioBuf rec = audio.blocking_record( SonarThread::WINDOW_LENGTH );
  Statistics s = measure_stats( rec, conf.ping_freq );
  this->logger.log( s );

  // update sliding window and GUI
  this->windowHistory.push_front( FEATURE(s) );
  unsigned int n = this->windowHistory.size();
  this->windowAvg = 0;

  unsigned int i;
  for( i=0; i<n; i++ ) windowAvg += windowHistory[i];
  windowAvg /= n;

  cout << s << '\t' <<"window_avg (over "<<n<<"): " << windowAvg <<endl;

  // if window is incomplete, do not draw avg.
  if ( n < SonarThread::SLIDING_WINDOW ){
    windowAvg = NAN;
  }else{
    windowHistory.pop_back();
  }
  
  updateGUI( FEATURE(s), windowAvg, this->threshold );
}

// send a dummy keystroke to disable OS screensaver and power management
void SonarThread::sendDummyInput(){
#ifdef PLATFORM_WINDOWS
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

  SendInput( 2, input, sizeof(INPUT) );
#endif
  // TODO: linux version (or some linux-specific screensaver deactivation)
}

duration_t SonarThread::true_idle_seconds(){
  long currentTime = SysInterface::current_time();
  duration_t sinceOS = SysInterface::idle_seconds(); // OS-reported idle time
  duration_t sinceDummy = currentTime - this->lastDummyInputTime;
  duration_t sinceUser  = currentTime - this->lastUserInputTime;
  // the above value is not up-to-date if the last input event was from user.

  if( sinceOS < sinceDummy ){
    // last input was from user
    return sinceOS;
  }else{
    // last input was from app
    return sinceUser;
  }
}

/* these functions are not currently needed
bool SonarThread::waitUntilIdle(){
  while( SysInterface::idle_seconds() < SonarThread::IDLE_TIME ){
    SysInterface::sleep( SonarThread::SLEEP_LENGTH ); //don't poll idle_seconds() constantly
    if( this->TestDestroy() ) return false; //test to see if thread was destroyed
  }
  this->logger.log( "idle" );
  return true;
}

bool SonarThread::waitUntilActive(){
  duration_t prev_idle_time = SysInterface::idle_seconds();
  duration_t current_idle_time = SysInterface::idle_seconds();
  while( current_idle_time >= prev_idle_time ){
    SysInterface::sleep( SonarThread::SLEEP_LENGTH );
    prev_idle_time = current_idle_time;
    current_idle_time = SysInterface::idle_seconds();
    if( this->TestDestroy() ) return false; //test to see if thread was destroyed
  }
  this->logger.log( "active" );
  return true;
}
*/

//=========================================================================
EchoThread::EchoThread( wxDialog* diag,
                        unsigned int r_dev, unsigned int p_dev )
  : wxThread(wxTHREAD_DETACHED), 
    echoDialog(diag), rec_dev(r_dev), play_dev(p_dev) {}

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


