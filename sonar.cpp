/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel and fftw-devel
 */
#include "sonar.hpp"
#include "SimpleIni.h" // for config files
#include <iostream>
#include <cmath>
#include <vector>
#include <sstream>

#define FFT_POINTS (1024)
#define FFT_FREQUENCIES (FFT_POINTS/2)
#define TONE_LENGTH (0.3) /* sonar ping length */
#define WINDOW_SIZE (0.01) /* this is for welch */
#define SAMPLE_RATE (44100)
#define CONFIG_FILENAME "/home/steve/.sonarPM/sonarPM.cfg"
#define LOG_FILENAME "/home/steve/.sonarPM/log.txt"
#define PHONE_HOME_ADDR "storage@stevetarzia.com"

// One of the following should be defined to activate platform-specific code.
//#define PLATFORM_LINUX
//#define PLATFORM_WINDOWS
//#define PLATFORM_MAC

#ifdef PLATFORM_LINUX
/* The following headers are provided by these packages on a Redhat system:
 * libX11-devel, libXext-devel, libScrnSaver-devel
 * There are also some additional dependencies for libX11-devel */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/scrnsaver.h>
#endif //def PLATFORM_LINUX

using namespace std;

AudioBuf::AudioBuf( ){
  this->num_samples=0;
  this->data = NULL;
}

AudioBuf::AudioBuf( string filename ){
  cerr << "unimplemented\n";
}

AudioBuf::AudioBuf( duration_t length ){
  this->num_samples = ceil( length * SAMPLE_RATE );
  this->data = (fft_array_t)fftwf_malloc( sizeof(sample_t)*this->num_samples );
}

AudioBuf::~AudioBuf(){
  //TODO: I don't know why the following is causing segfaults
  //if( this->data ) fftwf_free( this->data ); // free sample array
}

sample_t* AudioBuf::operator[]( unsigned int index ) const{
  return this->data+index;
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

fft_array_t AudioBuf::get_array() const{
  return this->data;
}

AudioBuf* AudioBuf::window( duration_t length, duration_t start ) const{
  unsigned int start_index;
  start_index = floor( start * SAMPLE_RATE );
  AudioBuf* ret = new AudioBuf( length );
  unsigned int i;
  for( i=0; i < ret->get_num_samples(); i++ ){
    *((*ret)[i]) = this->data[start_index+i];
  }
  return ret;
}
  
AudioBuf AudioBuf::repeat( int repetitions ) const{
  AudioBuf ret = AudioBuf( this->get_length() * repetitions );
  unsigned int i;
  for( i=0; i < ret.get_num_samples(); i++ ){
    *(ret[i]) = this->data[ i % this->get_num_samples() ];
  }
  return ret;
}

bool AudioBuf::write_to_file( string filename ) const{
  cerr << "unimplemented\n";
}

AudioRequest::AudioRequest( const AudioBuf & buf ){
  this->progress_index = 0; // set to zero so playback starts at beginning
  this->audio = buf;
}

/** this constructor is used for recording, so we have to allocate an new 
    audio buffer */
AudioRequest::AudioRequest( duration_t len ){
  this->progress_index = 0; // set to zero so playback starts at beginning
  this->audio = AudioBuf( len );  
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

  // record this choice
  this->choose_device( in_dev_num, out_dev_num );
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
      *out++ = *(req->audio[i]);  /* left */
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
    *out++ = *(req->audio[ ( req->progress_index + i ) % total_samples ]);  /* left */
    *out++ = 0;  /* right */
  }
  req->progress_index = i; // update progress index
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
      *(req->audio[i]) = *in++;  /* there is only one channel */
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
Config::Config( AudioDev & audio ){
  pair<unsigned int,unsigned int> devices = audio.prompt_device();
  this->rec_dev = devices.first;
  this->play_dev = devices.second;
  this->warn_audio_level( audio );
  this->choose_ping_freq( audio );
  this->choose_ping_threshold( audio );
  //this->choose_phone_home( );
  this->allow_phone_home=true;
}

Config::Config( string filename ){
  // load from a data file                                                    
  CSimpleIniA ini(false,false,false);
  SI_Error rc = ini.LoadFile( filename.c_str() );
  if (rc < 0){
    cerr<< "error opening config file "<<filename<<endl;
  }
  
  // get the key values 
  stringstream ss;
  ss << ini.GetValue("general","phone_home" );
  ss >> this->allow_phone_home;
  ss << ini.GetValue("general","recording_device" );
  ss >> this->rec_dev;
  ss << ini.GetValue("general","playback_device" );
  ss >> this->play_dev;
  ss << ini.GetValue("calibration","frequency" );
  ss >> this->ping_freq;
  ss << ini.GetValue("calibration","threshold" );
  ss >> this->threshold;
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
    cerr<< "error opening config file "<<filename<<endl;
    return false;
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
  float scaling_factor = 0.95;
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
  
void Config::choose_ping_threshold( AudioDev & audio ){
  cerr << "unimplemented\n";
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
  cerr << "unimplemented\n";
}

bool phone_home(){
  Emailer email( PHONE_HOME_ADDR );
  email.phone_home( LOG_FILENAME );
}

bool SysInterface::sleep_monitor(){
  cerr << "unimplemented\n";
}

duration_t SysInterface::idle_seconds(){
#ifdef PLATFORM_LINUX
  Display *dis;
  XScreenSaverInfo *info;
  dis = XOpenDisplay((char *)0);
  Window win = DefaultRootWindow(dis);
  info = XScreenSaverAllocInfo();
  XScreenSaverQueryInfo( dis, win, info );
  return (info->idle)/1000;
#endif //PLATFORM_LINUX
#ifdef PLATFORM_WINDOWS
#endif //PLATFORM_WINDOWS
#ifdef PLATFORM_MAC
#endif //PLATFORM_MAC
}

void SysInterface::sleep( duration_t duration ){
  /* use portaudio's convenient portable sleep function */
  Pa_Sleep( (int)(duration*1000) );
}

bool SysInterface::log( string message ){}

AudioBuf tone( duration_t duration, frequency freq, duration_t delay, 
	       unsigned int fade_samples ){
  // create empty buffer
  AudioBuf buf = AudioBuf( duration );
  unsigned int i, end_i = buf.get_num_samples();
  for( i=0; i < end_i; i++ ){
    *(buf[i]) = sin( 2*M_PI * freq * i / SAMPLE_RATE );
  }
  // fade in and fade out to prevent 'click' sound
  for( i=0; i<fade_samples; i++ ){
    float attenuation = 1.0*i/fade_samples;
    *(buf[i]) = attenuation * *(buf[i]);
    *(buf[end_i-1-i]) = attenuation * *(buf[end_i-1-i]);
  }
  return buf;
}

int freq_index( frequency freq ){
  return round( (2.0*freq/SAMPLE_RATE) * FFT_FREQUENCIES );
}

/** complex absolute value (float data storage) */
inline float cabsf( float* c ){
  return sqrt(c[0]*c[0] + c[1]*c[1]);
}

ostream& operator<<(ostream& os, const Statistics& s){
  os << "{mean:" << s.mean << " var:" << s.variance << '}';
  return os;
}

/** We choose a window size to be precisely the same as the number of FFT
    points.  In other words, we do an exact DFT, and choose a window size
    small enough so that this computation is not too exensive.
    Note that there is an explanation of how to take an approximate DFT,
    that is, find the first K outputs of a real-input FFT at:
    http://www.fftw.org/pruned.html */
Statistics measure_stats( const AudioBuf & buf, frequency freq ){
  vector<float> energies;

  unsigned int i, N=FFT_POINTS;

  // use a sliding window
  duration_t start;
  for( start=0; start + N < buf.get_num_samples(); start += N ){
    // set up FFT
    // allocate buffer for this window
    float* in = (float*)fftwf_malloc( sizeof(float)*N );
    // although input is real array, output is complex.
    fftwf_complex *out = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex)*N );

    // prep FFT
    fftwf_plan p = fftwf_plan_dft_r2c_1d( N, in, out, 
					  FFTW_ESTIMATE | FFTW_PRESERVE_INPUT );
    // we must fill in input data AFTER generating plan
    for( i=0; i<N; i++ ){
      in[i] = *(buf[start+i]);
    }
    // do FFT
    fftwf_execute(p);
    float* val = out[ freq_index( freq ) ];
    //cout << "fftval1:" << val[0] <<", " <<val[1] <<endl;
    float energy = cabsf( val ); //complex absolute value (float)
    //cout << "fftval2:" << energy <<endl;
    energy *= energy; // square to get power
    //cout << "fftval3:" << energy <<endl<<endl;
    fftwf_destroy_plan(p);
    fftwf_free(out);
    fftwf_free(in);

    energies.push_back( energy );
  }

  // calculate statistics
  Statistics ret;
  ret.mean=0;
  ///cout << "energies:\n";
  for (i=0; i < energies.size(); i++){
    ///  cout << energies[i] <<endl;
    ret.mean += energies[i];
  }
  ret.mean /= energies.size();
  ret.variance=0;
  for (i=0; i < energies.size(); i++)
    ret.variance += ( energies[i] - ret.mean ) * ( energies[i] - ret.mean );
  return ret;
}

void term_handler( int signum, int frame ){
  cerr << "unimplemented\n";
}

long get_log_start_time( ){
  cerr << "unimplemented\n";
}

void power_management( frequency freq, float threshold ){
  cerr << "unimplemented\n";
}

int main( int argc, char **argv ){
  duration_t length = 3;
  AudioDev my_audio = AudioDev();

  Config conf( my_audio );
  conf.write_config_file( CONFIG_FILENAME );

  cout << "First, do some debugging\n";
  cout << "recording audio...\n";
  AudioBuf my_buf = my_audio.blocking_record( length );
  cout << "playback...\n";
  PaStream* s = my_audio.nonblocking_play( my_buf ); 
  SysInterface::sleep( length+1 ); // give the audio some time to play
  AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free up dev

  AudioBuf ping;
  int freq = 18000;
  ping = tone( 1, freq ); //duration is one second
  ///cout << "index into fft array is " << freq_index( freq ) << endl;
  cout << measure_stats( my_buf, freq ) << endl;
  cout << measure_stats( ping, 0.9*freq ) << endl;
  cout << measure_stats( ping, freq ) << endl;
  cout << measure_stats( ping, 1.1*freq ) << endl;

  cout << "Begin pinging loop at frequency of " <<freq<<"Hz"<<endl;
  while( 1 ){
    AudioBuf rec = my_audio.recordback( ping );
    cout << measure_stats( rec, freq ) << endl;
    //SysInterface::sleep( 1 ); // sleep briefly between pings
  }
  return 0;
}
