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

void AudioBuf::prepend_silence( duration_t silence_duration ){}

duration_t AudioBuf::get_length(){}
  
fft_array_t AudioBuf::get_array(){}

AudioBuf AudioBuf::window( duration_t length, duration_t start ){}
  
AudioBuf AudioBuf::repeat( int repetitions ){}

bool AudioBuf::write_to_file( string filename ){}

AudioDev::AudioDev(){
  // Initialize PortAudio
  check_error( Pa_Initialize() );
}

AudioDev::~AudioDev(){
  // close PortAudio
  check_error( Pa_Terminate() );
}

int AudioDev::player_callback( const void *inputBuffer, void *outputBuffer,
			       unsigned long framesPerBuffer,
			       const PaStreamCallbackTimeInfo* timeInfo,
			       PaStreamCallbackFlags statusFlags,
			       void *userData ){
  //cout << "inside callback" << endl;
  /* Cast data passed through stream to our structure. */
  // in this case, we are storing the index where we left off playback.
  int *index = (int*)userData; 
  float *out = (float*)outputBuffer;
  (void) inputBuffer; /* Prevent unused variable warning. */

  float left_phase=1.0, right_phase=-1.0;
  unsigned int i;
  for( i=0; i<framesPerBuffer; i++ ){
    *out++ = left_phase;  /* left */
    *out++ = right_phase;  /* right */
    /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
    left_phase += 0.01f;
    /* When signal reaches top, drop back down. */
    if( left_phase >= 1.0f ) left_phase -= 2.0f;
    /* higher pitch so we can distinguish left and right. */
    right_phase += 0.03f;
    if( right_phase >= 1.0f ) right_phase -= 2.0f;
  }
  return 0;
  // we would return 1 when playback is complete (ie when we want the stream
  // to die)
}

void AudioDev::nonblocking_play( AudioBuf buf ){
  PaStream *stream;
  int *data = new int;
  *data = 0;
  // TODO: include pointer to buf in $data

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
	 data ) ); /*This is a pointer that will be passed to
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
	       duration_t fade_time ){}

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
  AudioDev my_audio = AudioDev();
  AudioBuf my_buf = AudioBuf();
  my_audio.nonblocking_play( my_buf ); 
  while(1){
    SysInterface::sleep( 1 );
  }
  return 0;
}
