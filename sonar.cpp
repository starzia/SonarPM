/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * see README.txt
 */
#include "sonar.hpp"
#include "SimpleIni.h" // for config files
#include <iostream>
#include <cmath>
#include <vector>
#include <sstream>
#include <exception>
#include <iostream>// for logfile writing
#include <fstream> // for logfile writing

#define RECORDING_PERIOD (10.0) 
#define WINDOW_SIZE (0.1) // sliding window size
#define BARTLETT_WINDOWS (10) // num windows in bartlett's method
int SAMPLE_RATE;
#define FRAMES_PER_BUFFER (32768) // PortAudio buf size.  The examples use 256.
#define CONFIG_FILENAME ".sonarPM.cfg"
#define LOG_FILENAME ".sonarPM.log"
#define FTP_SERVER "belmont.eecs.northwestern.edu"
#define FTP_USER "sonar"
#define FTP_PASSWD "ppinngg"
#define SLEEP_TIME (0.2) // sleep time between idleness checks
#define IDLE_THRESH (5) // don't activate sonar until idle for this long
#define IDLE_SAFETYNET (300) // assume that if idle for this long, user is gone
#define DEFAULT_PING_FREQ (22000)
#define DYNAMIC_THRESH_FACTOR (1.3) // how rapidly does dynamic threshold move

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
#elif defined PLATFORM_MAC
#endif
#ifndef PLATFORM_WINDOWS
#include <stdlib.h> //getenv, etc.
#include <signal.h>
#endif

using namespace std;

AudioBuf::AudioBuf( ){
  this->num_samples=0;
  this->data = NULL;
}

AudioBuf::AudioBuf( string filename ){
  cerr << "audio file read unimplemented"<<endl;
}

AudioBuf::AudioBuf( duration_t length ){
  this->num_samples = (unsigned int)ceil( length * SAMPLE_RATE );
  this->data = (sample_t*)malloc( sizeof(sample_t)*this->num_samples );
}

AudioBuf::~AudioBuf(){
  if( this->data ) free( this->data ); // free sample array
}

sample_t& AudioBuf::operator[]( unsigned int index ) const{
  return this->data[index];
}

void AudioBuf::prepend_silence( duration_t silence_duration ){
  cerr << "prepend_silence unimplemented"<<endl;
}

duration_t AudioBuf::get_length() const{
  return ( this->num_samples / SAMPLE_RATE );
}
  
unsigned int AudioBuf::get_num_samples() const{ 
  return this->num_samples;
}

sample_t* AudioBuf::get_array() const{
  return this->data;
}

AudioBuf* AudioBuf::window( duration_t length, duration_t start ) const{
  unsigned int start_index;
  start_index = (unsigned int)floor( start * SAMPLE_RATE );
  AudioBuf* ret = new AudioBuf( length );
  unsigned int i;
  for( i=0; i < ret->get_num_samples(); i++ ){
    (*ret)[i] = this->data[start_index+i];
  }
  return ret;
}
  
AudioBuf AudioBuf::repeat( int repetitions ) const{
  AudioBuf ret = AudioBuf( this->get_length() * repetitions );
  unsigned int i;
  for( i=0; i < ret.get_num_samples(); i++ ){
    ret[i] = this->data[ i % this->get_num_samples() ];
  }
  return ret;
}

bool AudioBuf::write_to_file( string filename ) const{
  cerr << "audio file writing unimplemented"<<endl;
  return true;
}

AudioRequest::AudioRequest( const AudioBuf & buf ){
  this->progress_index = 0; // set to zero so playback starts at beginning
  this->audio = buf;
}

/** this constructor is used for recording, so we have to allocate an new 
    audio buffer */
AudioRequest::AudioRequest( duration_t len ){
  this->progress_index = 0; // set to zero so playback starts at beginning
  this->audio = *(new AudioBuf( len ));  
}

bool AudioRequest::done(){
  return this->progress_index > this->audio.get_num_samples();
}

AudioDev::AudioDev(){
  // Initialize PortAudio
  check_error( Pa_Initialize() );
}

AudioDev::~AudioDev(){
  // close PortAudio
  check_error( Pa_Terminate() );
}

void AudioDev::choose_device( unsigned int in_dev_num, 
			      unsigned int out_dev_num ){
  // now create the device definition
  this->in_params.channelCount = 1;
  this->in_params.device = in_dev_num;
  this->in_params.sampleFormat = paFloat32;
  this->in_params.suggestedLatency = Pa_GetDeviceInfo(in_dev_num)->defaultLowInputLatency ;
  this->in_params.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field

  this->out_params.channelCount = 2;
  this->out_params.device = out_dev_num;
  this->out_params.sampleFormat = paFloat32;
  this->out_params.suggestedLatency = Pa_GetDeviceInfo(out_dev_num)->defaultLowOutputLatency ;
  this->out_params.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field

  // set global SAMPLE_RATE
  int i;
  const int rates[6] = {96000, 48000, 44100, 22050, 16000, 8000};
  for( i=0; i<6; i++ ){
    if( Pa_IsFormatSupported( &this->in_params,NULL,rates[i] ) == paNoError &&
	Pa_IsFormatSupported( NULL,&this->out_params,rates[i] ) == paNoError ){
      SAMPLE_RATE=rates[i];
      cerr << "Sample rate = "<<SAMPLE_RATE<<"Hz"<<endl;
      return;
    }
  }
  cerr<<"ERROR: no supported sample rate found!"<<endl;
}

pair<unsigned int,unsigned int> AudioDev::prompt_device(){
  // get the total number of devices
  int numDevices = Pa_GetDeviceCount();
  if( numDevices < 0 ){
    printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
  }

  // get data on each of the devices
  cout << "These are the available audio devices:" << endl;
  const PaDeviceInfo *deviceInfo;
  int i;
  for( i=0; i<numDevices; i++ ){
    deviceInfo = Pa_GetDeviceInfo( i );
    cout << i << ": ";
    cout << deviceInfo->name << endl;
  }

  unsigned int in_dev_num, out_dev_num;
  // prompt user on which of the above devices to use
  cout << endl << "Please enter an input device number: " << endl;
  cin >> in_dev_num;
  cout << endl << "Please enter an output device number: " << endl;
  cin >> out_dev_num;

  return make_pair( in_dev_num, out_dev_num );
}

/** here, we are copying data from the AudioBuf (stored in userData) into
    outputBuffer and then updating the progress_index so that we know where
    to start the next time this callback function is called */
int AudioDev::player_callback( const void *inputBuffer, void *outputBuffer,
			       unsigned long framesPerBuffer,
			       const PaStreamCallbackTimeInfo* timeInfo,
			       PaStreamCallbackFlags statusFlags,
			       void *userData ){
  /* Cast data passed through stream to our structure. */
  AudioRequest *req = (AudioRequest*)userData; 
  sample_t *out = (sample_t*)outputBuffer;
  (void) inputBuffer; /* Prevent unused variable warning. */

  unsigned int i;
  for( i=req->progress_index; i < req->progress_index + framesPerBuffer; i++ ){
    if( i < req->audio.get_num_samples() ){
      *out++ = req->audio[i];  /* left */
    }else{
      *out++ = 0; // play silence if we've reqched end of buffer
    }
    *out++ = 0;  /* right */
  }
  req->progress_index = i; // update progress index
  // we would return 1 when playback is complete (ie when we want the stream
  // to die), otherwise return 0
  return req->done();
}

/** Similiar to player_callback, but "wrap around" buffer indices */
int AudioDev::oscillator_callback( const void *inputBuffer, void *outputBuffer,
				   unsigned long framesPerBuffer,
				   const PaStreamCallbackTimeInfo* timeInfo,
				   PaStreamCallbackFlags statusFlags,
				   void *userData ){
  /* Cast data passed through stream to our structure. */
  AudioRequest *req = (AudioRequest*)userData; 
  sample_t *out = (sample_t*)outputBuffer;
  (void) inputBuffer; /* Prevent unused variable warning. */

  unsigned int i, total_samples = req->audio.get_num_samples();
  for( i=0; i<framesPerBuffer; i++ ){
    *out++ = req->audio[ ( req->progress_index + i ) % total_samples ]; // left
    *out++ = 0; // right
  }
  // update progress index
  req->progress_index = ( req->progress_index + i ) % total_samples; 
  return 0; // returning 0 makes the stream stay alive.
}

/** here, we are copying data from the input buffer into the AudioBuf.
    and then updating the progress_index so that we know where
    to start the next time this callback function is called */
int AudioDev::recorder_callback( const void *inputBuffer, void *outputBuffer,
				 unsigned long framesPerBuffer,
				 const PaStreamCallbackTimeInfo* timeInfo,
				 PaStreamCallbackFlags statusFlags,
				 void *userData ){
  /* Cast data passed through stream to our structure. */
  AudioRequest *req = (AudioRequest*)userData; 
  sample_t *in = (sample_t*)inputBuffer;
  (void) outputBuffer; /* Prevent unused variable warning. */

  unsigned int i;
  for( i=req->progress_index; i < req->progress_index + framesPerBuffer; i++ ){
    if( i < req->audio.get_num_samples() ){
      req->audio[i] = *in++;  /* there is only one channel */
    }
  }
  req->progress_index = i; // update progress index
  // we would return 1 when playback is complete (ie when we want the stream
  // to die), otherwise return 0
  return req->done();
}

PaStream* AudioDev::nonblocking_play( const AudioBuf & buf ){
  PaStream *stream;
  AudioRequest *play_request = new AudioRequest( buf );
  /* Open an audio I/O stream. */
  check_error( Pa_OpenStream( 
	 &stream,
	 NULL,          /* no input channels */
	 &(this->out_params),
	 SAMPLE_RATE,
	 FRAMES_PER_BUFFER,
	 paNoFlag,
	 AudioDev::player_callback, /* this is your callback function */
	 play_request ) ); /*This is a pointer that will be passed to
			     your callback*/
  // start playback
  check_error( Pa_StartStream( stream ) );
  // caller is responsible for freeing stream
  return stream;
}

PaStream* AudioDev::nonblocking_play_loop( const AudioBuf & buf ){
  PaStream *stream;
  AudioRequest *play_request = new AudioRequest( buf );
  /* Open an audio I/O stream. */
  check_error( Pa_OpenStream( 
	 &stream,
	 NULL,          /* no input channels */
	 &(this->out_params),
	 SAMPLE_RATE,
	 FRAMES_PER_BUFFER,   /* frames per buffer */
	 paNoFlag,
	 AudioDev::oscillator_callback, /* this is your callback function */
	 play_request ) ); /*This is a pointer that will be passed to
			     your callback*/
  // start playback
  check_error( Pa_StartStream( stream ) );
  // caller is responsible for freeing stream
  return stream;
}

AudioBuf AudioDev::blocking_record( duration_t duration ){
  PaStream *stream;
  AudioRequest *rec_request = new AudioRequest( duration );
  /* Open an audio I/O stream. */
  check_error( Pa_OpenStream (
	 &stream, 
	 &(this->in_params),
	 NULL,       /* no output channels */
	 SAMPLE_RATE,
	 FRAMES_PER_BUFFER,        /* frames per buffer */
	 paNoFlag, 
	 AudioDev::recorder_callback, /* this is your callback function */
	 rec_request ) ); /*This is a pointer that will be passed to
			     your callback*/
  // start recording
  check_error( Pa_StartStream( stream ) );
  // wait until done
  while( !rec_request->done() ) SysInterface::sleep( 0.1 );
  check_error( Pa_CloseStream( stream ) ); // free up stream resource
  return rec_request->audio;
}

/** This function could probably be more accurately (re synchronization)
    implemented by creating a new duplex stream.  For sonar purposes,
    it shouldn't be necessary*/
AudioBuf AudioDev::recordback( const AudioBuf & buf ){
  // play audio
  PaStream* s = nonblocking_play( buf );
  // record echo
  AudioBuf ret = blocking_record( buf.get_length() );
  AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free up dev
  return ret;
}

/** @return float[2] = { best_freq, best_gain } */
float* AudioDev::test_freq_response(){
  cout << "Please wait while the system is calibrated..."<<endl;

  // record silence (as a reference point)
  AudioBuf silence = tone( RECORDING_PERIOD, 0 );
  AudioBuf silence_rec = this->recordback( silence );
  // record white noise
  AudioBuf noise = gaussian_white_noise( RECORDING_PERIOD );
  AudioBuf noise_rec = this->recordback( noise );

  // now choose highest freq with energy reading well above that of silence
  float scaling_factor = 1.02; // freq bin scaling factor
  frequency best_freq = 0;
  float best_gain = -999999;
  frequency freq, lowest_freq = 20000, highest_freq = SAMPLE_RATE/2; //Nyquist
  cout << "Frequency response:"<<endl;
  for( freq = lowest_freq; 
       freq <= highest_freq;
       freq *= scaling_factor ){
    float gain = measure_stats(noise_rec,freq).mean / 
                                          measure_stats(silence_rec,freq).mean;
    cout << freq <<"Hz  \t"<<gain<<endl;
    if( gain > best_gain ){
      best_gain = gain;
      best_freq = freq;
    }
  }
  float* ret = new float[2];
  ret[0] = best_freq; ret[1] = best_gain;
  return ret;
}

inline void AudioDev::check_error( PaError err ){
  if( err != paNoError )
    cerr << "PortAudio error: " << Pa_GetErrorText( err ) << endl;
}

/** call calibration functions to create a new configuration */
Config::Config( AudioDev & audio, string filename ){
  this->filename = filename;
  // try to load from a data file
  CSimpleIniA ini(false,false,false);
  SI_Error rc = ini.LoadFile( filename.c_str() );
  if (rc < 0){
    // if file open unsuccessful, then run calibration
    cerr<< "Unable to open config file "<<filename<<endl;
    cerr<< "A new configuration file will now be created."<<endl;

    pair<unsigned int,unsigned int> devices = audio.prompt_device();
    this->rec_dev = devices.first;
    this->play_dev = devices.second;
    // set audio object to use the desired devices
    audio.choose_device( this->rec_dev, this->play_dev );
    this->warn_audio_level( audio );
    //this->choose_ping_freq( audio );
    this->ping_freq = DEFAULT_PING_FREQ;
    this->choose_ping_threshold( audio, this->ping_freq );
    //this->choose_phone_home( );
    this->allow_phone_home=true;
    
    // write configuration to file
    this->write_config_file();
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
  
  float* best = audio.test_freq_response();
  float best_freq = best[0];
  float best_gain = best[1];
  delete best;

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
  cerr << "choose_phone_home() unimplemented"<<endl;
}
  
void Config::warn_audio_level( AudioDev & audio ){
  //TODO: cerr << "warn_audio_level() unimplemented"<<endl;
}

bool SysInterface::phone_home(){
  SysInterface::log( "phonehome" );
  cerr << "Sending log file to Northwestern University server..."<<endl;
#if defined PLATFORM_WINDOWS
  HINTERNET hnet = InternetOpen( "sonar", INTERNET_OPEN_TYPE_PRECONFIG,
				 NULL,NULL,NULL);
  hnet = InternetConnect( hnet, FTP_SERVER,
			  INTERNET_DEFAULT_FTP_PORT, FTP_USER, FTP_PASSWD,
			  INTERNET_SERVICE_FTP, NULL, NULL );
  string logfile = SysInterface::config_dir()+LOG_FILENAME;
  bool ret = FtpPutFile( hnet, logfile.c_str(), "windows.log",
			 FTP_TRANSFER_TYPE_BINARY, NULL );
  InternetCloseHandle( hnet );
  return ret;
#else
  string command = "curl -T "+SysInterface::config_dir()+LOG_FILENAME
    +" ftp://"+FTP_USER+':'+FTP_PASSWD+'@'+FTP_SERVER+'/';
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

bool SysInterface::log( string message ){
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

AudioBuf tone( duration_t duration, frequency freq, duration_t delay, 
	       unsigned int fade_samples ){
  // create empty buffer
  AudioBuf buf = AudioBuf( duration );
  unsigned int i, end_i = buf.get_num_samples();
  for( i=0; i < end_i; i++ ){
    buf[i] = sin( 2*M_PI * freq * i / SAMPLE_RATE );
  }
  // fade in and fade out to prevent 'click' sound
  for( i=0; i<fade_samples; i++ ){
    float attenuation = 1.0*i/fade_samples;
    buf[i] = attenuation * buf[i];
    buf[end_i-1-i] = attenuation * buf[end_i-1-i];
  }
  return buf;
}

AudioBuf gaussian_white_noise( duration_t duration ){
  // create empty buffer
  AudioBuf buf = AudioBuf( duration );
  unsigned int i, end_i = buf.get_num_samples();

  /* inner loop taken from http://www.musicdsp.org/showone.php?id=113 */
  for( i=0; i < end_i; i++ ){
    float R1 = (float) rand() / (float) RAND_MAX;
    float R2 = (float) rand() / (float) RAND_MAX;
    buf[i] = (float) sqrt( -2.0f * log( R1 )) * cos( 2.0f * M_PI * R2 );
  }
  return buf;
}

/** statistical mean */
template<class precision> precision mean( const vector<precision> & arr ){
  unsigned int i;
  precision mean=0;
  for( i=0; i<arr.size(); i++ )
    mean += arr[i];
  return mean / arr.size();
}
/** statistical variance */
template<class precision> precision variance( const vector<precision> & arr ){
  unsigned int i;
  precision mu = mean( arr );
  precision var = 0;
  for( i=0; i<arr.size(); i++ )
    var += ( arr[i] - mu ) * ( arr[i] - mu );
  return var;
}
/** average inter-sample absolute difference */
template<class precision> precision delta( const vector<precision> & arr ){
  unsigned int i;
  precision delta=0;
  for( i=1; i<arr.size(); i++ )
    delta += abs( arr[i] - arr[i-1] );
  return delta / arr.size();
}

ostream& operator<<(ostream& os, const Statistics& s){
  os<< "{mean:" <<s.mean<< "\tvar:" <<s.variance<< "\tdelta:" <<s.delta<< '}';
  return os;
}
ostream& operator<<(ostream& os, Statistics& s){
  os<< "{mean:" <<s.mean<< "\tvar:" <<s.variance<< "\tdelta:" <<s.delta<< '}';
  return os;
}

/** Geortzel's algorithm for finding the energy of a signal at a certain
    frequency.  Note that end_index is one past the last valid value. 
    Normalized frequency is measured in cycles per sample: (F/sample_rate)*/
template<class indexable,class precision> 
precision goertzel( const indexable & arr, unsigned int start_index, 
		    unsigned int end_index, precision norm_freq ){
  precision s_prev = 0;
  precision s_prev2 = 0;
  precision coeff = 2 * cos( 2 * M_PI * norm_freq );
  unsigned int i;
  for( i=start_index; i<end_index; i++ ){
    precision s = arr[i] + coeff*s_prev - s_prev2;
    s_prev2 = s_prev;
    s_prev = s;
  }
  // return the energy (power)
  return s_prev2*s_prev2 + s_prev*s_prev - coeff*s_prev2*s_prev;
}


//16 byte vector of 4 floats
typedef float v4sf __attribute__ ((vector_size(16)));
//16 byte vector of 4 uints
typedef unsigned int v4sd __attribute__ ((vector_size(16))); 

typedef union f4vector { // wrapper for two alternative vector representations
  v4sf v;
  float f[4];
} f4v; 

typedef union d4vector { // wrapper for two alternative vector representations
  v4sd v;
  unsigned int f[4];
} d4v;

/** vectorized version of goertzel that operates on four consecutive windows
    simultaneously.  ie, operates on arr in the index range:
    [ start_index, start_index + 4*window_size ) */
template<class indexable>
f4v quad_goertzel( const indexable & arr, unsigned int start_index, 
		   unsigned int window_size, float norm_freq ){
  const v4sd one = {1, 1, 1, 1};
  const v4sf zero = {0.0, 0.0, 0.0, 0.0};

  f4v ret, s_prev, s_prev2;
  s_prev.v = s_prev2.v = zero;
  f4v coeff;
  coeff.f[0]=coeff.f[1]=coeff.f[2]=coeff.f[3]= 2 * cos( 2 * M_PI * norm_freq );

  d4v i; // set starting indices
  i.f[0] = start_index + 0*window_size;
  i.f[1] = start_index + 1*window_size;
  i.f[2] = start_index + 2*window_size;
  i.f[3] = start_index + 3*window_size;
  for(; i.f[0]<start_index+window_size; i.v = i.v+one ){
    f4v s,a;
    a.f[0] = arr[i.f[0]];
    a.f[1] = arr[i.f[1]];
    a.f[2] = arr[i.f[2]];
    a.f[3] = arr[i.f[3]];
    s.v = a.v + coeff.v * s_prev.v - s_prev2.v;
    s_prev2 = s_prev;
    s_prev = s;
  }
  // return the energy (power)
  ret.v = s_prev2.v * s_prev2.v + s_prev.v * s_prev.v 
    - coeff.v * s_prev2.v * s_prev.v;
  return ret;
}


/** Bartlett's method is the mean of several runs of Goertzel's algorithm in
    consecutive windows. */
template<class indexable,class precision> 
precision bartlett( const indexable & arr, unsigned int start_index, 
		    unsigned int end_index, precision norm_freq, 
		    unsigned int num_windows ){
  vector<float> energies;
  unsigned int i, window_size = (end_index-start_index)/num_windows;

  // for each window
  for( i=0; i<num_windows; i++ ){
    if( i > num_windows-4 ){
      // if less than four windows left, do one at a time.
      // be careful not to go beyond end_index
      unsigned int j = (end_index<(i+1)*window_size)? 
	end_index: start_index+(i+1)*window_size;
      energies.push_back( goertzel( arr, start_index + i*window_size, 
				    j, norm_freq) );
    }else{
      // otherwise do four at once.
      f4v res = quad_goertzel( arr, start_index + i*window_size,
			       window_size, norm_freq );
      energies.push_back( res.f[0] );
      energies.push_back( res.f[1] );
      energies.push_back( res.f[2] );
      energies.push_back( res.f[3] );
      i+=3; //skip ahead
    }  
  }
  return mean( energies );
}
template<class precision> 
precision bartlett( const AudioBuf & arr, precision norm_freq ){
  return bartlett(arr, 0, arr.get_num_samples(), norm_freq, BARTLETT_WINDOWS);
}


Statistics measure_stats( const AudioBuf & buf, frequency freq ){
  vector<float> energies;

  unsigned int start, N = (unsigned int)ceil(WINDOW_SIZE*SAMPLE_RATE);
  // Calculate echo intensities: use a sliding non-overlapping window
  // TODO: parallelize window computations using SIMD instructions.
  for( start=0; start + N < buf.get_num_samples(); start += N ){
    energies.push_back( bartlett(buf,start,start+N,
				 (float)freq/SAMPLE_RATE,BARTLETT_WINDOWS) );
  }

  // calculate statistics
  Statistics ret;
  ret.mean = mean( energies );
  ret.variance = variance( energies );
  ret.delta = delta( energies );
  return ret;
}

long get_log_start_time( ){
  cerr << "log_start_time() unimplemented"<<endl;
  return 0;
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
  while( 1 ){
    SysInterface::wait_until_idle();
    AudioDev::check_error( Pa_StartStream( strm ) ); // resume ping

    //-- THRESHOLD RAISING
    if( SysInterface::idle_seconds() > IDLE_SAFETYNET ){
      // if we've been tring to detect use for too long, then this probably
      // means that the threshold is too low.
      cout << "False attention detected." <<endl;
      conf.threshold *= DYNAMIC_THRESH_FACTOR;
      conf.write_config_file(); // config save changes
    }

    // record and process
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    AudioDev::check_error( Pa_StopStream( strm ) ); // stop ping
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;

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
	conf.threshold /= DYNAMIC_THRESH_FACTOR;
	conf.write_config_file(); // config save changes
      }
    }
  }
  // TODO: we never get here, so a signal handler needs to free the dev.
  AudioDev::check_error( Pa_CloseStream( strm ) ); // close ping stream
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

  // set audio device, loading from config file if present
  AudioDev my_audio = AudioDev();
  Config conf( my_audio, SysInterface::config_dir()+CONFIG_FILENAME );

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
    my_audio.test_freq_response();
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
