/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * see README.txt
 */
#include "sonar.hpp"
#include "dsp.hpp"
#include "SimpleIni.h" // for config files
#include <iostream>
#include <sstream>
#include <vector>
#include <exception>
#include <fstream> // for logfile writing
#include <limits> // for numeric_limit

#define RECORDING_PERIOD (10.0) 
#define CONFIG_FILENAME ".sonarPM.cfg"
#define LOG_FILENAME ".sonarPM.log"
#define FTP_SERVER "belmont.eecs.northwestern.edu"
#define FTP_USER "sonar"
#define FTP_PASSWD "ppiinngg"
#define SLEEP_TIME (0.2) // sleep time between idleness checks
#define IDLE_THRESH (5) // don't activate sonar until idle for this long
#define IDLE_SAFETYNET (300) // assume that if idle for this long, user is gone
#define DEFAULT_PING_FREQ (22000)
#define DYNAMIC_THRESH_FACTOR (1.3) // how rapidly does dynamic threshold move
#define PHONEHOME_TIME (604800) // number of seconds before phone home (1 week)

// One of the following should be defined to activate platform-specific code.
// It is best to make this choice in the Makefile, rather than uncommenting here
//#define PLATFORM_LINUX
//#define PLATFORM_WINDOWS
//#define PLATFORM_MAC

#include <ctime>
#if defined PLATFORM_LINUX
/* The following headers are provided by these packages on a Redhat system:
 * libX11-devel, libXext-devel, libScrnSaver-devel
 * There are also some additional dependencies for libX11-devel */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/scrnsaver.h>
#elif defined PLATFORM_WINDOWS
#include <ddk/ntddvdeo.h> // backlight control
#define IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS \
  CTL_CODE(FILE_DEVICE_VIDEO, 0x125, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#include <userenv.h> // home path query
#include <shlobj.h> // for CSIDL, SHGetFolderPath
#include <wininet.h> //  for ftp
typedef enum {
  SHGFP_TYPE_CURRENT = 0,
  SHGFP_TYPE_DEFAULT = 1,
} SHGFP_TYPE; // this should be in <shlobj.h> anyway
#endif
#ifndef PLATFORM_WINDOWS
#include <stdlib.h> //getenv, etc.
#include <signal.h>
#endif

using namespace std;

Config::Config(){}

/** call calibration functions to create a new configuration */
bool Config::load( AudioDev & audio, string filename ){
  this->filename = filename;
  // try to load from a data file
  CSimpleIniA ini(false,false,false);
  SI_Error rc = ini.LoadFile( filename.c_str() );
  if (rc < 0){
    // if file open unsuccessful, then run calibration
    cerr<< "Unable to open config file "<<filename<<endl;
    cerr<< "A new configuration file will now be created."<<endl;
    this->choose_phone_home();

    pair<unsigned int,unsigned int> devices = audio.prompt_device();
    this->rec_dev = devices.first;
    this->play_dev = devices.second;
    // set audio object to use the desired devices
    audio.choose_device( this->rec_dev, this->play_dev );
    this->warn_audio_level( audio );
    //this->choose_ping_freq( audio );
    this->ping_freq = DEFAULT_PING_FREQ;
    this->choose_ping_threshold( audio, this->ping_freq );
    // write configuration to file
    this->write_config_file();
    return true;
  }else{
    // if file open successful, then get the config key values from it.
    try{
      istringstream ss;
      ss.clear();
      ss.str( ini.GetValue("general","phone_home" ) );
      ss >> this->allow_phone_home;
      ss.clear();
      ss.str( ini.GetValue("general","recording_device" ) );
      ss >> this->rec_dev;
      ss.clear();
      ss.str( ini.GetValue("general","playback_device" ) );
      ss >> this->play_dev;
      ss.clear();
      ss.str( ini.GetValue("calibration","frequency" ) );
      ss >> this->ping_freq;
      ss.clear();
      ss.str( ini.GetValue("calibration","threshold" ) );
      ss >> this->threshold;
      cerr<< "Loaded config file "<<filename<<" with threshold: " 
	  << this->threshold <<endl;
      // set audio object to use the desired devices
      audio.choose_device( this->rec_dev, this->play_dev );
    }catch( const exception& e ){
      cerr <<"Error loading data from Config file "<<filename<<endl
	   <<"Please check the file for errors and correct or delete it"<<endl;
      exit(-1);
    }
    return false;
  }
}

bool Config::write_config_file(){
  CSimpleIniA ini(false,false,false);
  // set the key values
  ostringstream ss;
  string str;
  ss.str(""); // reset stringstream
  ss << this->allow_phone_home;
  ini.SetValue("general","phone_home", ss.str().c_str());
  ss.str("");
  ss << this->rec_dev;
  ini.SetValue("general","recording_device", ss.str().c_str());
  ss.str("");
  ss << this->play_dev;
  ini.SetValue("general","playback_device", ss.str().c_str());
  ss.str("");
  ss << this->ping_freq;
  ini.SetValue("calibration","frequency", ss.str().c_str());
  ss.str("");
  ss << this->threshold;
  ini.SetValue("calibration","threshold", ss.str().c_str());

  // write to file
  SI_Error rc = ini.SaveFile( this->filename.c_str() );
  if (rc < 0){
    cerr<< "Error saving config file "<<this->filename<<endl;
    return false;
  }else{
    cerr<< "Saved config file "<<this->filename<<" with threshold: "
	<< this->threshold <<endl;
    ostringstream msg;
    msg << "threshold " << this->threshold;
    SysInterface::log( msg.str() );
  }
  return true;
}

void Config::disable_phone_home(){
  this->allow_phone_home = false;
  this->write_config_file();
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
    and relies on dynamic runtime adjustment for fine-tuning */
void Config::choose_ping_threshold( AudioDev & audio, frequency freq ){
  cout << "Please wait while the system is calibrated."<<endl;
  AudioBuf blip = tone( RECORDING_PERIOD, freq );
  AudioBuf rec = audio.recordback( blip );
  Statistics blip_s = measure_stats( rec, freq );
  cout << "chose preliminary threshold of "<<blip_s.delta<<endl;
  this->threshold = blip_s.delta;
}

void Config::choose_phone_home(){
  cout << "Would you like to allow usage statistics to be sent back to Northwestern" << endl
       << "University for the purpose of evaluating this software's performance and" << endl
       << "improving future versions of the software? [yes/no]:" <<endl;
  string input;
  while( cin >> input ){
    if( input == "yes" || input == "YES" || input == "Yes" ){
      this->allow_phone_home = true;
      return;
    }else if( input == "no" || input == "NO" || input == "No" ){
      this->allow_phone_home = false;
      return;
    }
    cout << "[yes/no]? ";
  }
}
  
void Config::warn_audio_level( AudioDev & audio ){
  cout << endl<<"In order for this software to function correctly, you must set your"<< endl 
       << "audio volume level to a normal listening level and unplug any" <<endl
       << "headphones so that the speakers are used."<<endl<<endl
       << "Please adjust your volume settings now and press <ENTER> to continue."<<endl;
  cin.ignore(256, '\n');
  cin.get();
}

bool SysInterface::phone_home(){
  SysInterface::log( "phonehome" );
  cerr << "Sending log file to Northwestern University server."<<endl;
  ostringstream remote_filename;
  remote_filename << SysInterface::current_time();
#if defined PLATFORM_WINDOWS
  HINTERNET hnet = InternetOpen( "sonar", INTERNET_OPEN_TYPE_PRECONFIG,
				 NULL,NULL,NULL);
  hnet = InternetConnect( hnet, FTP_SERVER,
			  INTERNET_DEFAULT_FTP_PORT, FTP_USER, FTP_PASSWD,
			  INTERNET_SERVICE_FTP, NULL, NULL );
  string logfile = SysInterface::config_dir()+LOG_FILENAME;
  remote_filename << ".w.log";
  bool ret = FtpPutFile( hnet, logfile.c_str(), remote_filename.str().c_str(),
			 FTP_TRANSFER_TYPE_BINARY, NULL );
  InternetCloseHandle( hnet );
  return ret;
#else
  remote_filename << ".u.log";
  string command = "curl -T "+SysInterface::config_dir()+LOG_FILENAME+
    " ftp://"+FTP_USER+':'+FTP_PASSWD+'@'+FTP_SERVER+'/'+remote_filename.str();
  system( command.c_str() );
  return true;
#endif
}

bool SysInterface::sleep_monitor(){
  SysInterface::log( "sleep" );
#if defined PLATFORM_LINUX
  system( "xset dpms force standby" );
  return true;
#elif defined PLATFORM_WINDOWS
  // send monitor off message
  PostMessage( HWND_TOPMOST, WM_SYSCOMMAND, SC_MONITORPOWER, 1 ); 
  ///SendMessage( GetDesktopWindow(), WM_SYSCOMMAND, SC_MONITORPOWER, 1 ); 
  //                               -1 for "on", 1 for "low power", 2 for "off".
  //SendMessage( h, WM_SYSCOMMAND, SC_SCREENSAVE, NULL ); // activate scrnsaver
  cout << "monitor in standby mode."<<endl;
  SysInterface::sleep( 0.5 ); // give system time to process message
  return true;
#elif 0 // the following is Windows LCD brightness control code
  //open LCD device handle
  HANDLE lcd_handle = CreateFile( 
	"\\\\.\\LCD",  /*__in      LPCTSTR lpFileName=@"\\.\LCD"*/
	GENERIC_READ | GENERIC_WRITE,/*__in      DWORD dwDesiredAccess*/
	0,             /*__in      DWORD dwShareMode*/
	NULL,          /*__in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes*/
	OPEN_EXISTING, /*__in      DWORD dwCreationDisposition*/
	FILE_ATTRIBUTE_NORMAL, /*__in      DWORD dwFlagsAndAttributes*/
	NULL ); /*__in_opt  HANDLE hTemplateFile*/
  if( lcd_handle == INVALID_HANDLE_VALUE ){
    cerr << "ERROR: could not open LCD handle"<<endl;
    return false;
  }
  // read LCD's brightness range
  char buf[256];
  DWORD num_levels=0;
  if( DeviceIoControl( lcd_handle,                // handle to device
		       IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS,//dwIoControlCode
		       NULL,                      // lpInBuffer
		       0,                         // nInBufferSize
		       buf,                       // output buffer
		       256,                       // size of output buffer
		       &num_levels,               // number of bytes returned
		       NULL )                     // OVERLAPPED structure
      == FALSE ){
    return false;
  }
  if( num_levels==0 ){
    cerr<<"ERROR: hardware does not support LCD brightness control!"<<endl;
  }
  unsigned int i;
  for( i=0; i<num_levels; i++ ){
    cout << "level["<<i<<"]="<<buf[i]<<endl;
  }
  /*
  DeviceIoControl( lcd_handle,                 // handle to device
		  IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS, // dwIoControlCode
		  (LPVOID) lpInBuffer,         // input buffer
		  (DWORD) nInBufferSize,       // size of the input buffer
		  NULL,                        // lpOutBuffer
		  0,                           // nOutBufferSize 
		  (LPDWORD) lpBytesReturned,   // number of bytes returned
		  (LPOVERLAPPED) lpOverlapped  // OVERLAPPED structure
		  );
  */
  return true;
#else
  cout << "Monitor sleep unimplemented for this platform."<<endl;
  return false;
#endif
}

duration_t SysInterface::idle_seconds(){
#if defined PLATFORM_LINUX
  Display *dis;
  XScreenSaverInfo *info;
  dis = XOpenDisplay((char *)0);
  Window win = DefaultRootWindow(dis);
  info = XScreenSaverAllocInfo();
  XScreenSaverQueryInfo( dis, win, info );
  duration_t ret = (info->idle)/1000.0;
  XCloseDisplay( dis );
  return ret;
#elif defined PLATFORM_WINDOWS
  LASTINPUTINFO info;
  info.cbSize = sizeof(LASTINPUTINFO); // prepare info struct
  if( !GetLastInputInfo( &info ) )
    cerr<<"ERROR: could not getLastInputInfo"<<endl;
  unsigned int idleTicks = GetTickCount() - info.dwTime; // compute idle time
  return (idleTicks/1000); // returns idle time in seconds
#else
  return 60;
#endif
}

void SysInterface::sleep( duration_t duration ){
  /* use portaudio's convenient portable sleep function */
  Pa_Sleep( (int)(duration*1000) );
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

long SysInterface::current_time(){
#if defined PLATFORM_WINDOWS
  SYSTEMTIME st;
  FILETIME ft; // in 100 ns increments
  GetSystemTime( &st );
  SystemTimeToFileTime( &st, &ft );
  unsigned long long high_bits = ft.dwHighDateTime;
  unsigned long long t = ft.dwLowDateTime+(high_bits<<32);
  return t/10000000;
#else
  time_t ret;
  return time(&ret);
#endif
}

template <class msg>
bool SysInterface::log( msg message ){
  string logfile = SysInterface::config_dir() + LOG_FILENAME;
  ofstream logstream( logfile.c_str(), ios::app ); // append mode
  logstream << SysInterface::current_time() << ": " << message << endl;
  logstream.close();
  return true;
}


string SysInterface::config_dir(){
#if defined PLATFORM_WINDOWS
  //char[80] cbuf;
  //PHANDLE process_token;
  //if( !OpenProcessToken( TOKEN_READ
  //if( !GetUserProfileDirectory( ) )
  //  cerr << "ERROR: couldn't obtain home directory name"<<endl;
  char buf[MAX_PATH];
  SHGetFolderPath( HWND_TOPMOST, CSIDL_APPDATA, 
		   NULL, SHGFP_TYPE_CURRENT, buf );
  return string( buf ) + '\\';
  //return "C:\\";
#else
  return string( getenv("HOME") ) + '/';
#endif
}

#ifdef PLATFORM_WINDOWS
BOOL windows_term_handler( DWORD fdwCtrlType ) {
  switch( fdwCtrlType ) { 
  case CTRL_C_EVENT: 
  case CTRL_CLOSE_EVENT: 
  case CTRL_BREAK_EVENT: 
  case CTRL_LOGOFF_EVENT: 
  case CTRL_SHUTDOWN_EVENT: 
    SysInterface::log( "quit" );
    exit( 0 );
    return true;   
  // Pass other signals to the next handler.  (by returning false)
  default: 
    return false; 
  }  
}
#else
void posix_term_handler( int signum ){
  SysInterface::log( "quit" );
  exit( 0 );
}
#endif


void SysInterface::register_term_handler(){
#ifdef PLATFORM_WINDOWS
  if(!SetConsoleCtrlHandler( (PHANDLER_ROUTINE) windows_term_handler, TRUE ))
    cerr << "register handler failed!" <<endl;
#else
  signal( SIGINT, posix_term_handler );
  signal( SIGQUIT, posix_term_handler );
  signal( SIGTERM, posix_term_handler );
  signal( SIGKILL, posix_term_handler );
#endif
}

long get_log_start_time( ){
  string logfilename = SysInterface::config_dir() + LOG_FILENAME;
  ifstream logfile( logfilename.c_str() );
  // default to very large value so that if error, phonehome does not occur.
  long time = numeric_limits<long>::max();
  if( logfile.good() ){
    logfile >> time;
  }
  return time;
}

bool log_freq_response( AudioDev & audio ){
  freq_response f = test_freq_response( audio );
  ostringstream log_msg;
  unsigned int i;
  log_msg << "response ";
  for( i=0; i<f.size(); i++ ){
    log_msg << f[i].first << ':' << f[i].second << ' ';
  }
  return SysInterface::log( log_msg.str() );
}

void power_management( AudioDev & audio, Config & conf ){
  // buffer duration is one second, but actually it just needs to be a multiple
  // of the ping_period.
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
    }

    // record and process
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;
    SysInterface::log( s );

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


/** poll is a simplified version of the power management loop wherein we just
    constantly take readings */
void poll( AudioDev & audio, Config & conf ){
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it 
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* s = audio.nonblocking_play_loop( ping );
  while( 1 ){
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;
  }
  AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free up dev
}


int main( int argc, char* argv[] ){
  // parse commandline arguments
  bool do_poll=false, echo=false, response=false;
  int i;
  frequency poll_freq = 0;
  for( i=1; i<argc; i++ ){
    if( string(argv[i]) == string("--help") ){
      cout<<"usage is "<<argv[0]<<" [ --help | --poll [freq] | --echo | --response | --phonehome ]"<<endl;
      exit(0);
    }else if( string(argv[i]) == string("--poll") ){
      do_poll=true;
    }else if( string(argv[i-1]) == string("--poll") && argv[i][0] != '-' ){
      poll_freq = atof( argv[i] );
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
  // Try to Load config file.  If config file does not exist, this will prompt
  // the user for first-time setup.  load() returns true if new config created
  if( conf.load( my_audio, SysInterface::config_dir()+CONFIG_FILENAME ) ){
    log_freq_response( my_audio );
    // send initial configuration home
    if( conf.allow_phone_home ) SysInterface::phone_home();
  }


  if( echo ){
    // This next block is a debugging audio test
    duration_t test_length = 3;
    cout<<"First, we are going to record and playback to test the audio HW."<<endl;
    cout<<"recording audio..."<<endl;
    AudioBuf my_buf = my_audio.blocking_record( test_length );
    cout<<"playing back the recording..."<<endl;
    PaStream* s = my_audio.nonblocking_play( my_buf ); 
    SysInterface::sleep( test_length ); // give the audio some time to play
    AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free dev
    return 0;
  }
  
  if( response ){
    test_freq_response( my_audio );
    return 0;
  }

  // choose operating mode
  if( do_poll ){
    if( poll_freq != 0 ) conf.ping_freq = poll_freq;
    poll( my_audio, conf );
  }else{
    power_management( my_audio, conf );
  }
  return 0;
}
