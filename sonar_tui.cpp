#include "sonar.hpp"
#include "dsp.hpp"
#include <iostream>
#include <sstream>

//#define PROFILING // this enables audio HW profiling mode
using namespace std;

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
  if( conf.load( my_audio, SysInterface::config_dir()+CONFIG_FILENAME ) ){
    log_freq_response( my_audio );
    log_model();
    // send initial configuration home
    if( conf.allow_phone_home ) SysInterface::phone_home();
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
