/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel and fftw-devel
 */
#include <iostream>
#include <string>
#include <cmath>
#include <portaudio.h>
#include <fftw3.h>
#define FFT_POINTS (1024)
#define WINDOW_SIZE (0.01) /* this is for welch */
#define SAMPLE_RATE (44100)

using namespace std;

typedef float duration_t; /* durations are expressed as seconds, with floats */
typedef int frequency;
typedef float sample_t;
typedef sample_t* fft_array_t;

/** This class represents MONO audio buffers */
class AudioBuf{
public:
  /** default constuctor */
  AudioBuf();
  /** alternative contructor reads a WAV file */
  AudioBuf( std::string filename );
  /** alternative allocates an empty buffer (not necessarily silent) */
  AudioBuf( duration_t len );
  /** returns an audio sample */
  sample_t* operator[]( unsigned int index );
  void prepend_silence( duration_t silence_duration );
  duration_t get_length();
  unsigned int get_num_samples();
  /** return audio samples as an array for fftw or other processing */
  fft_array_t get_array();
  /** return a trimmed audio buffer starting at start seconds with a given
      length */
  AudioBuf window( duration_t length, duration_t start=0 );
  /** return a given number of repetitions of this audio buffer */
  AudioBuf repeat( int repetitions=2 );
  bool write_to_file( std::string filename );
private:
  unsigned int num_samples;
  fft_array_t data;
};

/** this class stores all data relevant to an Audio callback function */
class AudioRequest{
public:
  /** this constructor creates a playback request */
  AudioRequest( AudioBuf buf );
  /** this constructor creates a record request */
  AudioRequest( duration_t len );
  /** has this request been fully serviced? */
  bool done();

  AudioBuf audio;
  unsigned int progress_index;
};

class AudioDev{
public:
  /** Constructor opens a new instance of recording device */
  AudioDev();
  /** Destructor closes the recording device */
  ~AudioDev();
  /* This routine will be called by the PortAudio engine when audio is needed.
  ** It may called at interrupt level on some machines so don't do anything
  ** that could mess up the system like calling malloc() or free(). */ 
  static int player_callback( const void *inputBuffer, void *outputBuffer,
			      unsigned long framesPerBuffer,
			      const PaStreamCallbackTimeInfo* timeInfo,
			      PaStreamCallbackFlags statusFlags,
			      void *userData );
  /** This routine will be called by the PortAudio engine when it has audio
      ready. */
  static int recorder_callback( const void *inputBuffer, void *outputBuffer,
				unsigned long framesPerBuffer,
				const PaStreamCallbackTimeInfo* timeInfo,
				PaStreamCallbackFlags statusFlags,
				void *userData );
  void nonblocking_play( AudioBuf buf );
  AudioBuf blocking_record( duration_t duration );
  /** Record the echo of buf */
  AudioBuf recordback( AudioBuf buf );
private:
  /** prints PortAudio error message, if any */
  inline void check_error( PaError err );
};

/** stores and elicits the program configuration */
class Config{
public:
  /** default, empty contructor, calls the calibration functions */
  Config();
  /** load config from file */
  Config( std::string filename );
  bool write_config_file( std::string filename );
  /** this would be called after we've already phoned home */
  void disable_phone_home();
  
  frequency ping_freq;
  float threshold;
  bool phone_home;
  AudioDev rec_dev;
  AudioDev play_dev;

  /** Prompt the user to choose their preferred recording and playback devices.
      This must be done before any audio recording can take place. */
  void choose_audio_devices();

  /** CALIBRATION FUNCTIONS */
  /** Prompt the user to find the best ping frequency.
      Generally, we want to choose a frequency that is both low enough to
      register on the (probably cheap) audio equipment but high enough to be
      inaudible. */
  frequency choose_ping_freq();
  /** Choose the variance threshold for presence detection by prompting
      the user.
      NB: ping_freq must already be set!*/
  float choose_ping_threshold();
  /** Plays a series of loud tones to help users adjust their speaker volume */
  void warn_audio_level();
};

class Emailer{
  Emailer( std::string dest_addr );
  /** sends the filename to destination_address */
  bool phone_home( std::string filename );
  /** this is the email address that we phone home to */
  std::string destination_address;
};

class SysInterface{
 public:
  static bool sleep_monitor();
  static duration_t idle_seconds();
  /** blocks the process for the specified time */
  static void sleep( duration_t duration );
  /** appends message to log */
  static bool log( std::string message, std::string log_filename );
};

/** duration is in seconds and freq is the  tone pitch.  
    Delay, in seconds, is silent time added to before tone.
    FADE_SAMPLES is the length of the fade in and out periods. */
AudioBuf tone( duration_t duration=0.5, frequency freq=440, duration_t delay=0, 
	       duration_t fade_time=0.001 );

/** wrapper for fftw library's fft function */
fft_array_t fft( AudioBuf buf, int N=FFT_POINTS );
/** Gives the frequency energy spectrum of an audio buffer, without using
    windowing.  This is useful for periodic signals.  When using this
    function, it may be important to trim the buffer to precisely an integer
    multiple of the fundamental tone's period. */
fft_array_t energy_spectrum( AudioBuf buf, int N=FFT_POINTS );
/** Estimates the frequency energy spectrum of an audio buffer by averaging
    the spectrum over a sliding rectangular window.  
    I think that this is an implementation of Welch's Method. */
fft_array_t welch_energy_spectrum( AudioBuf buf, int N=FFT_POINTS, 
				   duration_t window_size=WINDOW_SIZE );
/** returns the spectrum index closest to a given frequency. */
int freq_index( frequency freq );
/** returns the power/energy of the given time series data at the frequency
    of interest. */
float freq_energy( AudioBuf buf, frequency freq_of_interest, 
		   duration_t window_size );

typedef struct{
  float mean;
  float variance;
} Statistics;
/** Returns the mean and variance of the intensities of a given frequency
    in the audio buffer sampled in windows spread throughout the recording. */
Statistics measure_stats( AudioBuf buf, frequency freq );
/** cleans up and terminates the program when SIGTERM is received */
void term_handler( int signum, int frame );
/** returns the time at which the program was first run, as indicated in
    the logfile. */
long log_start_time( std::string log_filename );
/** This is the main program loop.  It checks for a user and powers down
    the display if it's reasonably confident that no one is there */
void power_management( frequency freq, float threshold );
int main( int argc, char **argv );
