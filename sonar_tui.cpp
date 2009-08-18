#include "sonar.hpp"
#include "dsp.hpp"
#include <iostream>
#include <sstream>

//========================= IMPLEMENTATION CONSTANTS =========================
#define RECORDING_PERIOD (2.0)
//#define CONFIG_FILENAME ".sonarPM.cfg"
//#define LOG_FILENAME ".sonarPM.log"
#define SLEEP_TIME (0.2) // sleep time between idleness checks
#define IDLE_THRESH (5) // don't activate sonar until idle for this long
#define IDLE_SAFETYNET (300) // assume that if idle for this long, user is gone
#define DEFAULT_PING_FREQ (22000)
#define DYNAMIC_THRESH_FACTOR (1.3) // how rapidly does dynamic threshold move
//#define PHONEHOME_TIME (604800) // number of seconds before phone home (1 week)


//#define PROFILING // this enables audio HW profiling mode
using namespace std;

// FORWARD FUNCTION DECLARATIONS
void test_echo( AudioDev & audio );
void poll( AudioDev & audio, Config & conf );

/** This is the main program loop.  It checks for a user and powers down
    the display if it's reasonably confident that no one is there */
void power_management( AudioDev & audio, Config & conf );


int main( int argc, char* argv[] ){
  // parse commandline arguments
  bool do_poll=false, echo=false, response=false;
  int i;
  frequency poll_freq = 0;
  for( i=1; i<argc; i++ ){
    if( string(argv[i]) == string("--help") ){
      cout<<"usage is "<<argv[0]<<" [ --help | --poll [freq] | --echo | --response | --phonehome ]"<<endl;
      return -1;
    }else if( string(argv[i]) == string("--poll") ){
      do_poll=true;
    }else if( string(argv[i-1]) == string("--poll") && argv[i][0] != '-' ){
      istringstream ss( argv[i] );
      ss >> poll_freq;
      //poll_freq = atof( argv[i] );
    }else if( string(argv[i]) == string("--echo") ){
      echo=true;
    }else if( string(argv[i]) == string("--phonehome") ){
      SysInterface::phone_home();
    }else if( string(argv[i]) == string("--response") ){
      response=true;
    }else{
      cerr << "unrecognized parameter: " << argv[i] <<endl;
    }
  }

  AudioDev::init(); //start portaudio
  AudioDev my_audio = AudioDev();
  Config conf;

#ifdef PROFILING
  // if profiling, don't use config files.
  conf.new_config( my_audio );
  log_freq_response( my_audio );
  log_model();
  // send initial configuration home
  if( conf.allow_phone_home ) SysInterface::phone_home();
  cout << endl << "Thank you for your participation!"<<endl;
  cin.ignore();
  cin.get();
  return 0; // for HW profiling, stop here.
#endif

  // Try to Load config file.  If config file does not exist, this will prompt
  // the user for first-time setup.  load() returns true if new config created
  if( !conf.load() ){
    cerr<< "A new configuration file will now be created."<<endl;
    // prompt use for configuration
    conf.new_config( my_audio );

    // write configuration to file
    return conf.write_config_file();

    log_freq_response( my_audio );
    log_model();
    // send initial configuration home
    if( conf.allow_phone_home ) SysInterface::phone_home();
  }else{
    my_audio.choose_device( conf.rec_dev, conf.play_dev );
  }

  // choose operating mode
  if( echo ){
    test_echo( my_audio );
    return 0;
  }
  if( response ){
    test_freq_response( my_audio );
    return 0;
  }
  if( do_poll ){
    if( poll_freq != 0 ) conf.ping_freq = poll_freq;
    poll( my_audio, conf );
  }else{
    power_management( my_audio, conf );
  }
  return 0;
}

void test_echo( AudioDev & audio ){
  // This next block is a debugging audio test
  duration_t test_length = 3;
  cout<<"recording audio..."<<endl;
  AudioBuf buf = audio.blocking_record( test_length );
  cout<<"playing back the recording..."<<endl;
  audio.blocking_play( buf );
}

void power_management( AudioDev & audio, Config & conf ){
  // buffer duration is one second, but actually it just needs to be a multiple
  // of the ping_period.
  SysInterface::register_term_handler();
  float threshold = choose_ping_threshold( audio, conf.ping_freq );
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
      threshold *= DYNAMIC_THRESH_FACTOR;
      conf.write_config_file(); // config save changes
    }

    // record and process
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;
    SysInterface::log( s );

    // if sonar reading below thresh. and user is still idle ...
    if( FEATURE(s) < threshold && SysInterface::idle_seconds() > IDLE_THRESH ){
      // sleep monitor
      SysInterface::sleep_monitor();
      SysInterface::log( "sleep" );
      long sleep_timestamp = SysInterface::current_time();
      SysInterface::wait_until_active(); // OS will have turned on monitor

      //-- THRESHOLD LOWERING
      // waking up too soon means that we've just irritated the user
      if( SysInterface::current_time( ) - sleep_timestamp < IDLE_THRESH ){
        cout << "False sleep detected." << endl;
        SysInterface::log( "false sleep" );
        threshold /= DYNAMIC_THRESH_FACTOR;
      }
    }
    //-- PHONE HOME, if enough time has passed
    if( conf.allow_phone_home &&
	(SysInterface::current_time()-start_time) > PHONEHOME_TIME ){
      if( SysInterface::phone_home( ) ){
        // if phone home was successful, then disable future phonehome attempts
        conf.disable_phone_home( );
      }
    }

  }
}


/** poll is a simplified version of the power management loop wherein we just
    constantly take readings */
void poll( AudioDev & audio, Config & conf ){
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* strm = audio.nonblocking_play_loop( ping );
  while( 1 ){
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;
  }
  AudioDev::check_error( Pa_CloseStream( strm ) ); // close stream to free up dev
}




void Config::new_config( AudioDev & audio ){
    this->choose_phone_home();

    pair<unsigned int,unsigned int> devices = audio.prompt_device();
    this->rec_dev = devices.first;
    this->play_dev = devices.second;
    // set audio object to use the desired devices
    audio.choose_device( this->rec_dev, this->play_dev );
    this->warn_audio_level( audio );
    //this->choose_ping_freq( audio );
}

void Config::choose_ping_freq( AudioDev & audio ){
  cout <<""<<endl
       <<"This power management system uses sonar to detect whether you are sitting"<<endl
       <<"in front of the computer.  This means that the computer plays a very high"<<endl
       <<"frequency sound while recording the echo of that sound (and then doing"<<endl
       <<"some signal processing).  The calibration procedure that follows will try"<<endl
       <<"to choose a sound frequency that is low enough to register on your"<<endl
       <<"computer's microphone but high enough to be inaudible to you.  Note that"<<endl
       <<"younger persons and animals are generally more sensitive to high frequency"<<endl
       <<"noises.  Therefore, we advise you not to use this software in the company"<<endl
       <<"of children or pets."<<endl;

  cout << endl
       <<"Please adjust your speaker and microphone volume to normal levels."<<endl
       <<"Several seconds of white noise will be played.  Do not be alarmed and"<<endl
       <<"do not adjust your speaker volume level."<<endl
       <<"This is a normal part of the system calibration procedure."<<endl<<endl
       <<"Press <enter> to continue"<<endl;
  cin.ignore();
  cin.get();

  freq_response f = test_freq_response( audio );
  // TODO: search for maximum gain here
  float best_freq = f[0].first;
  float best_gain = f[0].second;

  if( best_gain < 10 ){
    cerr << "ERROR: Your mic and/or speakers are not sensitive enough to proceed!"<<endl;
    SysInterface::log( "freq>=start_freq" );
    SysInterface::phone_home();
    exit(-1);
  }
  this->ping_freq = best_freq;
}

/** current implementation just chooses a threshold in the correct neighborhood
    and relies on dynamic runtime adjustment for fine-tuning.
    Uses one half of the initial reading as the threshold.
    Assumption here is that user is present when calibrating */
float choose_ping_threshold( AudioDev & audio, frequency freq ){
  cout << "Please wait while the system is calibrated."<<endl;
  AudioBuf blip = tone( RECORDING_PERIOD, freq );
  AudioBuf rec = audio.recordback( blip );
  Statistics blip_s = measure_stats( rec, freq );
  cout << "chose preliminary threshold of "<<FEATURE(blip_s)<<endl<<endl;
  return ( FEATURE(blip_s) )/2;
}

void Config::choose_phone_home(){
  cout<<endl<<"Would you like to allow usage statistics to be sent back to Northwestern" << endl
      << "University computer systems researchers for the purpose of evaluating this"<<endl
      <<"software's performance and improving future versions of the software?"<<endl
      <<"[yes/no]: ";
  string input;
  while( cin >> input ){
    if( input == "yes" || input == "YES" || input == "Yes" ){
      this->allow_phone_home = true;
      break;
    }else if( input == "no" || input == "NO" || input == "No" ){
      this->allow_phone_home = false;
      break;
    }
    cout << "Send back statistics? [yes/no]: ";
  }
  cout << endl;
}

void Config::warn_audio_level( AudioDev & audio ){
  cout << endl<<"In order for this software to function correctly, you must set your"<< endl
       << "audio volume level to a normal listening level and unplug any" <<endl
       << "headphones so that the speakers are used."<<endl<<endl;
  cout << "Please adjust your volume settings now and press <ENTER> to continue."<<endl;
  cin.ignore();
  cin.get();
  string input;
  do{
    test_echo( audio );
    cout<<endl<<"If you just heard a recording play back, then your audio is working properly." << endl;
    cout<<"If this is true, type 'yes' to continue.  Otherwise, re-adjust your volume"<<endl
	<<"settings and type 'no' to try again."<<endl
	<<"It is also possible that you chose the wrong audio device numbers."<<endl
	<<"in this case you may close this application and start over."<<endl
	<<endl<<"Continue? [yes/no]: ";
    cin >> input;
  }while( !( input == "yes" || input == "YES" || input == "Yes" ) );
  cout << endl;
}

void SysInterface::wait_until_active(){
  duration_t prev_idle_time = SysInterface::idle_seconds();
  duration_t current_idle_time = SysInterface::idle_seconds();
  while( current_idle_time >= prev_idle_time ){
    SysInterface::sleep( SLEEP_TIME );
    prev_idle_time = current_idle_time;
    current_idle_time = SysInterface::idle_seconds();
  }
  SysInterface::log( "active" );
}
void SysInterface::wait_until_idle(){
  while( SysInterface::idle_seconds() < IDLE_THRESH ){
    SysInterface::sleep( SLEEP_TIME ); //don't poll idle_seconds() constantly
  }
  SysInterface::log( "idle" );
}