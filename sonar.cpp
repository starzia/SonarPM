/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel and fftw-devel
 */
#include "sonar.hpp"
#include <iostream>
#include <cmath>

#ifdef PLATFORM_POSIX
/* The following headers are provided by these packages on a Redhat system:
 * libX11-devel, libXext-devel, libScrnSaver-devel
 * There are also some additional dependencies for libX11-devel */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/scrnsaver.h>
#endif //def PLATFORM_POSIX

using namespace std;

AudioBuf::AudioBuf( ){}

AudioBuf::AudioBuf( string filename ){
  cerr << "unimplemented\n";
}

AudioBuf::AudioBuf( duration_t length ){
  this->num_samples = ceil( length * SAMPLE_RATE );
  this->data = (fft_array_t)fftw_malloc( sizeof(sample_t)*this->num_samples );
}

sample_t* AudioBuf::operator[]( unsigned int index ){
  return this->data+index;
}

void AudioBuf::prepend_silence( duration_t silence_duration ){
  cerr << "unimplemented\n";
}

duration_t AudioBuf::get_length(){
  return ( this->num_samples / SAMPLE_RATE );
}
  
unsigned int AudioBuf::get_num_samples(){ 
  return this->num_samples;
}

fft_array_t AudioBuf::get_array(){
  return this->data;
}

AudioBuf AudioBuf::window( duration_t length, duration_t start ){
  unsigned int start_index;
  start_index = floor( start * SAMPLE_RATE );
  AudioBuf ret = AudioBuf( length );
  unsigned int i;
  for( i=0; i < ret.get_num_samples(); i++ ){
    *(ret[i]) = this->data[start_index+i];
  }
  return ret;
}
  
AudioBuf AudioBuf::repeat( int repetitions ){
  AudioBuf ret = AudioBuf( this->get_length() * repetitions );
  unsigned int i;
  for( i=0; i < ret.get_num_samples(); i++ ){
    *(ret[i]) = this->data[ i % this->get_num_samples() ];
  }
  return ret;
}

bool AudioBuf::write_to_file( string filename ){
  cerr << "unimplemented\n";
}

AudioRequest::AudioRequest( AudioBuf buf ){
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

void AudioDev::choose_device(){
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

void AudioDev::nonblocking_play( AudioBuf buf ){
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
  // TODO: somehow free up the stream resource when playback stops
  //check_error( Pa_CloseStream( stream ) );
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

AudioBuf AudioDev::recordback( AudioBuf buf ){}

inline void AudioDev::check_error( PaError err ){
  if( err != paNoError )
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
}

/** call calibration functions to create a new configuration */
Config::Config(){
  //TODO: choose_audio_devices()
  this->warn_audio_level();
  this->choose_ping_freq();
  this->choose_ping_threshold();
  this->choose_phone_home();
}

Config::Config( string filename ){
  cerr << "unimplemented\n";
}

bool Config::write_config_file( string filename ){
  cerr << "unimplemented\n";
}

void Config::disable_phone_home(){
  phone_home = false;
  this->write_config_file( CONFIG_FILE_NAME );
}
  
void Config::choose_ping_freq(){}
  
void Config::choose_ping_threshold(){}

void Config::choose_phone_home(){}
  
void Config::warn_audio_level(){}

Emailer::Emailer( string dest_addr ){}

bool Emailer::phone_home( string filename ){}

bool SysInterface::sleep_monitor(){}

duration_t SysInterface::idle_seconds(){
#ifdef PLATFORM_POSIX
  Display *dis;
  XScreenSaverInfo *info;
  dis = XOpenDisplay((char *)0);
  Window win = DefaultRootWindow(dis);
  info = XScreenSaverAllocInfo();
  XScreenSaverQueryInfo( dis, win, info );
  return (info->idle)/1000;
#endif //PLATFORM_POSIX
}

void SysInterface::sleep( duration_t duration ){
  /* use portaudio's convenient portable sleep function */
  Pa_Sleep( (int)(duration*1000) );
}

bool SysInterface::log( string message, string log_filename ){}

AudioBuf tone( duration_t duration, frequency freq, duration_t delay, 
	       duration_t fade_time ){
  // create empty buffer
  AudioBuf buf = AudioBuf( duration );
  unsigned int i;
  for( i=0; i < buf.get_num_samples(); i++ ){
    *(buf[i]) = sin( 2*M_PI * freq * i / SAMPLE_RATE );
  }
  return buf;
}

fft_array_t fft( AudioBuf buf, int N ){}

fft_array_t energy_spectrum( AudioBuf buf, int N ){}

fft_array_t welch_energy_spectrum( AudioBuf buf, int N, 
				   duration_t window_size ){}

int freq_index( frequency freq ){}

float freq_energy( AudioBuf buf, frequency freq_of_interest, 
		   duration_t window_size ){}

Statistics measure_stats( AudioBuf buf, frequency freq ){}

void term_handler( int signum, int frame ){}

long log_start_time( string log_filename ){}

void power_management( frequency freq, float threshold ){}

int main( int argc, char **argv ){
  duration_t length = 3;
  AudioDev my_audio = AudioDev();

  my_audio.choose_device();

  //AudioBuf my_buf = tone( length, 440 );
  cout << "recording audio...\n";
  AudioBuf my_buf = my_audio.blocking_record( length );
  cout << "playback...\n";
  my_audio.nonblocking_play( my_buf ); 
  SysInterface::sleep( length ); // give the audio some time to play
  return 0;
}
