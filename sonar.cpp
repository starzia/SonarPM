/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel and fftw-devel
 */
#include "sonar.hpp"

using namespace std;

AudioBuf::AudioBuf( ){}

AudioBuf::AudioBuf( string filename ){}

AudioBuf::AudioBuf( duration_t length ){
  this->num_samples = ceil( length * SAMPLE_RATE );
  this->data = (fft_array_t)fftw_malloc( sizeof(sample_t)*this->num_samples );
}

sample_t* AudioBuf::operator[]( unsigned int index ){
  return this->data+index;
}

void AudioBuf::prepend_silence( duration_t silence_duration ){}

duration_t AudioBuf::get_length(){}
  
unsigned int AudioBuf::get_num_samples(){ 
  return this->num_samples;
}

fft_array_t AudioBuf::get_array(){}

AudioBuf AudioBuf::window( duration_t length, duration_t start ){}
  
AudioBuf AudioBuf::repeat( int repetitions ){}

bool AudioBuf::write_to_file( string filename ){}

AudioRequest::AudioRequest( AudioBuf buf ){
  this->progress_index = 0; // set to zero so playback starts at beginning
  this->audio = buf;
}

AudioRequest::AudioRequest( duration_t len ){}

AudioDev::AudioDev(){
  // Initialize PortAudio
  check_error( Pa_Initialize() );
}

AudioDev::~AudioDev(){
  // close PortAudio
  check_error( Pa_Terminate() );
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
  // in this case, we are storing the index where we left off playback.
  AudioRequest *req = (AudioRequest*)userData; 
  float *out = (float*)outputBuffer;
  (void) inputBuffer; /* Prevent unused variable warning. */

  unsigned int i;
  for( i=req->progress_index; i < req->progress_index + framesPerBuffer;
       i++ ){
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
  return ( i >= req->audio.get_num_samples() );
}

void AudioDev::nonblocking_play( AudioBuf buf ){
  PaStream *stream;
  AudioRequest *play_request = new AudioRequest( buf );

  /* Open an audio I/O stream. Opening a *default* stream means opening 
     the default input and output devices */
  check_error( Pa_OpenDefaultStream( 
	 &stream,
	 0,          /* no input channels */
	 2,          /* stereo output */
	 paFloat32,  /* 32 bit floating point output */
	 SAMPLE_RATE,
	 256,        /* frames per buffer, i.e. the number
			of sample frames that PortAudio will
			request from the callback. Many apps
			may want to use
			paFramesPerBufferUnspecified, which
			tells PortAudio to pick the best,
			possibly changing, buffer size.*/
	 AudioDev::player_callback, /* this is your callback function */
	 play_request ) ); /*This is a pointer that will be passed to
			     your callback*/

  // start playback
  check_error( Pa_StartStream( stream ) );
}

AudioBuf AudioDev::blocking_record( duration_t duration ){}

AudioBuf AudioDev::recordback( AudioBuf buf ){}

inline void AudioDev::check_error( PaError err ){
  if( err != paNoError )
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
}


Config::Config(){}

Config::Config( string filename ){}

bool Config::write_config_file( string filename ){}

void Config::disable_phone_home(){}
  
void Config::choose_audio_devices(){}

frequency Config::choose_ping_freq(){}
  
float Config::choose_ping_threshold(){}
  
void Config::warn_audio_level(){}

Emailer::Emailer( string dest_addr ){}

bool Emailer::phone_home( string filename ){}

bool SysInterface::sleep_monitor(){}

duration_t SysInterface::idle_seconds(){}

void SysInterface::sleep( duration_t duration ){
  /* use portaudio's convenient portable sleep function */
  Pa_Sleep( (int)(duration*1000) );
}

bool SysInterface::log( string message, string log_filename ){}

AudioBuf tone( duration_t duration, frequency freq, duration_t delay, 
	       duration_t fade_time ){
  // create empty buffer
  AudioBuf buf = AudioBuf( duration );
  int i;
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
  AudioBuf my_buf = tone( length, 440 );
  my_audio.nonblocking_play( my_buf ); 
  SysInterface::sleep( length ); // give the audio some time to play
  return 0;
}
