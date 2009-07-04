#include "SonarThread.hpp"
#include "PlotEvent.hpp"
#include <iostream>

#include <stdlib.h> //for rand

#include <wx/event.h> // for wxxQueueEvent

using namespace std;

SonarThread::SonarThread( Frame* mf, bool pm ) :
  wxThread(wxTHREAD_DETACHED), mainFrame( mf ), doPowerManagement(pm){}

void* SonarThread::Entry(){
  Config conf;
  cerr << "SonarThread entered" <<endl;
  if( !conf.load( SysInterface::config_dir()+CONFIG_FILENAME ) ){
   cerr << "fatel error: could not load config file" <<endl;   
  }
  AudioDev my_audio = AudioDev( conf.rec_dev, conf.play_dev );
  
  if( this->doPowerManagement ){
    this->power_management( my_audio, conf );
  }else{
    this->poll( my_audio, conf );
  }
  return 0;
  // thread is now terminated.
}

void SonarThread::OnExit(){
  this->mainFrame->nullifyThread(); // clear parent's pointer to this thread
  cerr << "SonarThread exited" <<endl;
}

void SonarThread::updateGUIThreshold( float thresh ){
  // update gui
  PlotEvent evt = PlotEvent( PLOT_EVENT_THRESHOLD );
  evt.setVal( thresh );
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );
}

void SonarThread::updateGUIDelta( float echo_delta ){
  // update gui
  PlotEvent evt = PlotEvent( PLOT_EVENT_POINT );
  evt.setVal( echo_delta );
  this->mainFrame->GetEventHandler()->AddPendingEvent( evt );
}

void SonarThread::poll( AudioDev & audio, Config & conf ){
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it 
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* strm = audio.nonblocking_play_loop( ping );

  // test to see whether we should die
  while( !this->TestDestroy() ){
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    if( this->TestDestroy() ) break;
    Statistics st = measure_stats( rec, conf.ping_freq );
    cout << st << endl;

    updateGUIDelta( st.delta );
  }

  // clean up portaudio so that we can use it again later.
  audio.check_error( Pa_StopStream( strm ) );
  audio.check_error( Pa_CloseStream( strm ) );
}


void SonarThread::power_management( AudioDev & audio, Config & conf ){
  // buffer duration is one second, but actually it just needs to be a multiple
  // of the ping_period.

  updateGUIThreshold( conf.threshold );

  SysInterface::register_term_handler();
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
      conf.write_config_file(); // config save changes
      updateGUIThreshold( conf.threshold );
    }

    // record and process
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;
    SysInterface::log( s );
    updateGUIDelta( s.delta );

    // if sonar reading below thresh. and user is still idle ...
    if( s.delta < conf.threshold 
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
	conf.write_config_file(); // config save changes
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
EchoThread::EchoThread( wxDialog* recDiag, wxDialog* playDiag,
                        unsigned int r_dev, unsigned int p_dev )
  : wxThread(wxTHREAD_DETACHED), 
    recDialog(recDiag), playDialog(playDiag), play_dev(p_dev), rec_dev(r_dev){}

EchoThread::~EchoThread(){}

void* EchoThread::Entry(){
  this->playDialog->Show(false);
  this->recDialog->Show(true);

  AudioDev audio = AudioDev( this->rec_dev, this->play_dev );

  duration_t test_length = 3;
  cout<<"recording audio..."<<endl;
  AudioBuf buf = audio.blocking_record( test_length );
  cout<<"playing back the recording..."<<endl;

  // update status dialogs
  ///wxCommandEvent evt = wxCommandEvent( recordingDoneCommandEvent );
  this->recDialog->Close(); //->AddPendingEvent( evt );
  this->playDialog->Show(true);

  audio.blocking_play( buf );

  // notify status window that playback is done
  this->playDialog->Close();
  return 0;
}


