/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel and fftw-devel
 */
#include <string>
#include <math>
#include <portaudio.h>
#include <fftw.h>
#define FFT_POINTS 1024

typedef time float; /* times are expressed as seconds, with floats */
typedef int frequency;

/** This class represents MONO audio buffers */
class AudioBuf{
public:
  /** Contructor reads a WAV file */
  AudioBuf( string filename ){}

  void prepend_silence( time silence_duration ){}
  time get_length(){}
  
  /** return audio samples as an array for fftw or other processing */
  fft_array_t get_array(){}

  /** return a trimmed audio buffer starting at start seconds with a given
      length */
  AudioBuf window( time length, time start=0 ){}
  
  /** return a given number of repetitions of this audio buffer */
  AudioBuf repeat( int repetitions=2 ){}

  bool write_to_file( string filename ){}

private:
  fft_array_t data
};

class AudioDev{
public:
  /** Constructor opens a new instance of recording device */
  AudioDev(){}

  /** Destructor closes the recording device */
  ~AudioDev(){}

  void nonblocking_play( AudioBuf buf ){}
  AudioBuf blocking_record( time duration ){}

  /** Record the echo of buf */
  AudioBuf recordback( AudioBuf buf ){}
};

/** stores and elicits the program configuration */
class Config{
public:
  /** default, empty contructor, calls the calibration functions */
  Config(){}
  /** load config from file */
  Config( string filename ){}
  bool write_config_file( string filename ){}
  /** this would be called after we've already phoned home */
  void disable_phone_home(){}
  
  frequency ping_freq;
  float threshold;
  bool phone_home;
  AudioDev rec_dev;
  AudioDev play_dev;

private:
  /** Prompt the user to choose their preferred recording and playback devices.
      This must be done before any audio recording can take place. */
  void choose_audio_devices(){}

  /** CALIBRATION FUNCTIONS */
  /** Prompt the user to find the best ping frequency.
      Generally, we want to choose a frequency that is both low enough to
      register on the (probably cheap) audio equipment but high enough to be
      inaudible. */
  freq choose_ping_freq(){}
  
  /** Choose the variance threshold for presence detection by prompting
      the user.
      NB: ping_freq must already be set!*/
  float choose_ping_threshold(){}
  
  /** Plays a series of loud tones to help users adjust their speaker volume */
  void warn_audio_level(){}
};

class Emailer(){
  Emailer( string dest_addr ){}

  /** sends the filename to destination_address */
  phone_home( string filename ){}

  /** this is the email address that we phone home to */
  string destination_address;
};

class SystemInterface(){
 public:
  bool sleep_monitor(){}
  time idle_seconds(){}
  /** blocks the process for the specified time */
  void sleep( time duration ){}
  /** appends message to log */
  bool log( string message, string log_filename ){}
};

/** duration is in seconds and freq is the  tone pitch.  
    Delay, in seconds, is silent time added to before tone.
    FADE_SAMPLES is the length of the fade in and out periods. */
AudioBuf tone( time duration=0.5, frequency freq=440, time delay=0, 
	       time fade_time=0.001 ){}

fft_array_t fft( AudioBuf buf, int N=FFT_POINTS ){}

/** Gives the frequency energy spectrum of an audio buffer, without using
    windowing.  This is useful for periodic signals.  When using this
    function, it may be important to trim the buffer to precisely an integer
    multiple of the fundamental tone's period. */
fft_array_t energy_spectrum( AudioBuf buf, int N=FFT_POINTS ){}


/** Estimates the frequency energy spectrum of an audio buffer by averaging
    the spectrum over a sliding rectangular window.  
    I think that this is an implementation of Welch's Method. */
fft_array_t welch_energy_spectrum( AudioBuf buf, int N=FFT_POINTS, 
				   time window_size ){}

/** returns the spectrum index closest to a given frequency. */
int freq_index( frequency freq ){}

/** returns the power/energy of the given time series data at the frequency
    of interest. */
float freq_energy( AudioBuf buf, frequency freq_of_interest, time window_size ){}

typedef struct{
  float mean;
  float variance;
} Statistics;

/** Returns the mean and variance of the intensities of a given frequency
    in the audio buffer sampled in windows spread throughout the recording. */
Statistics measure_stats( AudioBuf buf, frequency freq ){}

/** cleans up and terminates the program when SIGTERM is received */
void term_handler( int signum, int frame ){}

/** returns the time at which the program was first run, as indicated in
    the logfile. */
long log_start_time( string log_filename ){
  
}

/** This is the main program loop.  It checks for a user and powers down
    the display if it's reasonably confident that no one is there */
void power_management( frequency freq, float threshold ){}

int main( int argc, char **argv ){}
