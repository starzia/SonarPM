/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel
 */
#include "sonar.hpp"
#include "SimpleIni.h" // for config files
#include <iostream>
#include <cmath>
#include <vector>
#include <sstream>
#include <exception>

#define TONE_LENGTH (1) // sonar ping length for calibration
// this is the time period over which stats are calulated
#define RECORDING_PERIOD (2.0) 
#define WINDOW_SIZE (0.1) // sliding window size
#define BARTLETT_WINDOWS (8) // num windows in bartlett's method
#define SAMPLE_RATE (44100)
#define CONFIG_FILENAME "/home/steve/.sonarPM/sonarPM.cfg"
#define LOG_FILENAME "/home/steve/.sonarPM/log.txt"
#define PHONE_HOME_ADDR "storage@stevetarzia.com"
#define SLEEP_TIME (0.2) // sleep time between idleness checks
#define IDLE_THRESH (5) // don't activate sonar until idle for this long
#define IDLE_SAFETYNET (300) // assume that if idle for this long, user is gone.
#define DYNAMIC_THRESH_FACTOR (1.3) // how rapidly does dynamic threshold move

// One of the following should be defined to activate platform-specific code.
// It is best to make this choice in the Makefile, rather than uncommenting here
//#define PLATFORM_LINUX
//#define PLATFORM_WINDOWS
//#define PLATFORM_MAC

#if defined PLATFORM_LINUX
/* The following headers are provided by these packages on a Redhat system:
 * libX11-devel, libXext-devel, libScrnSaver-devel
 * There are also some additional dependencies for libX11-devel */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/scrnsaver.h>
#elif defined PLATFORM_WINDOWS
#elif defined PLATFORM_MAC
#endif
#ifndef PLATFORM_WINDOWS
#include <ctime>
#include <stdlib.h>
#endif

using namespace std;

AudioBuf::AudioBuf( ){
  this->num_samples=0;
  this->data = NULL;
}

AudioBuf::AudioBuf( string filename ){
  cerr << "unimplemented\n";
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
  cerr << "unimplemented\n";
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
  cerr << "unimplemented\n";
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
  cout << "\nPlease enter an input device number: " << endl;
  cin >> in_dev_num;
  cout << "\nPlease enter an output device number: " << endl;
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
	 256,   /* frames per buffer */
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
	 256,   /* frames per buffer */
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
  /* Open an audio I/O stream. Opening a *default* stream means opening 
     the default input and output devices */
  check_error( Pa_OpenDefaultStream( 
	 &stream,
	 1,          /* mono input */
	 0,          /* no output channels */
	 paFloat32,  /* 32 bit floating point output */
	 SAMPLE_RATE,
	 256,        /* frames per buffer */
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

inline void AudioDev::check_error( PaError err ){
  if( err != paNoError )
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
}

/** call calibration functions to create a new configuration */
Config::Config( AudioDev & audio, string filename ){
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
    this->choose_ping_freq( audio );
    this->choose_ping_threshold( audio, this->ping_freq );
    //this->choose_phone_home( );
    this->allow_phone_home=true;
    
    // write configuration to file
    this->write_config_file( CONFIG_FILENAME );
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

bool Config::write_config_file( string filename ){
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
  SI_Error rc = ini.SaveFile( filename.c_str() );
  if (rc < 0){
    cerr<< "Error saving config file "<<filename<<endl;
    return false;
  }else{
    cerr<< "Saved config file "<<filename<<" with threshold: "
	<< this->threshold <<endl;
  }
  return true;
}

void Config::disable_phone_home(){
  this->allow_phone_home = false;
  this->write_config_file( CONFIG_FILENAME );
}
  
void Config::choose_ping_freq( AudioDev & audio ){
  // We start with 20khz and reduce it until we get a reading on the mic.
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
  ///AudioBuf silence = tone( TONE_LENGTH, 0 );
  ///AudioBuf silence_rec = audio.recordback( silence );
  frequency start_freq = 22000;
  frequency freq = start_freq;
  // below, we subtract two readings because they are logarithms
  frequency scaling_factor = 0.95;
  cout << endl
       <<"Please press <enter> and listen carefully to continue with the "<<endl
       <<"calibration."<<endl;
  cin.ignore();
  cin.get();
  while( 1 ){
    freq *= scaling_factor;
    AudioBuf blip = tone( TONE_LENGTH, freq );
    AudioBuf rec = audio.recordback( blip );
    Statistics blip_s = measure_stats( rec, freq );
    ///Statistics silence_s = measure_stats( silence_rec, freq );
    cout << "Did you just hear a high frequency ("<<round(freq)<<"Hz) tone? [yes/no]"
	 <<endl;
    string ans;
    cin >> ans;
    if( ans == "yes" or ans == "Yes" or ans == "YES" or ans=="Y" or ans=="y"){
      freq /= scaling_factor;
      break;
    }
    // at some point stop descending frequency loop
    if( freq >= start_freq ){
      cout << "Your hearing is too good (or your speakers are too noisy)."<<endl;
      cout <<"CANNOT CONTINUE"<<endl;
      SysInterface::log( "freq>=start_freq" );
      phone_home();
      exit(-1);
    }
  }
  freq = round( freq );
  cout << "chose frequency of "<<freq<<endl;
  this->ping_freq = freq;
}
  
/** current implementation just chooses a threshold in the correct neighborhood
    and relies on dynamic runtime adjustment for fine-tuning */
void Config::choose_ping_threshold( AudioDev & audio, frequency freq ){
  AudioBuf blip = tone( TONE_LENGTH, freq );
  AudioBuf rec = audio.recordback( blip );
  Statistics blip_s = measure_stats( rec, freq );
  cout << "chose preliminary threshold of "<<blip_s.delta<<endl;
  this->threshold = blip_s.delta;
}

void Config::choose_phone_home(){
  cerr << "unimplemented\n";
}
  
void Config::warn_audio_level( AudioDev & audio ){
  cerr << "unimplemented\n";
}

Emailer::Emailer( string dest_addr ){
  cerr << "unimplemented\n";
}

bool Emailer::phone_home( string filename ){
#if defined PLATFORM_LINUX
#elif defined PLATFORM_WINDOWS
#elif defined PLATFORM_MAC
#endif
  cerr << "unimplemented\n";
  return true;
}

bool phone_home(){
  Emailer email( PHONE_HOME_ADDR );
  return email.phone_home( LOG_FILENAME );
}

bool SysInterface::sleep_monitor(){
#if defined PLATFORM_LINUX
  system( "xset dpms force standby" );
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
#else
  return 99999;
#endif
}

void SysInterface::sleep( duration_t duration ){
  /* use portaudio's convenient portable sleep function */
  Pa_Sleep( (int)(duration*1000) );
}

long SysInterface::current_time(){
#if defined PLATFORM_WINDOWS
  cerr << "unimplemented\n";
  return 1;
#else
  time_t ret;
  return time(&ret);
#endif
}

bool SysInterface::log( string message ){
  cerr << "unimplemented" <<endl;
  return true;
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
  for(; i.f[0]<window_size; i.v = i.v+one ){
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

void term_handler( int signum, int frame ){
  cerr << "unimplemented\n";
}

long get_log_start_time( ){
  cerr << "unimplemented\n";
  return 0;
}

void power_management( AudioDev & audio, Config & conf ){
  // buffer duration is one second, but actually it just needs to be a multiple
  // of the ping_period.
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* s = audio.nonblocking_play_loop( ping );
  while( 1 ){
    SysInterface::sleep( SLEEP_TIME ); // don't poll idle_seconds constantly
    while( SysInterface::idle_seconds() > IDLE_THRESH ){
      // if we've been tring to detect use for too long, then this probably
      // means that the threshold is too low.
      if( SysInterface::idle_seconds() > IDLE_SAFETYNET ){
	  cout << "False attention detected." <<endl;
	  conf.threshold *= DYNAMIC_THRESH_FACTOR;
	  conf.write_config_file( CONFIG_FILENAME ); // config save changes	
      }

      AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
      Statistics s = measure_stats( rec, conf.ping_freq );
      cout << s << endl;
      if( s.delta < conf.threshold ){
	// sleep monitor
	SysInterface::sleep_monitor();
	long sleep_time = SysInterface::current_time();

	// wait until woken up again
	duration_t prev_idle_time = SysInterface::idle_seconds();
	duration_t current_idle_time = SysInterface::idle_seconds();
	while( current_idle_time >= prev_idle_time ){
	  SysInterface::sleep( SLEEP_TIME );
	  prev_idle_time = current_idle_time;
	  current_idle_time = SysInterface::idle_seconds();
	}
	// waking up too soon means that we've just irritated the user
	if( SysInterface::current_time() - sleep_time < IDLE_THRESH ){
	  cout << "False sleep detected." <<endl;
	  conf.threshold /= DYNAMIC_THRESH_FACTOR;
	  conf.write_config_file( CONFIG_FILENAME ); // config save changes
	}
      }
    }
  }
  // TODO: we never get here, so a signal handler needs to free the dev.
  AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free up dev
}


/** poll is a simplified version of the power management loop wherein we just
    constantly take readings */
void poll( AudioDev & audio, Config & conf ){
  AudioBuf ping = tone( 1, conf.ping_freq, 0,0 ); // no fade since we loop it  
  cout << "Begin pinging loop at frequency of " <<conf.ping_freq<<"Hz"<<endl;
  PaStream* s = audio.nonblocking_play_loop( ping );
  while( 1 ){
    SysInterface::sleep( SLEEP_TIME ); // don't poll idle_seconds constantly
    AudioBuf rec = audio.blocking_record( RECORDING_PERIOD );
    Statistics s = measure_stats( rec, conf.ping_freq );
    cout << s << endl;
  }
  AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free up dev
}


int main( int argc, char* argv[] ){
  // parse commandline arguments
  bool do_poll=false, debug=false;
  int i;
  for( i=1; i<argc; i++ ){
    if( string(argv[i]) == string("--help") ){
      cout<<"usage is "<<argv[0]<<" [ --help | --poll | --debug ]"<<endl;
      exit(0);
    }else if( string(argv[i]) == string("--poll") ){
      do_poll=true;
    }else if( string(argv[i]) == string("--debug") ){
      debug=true;
    }else{
      cerr << "unrecognized parameter: " << argv[i] <<endl;
    }
  }

  // set audio device, loading from config file if present
  AudioDev my_audio = AudioDev();
  Config conf( my_audio, CONFIG_FILENAME );

  if( debug ){
    // This next block is a debugging audio test
    duration_t test_length = 3;
    cout<<"First, we are going to record and playback to test the audio HW.\n";
    cout<<"recording audio...\n";
    AudioBuf my_buf = my_audio.blocking_record( test_length );
    cout<<"playing back the recording...\n";
    PaStream* s = my_audio.nonblocking_play( my_buf ); 
    SysInterface::sleep( test_length ); // give the audio some time to play
    AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free dev
  }

  // choose operating mode
  if( do_poll ){
    poll( my_audio, conf );
  }else{
    power_management( my_audio, conf );
  }
  return 0;
}
