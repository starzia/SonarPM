#include "SonarThread.hpp"
#include "PlotEvent.hpp"
#include <iostream>

#include <stdlib.h> //for rand

#include <wx/event.h> // for wxxQueueEvent

using namespace std;

SonarThread::SonarThread( Frame* mf ) : 
  wxThread(), mainFrame( mf ){}

void* SonarThread::Entry(){
  AudioDev my_audio = AudioDev();
  Config conf;
  conf.load( my_audio, SysInterface::config_dir()+CONFIG_FILENAME );
  
  //this->poll( my_audio, conf );
  this->power_management( my_audio, conf );
  return 0;
}

void SonarThread::poll( AudioDev & audio, Config & conf ){
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it 
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* s = audio.nonblocking_play_loop( ping );
  while( 1 ){
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    Statistics st = measure_stats( rec, conf.ping_freq );
    cout << st << endl;
    
    // update gui
    PlotEvent evt = PlotEvent( PLOT_EVENT_POINT );
    evt.setVal( st.delta );
    mainFrame->GetEventHandler()->AddPendingEvent( evt );
  }
}


void SonarThread::power_management( AudioDev & audio, Config & conf ){
  // buffer duration is one second, but actually it just needs to be a multiple
  // of the ping_period.

  // update gui
  PlotEvent evt = PlotEvent( PLOT_EVENT_THRESHOLD );
  evt.setVal( conf.threshold );
  mainFrame->GetEventHandler()->AddPendingEvent( evt );

  SysInterface::register_term_handler();
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin power management loop at frequency of " 
       <<conf.ping_freq<<"Hz"<<endl;
  PaStream* strm;
  strm = audio.nonblocking_play_loop( ping ); // initialize and start ping
  AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
  SysInterface::log( "begin" );
  long start_time = get_log_start_time();
  while( 1 ){
    SysInterface::wait_until_idle();
    AudioDev::check_error( Pa_StartStream( strm ) ); // resume ping

    //-- THRESHOLD RAISING
    if( SysInterface::idle_seconds() > IDLE_SAFETYNET ){
      // if we've been tring to detect use for too long, then this probably
      // means that the threshold is too low.
      cout << "False attention detected." <<endl;
      SysInterface::log("false attention");
      conf.threshold *= DYNAMIC_THRESH_FACTOR;
      conf.write_config_file(); // config save changes

      // update gui
      PlotEvent evt = PlotEvent( PLOT_EVENT_THRESHOLD );
      evt.setVal( conf.threshold );
      mainFrame->GetEventHandler()->AddPendingEvent( evt );
    }

    // record and process
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;
    SysInterface::log( s );

    // update gui
    PlotEvent evt = PlotEvent( PLOT_EVENT_POINT );
    evt.setVal( s.delta );
    mainFrame->GetEventHandler()->AddPendingEvent( evt );

    // if sonar reading below thresh. and user is still idle ...
    if( s.delta < conf.threshold 
	&& SysInterface::idle_seconds() > IDLE_THRESH ){
      // sleep monitor
      SysInterface::sleep_monitor();
      long sleep_timestamp = SysInterface::current_time();
      SysInterface::wait_until_active(); // OS will have turned on monitor
      
      //-- THRESHOLD LOWERING
      // waking up too soon means that we've just irritated the user
      if( SysInterface::current_time() - sleep_timestamp < IDLE_THRESH ){
	cout << "False sleep detected." <<endl;
	SysInterface::log("false sleep");
	conf.threshold /= DYNAMIC_THRESH_FACTOR;
	conf.write_config_file(); // config save changes

	// update gui
	PlotEvent evt = PlotEvent( PLOT_EVENT_THRESHOLD );
	evt.setVal( conf.threshold );
	mainFrame->GetEventHandler()->AddPendingEvent( evt );
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
}



