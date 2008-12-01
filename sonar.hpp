/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel and fftw-devel
 */
#include <string>
#include <portaudio.h>
#include <fftw3.h>

#define FFT_POINTS (1024)
#define FFT_FREQUENCIES (FFT_POINTS/2)
#define WINDOW_SIZE (0.01) /* this is for welch */
#define SAMPLE_RATE (44100)
#define CONFIG_FILE_NAME "~/.sonarPM/sonarPM.cfg"

#define PLATFORM_POSIX

using namespace std;

typedef float duration_t; /* durations are expressed as seconds, with floats */
typedef int frequency;
typedef float sample_t;
typedef sample_t* fft_array_t;
/* Note that we are using floats, not the default doubles, for the fft functions.
   This is because audio samples are stored as floats so we can use audio arrays
   directly as input to the fft routines. */


/** This class represents MONO audio buffers */
class AudioBuf{
public:
  /** default constuctor */
  AudioBuf();
  /** alternative contructor reads a WAV file */
  AudioBuf( std::string filename );
  /** alternative allocates an empty buffer (not necessarily silent) */
  AudioBuf( duration_t len );
  /** destructor */
  ~AudioBuf();

  /** returns an audio sample */
  sample_t* operator[]( unsigned int index ) const;
  void prepend_silence( duration_t silence_duration );
  duration_t get_length() const;
  unsigned int get_num_samples() const;
  /** return audio samples as an array for fftw or other processing */
  fft_array_t get_array() const;
  /** return a trimmed audio buffer starting at start seconds with a given
      length */
  AudioBuf* window( duration_t length, duration_t start=0 ) const;
  /** return a given number of repetitions of this audio buffer */
  AudioBuf repeat( int repetitions=2 ) const;
  bool write_to_file( std::string filename ) const;
private:
  unsigned int num_samples;
  fft_array_t data;
};

/** this class stores all data relevant to an Audio callback function */
class AudioRequest{
public:
  /** this constructor creates a playback request */
  AudioRequest( const AudioBuf & buf );
  /** this constructor creates a record request */
  AudioRequest( duration_t len );
  /** has this request been fully serviced? */
  bool done();

  AudioBuf audio;
  unsigned int progress_index;
};

class AudioDev{
public:
  /** Default constructor prompts user for HW devices, if multiple exist */
  AudioDev();
  /** Alternative constructor specifies which HW devices to use */
  AudioDev( unsigned int rec_dev, unsigned int play_dev );

  /** Destructor closes the recording device */
  ~AudioDev();

  /** this is an interactive function that asks the user which playback and 
      recording hardware device to use */
  void choose_device();
  /** or if we already know which devices to use we can specify */
  void choose_device( unsigned int in_dev_num, unsigned int out_dev_num );

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
  void nonblocking_play( const AudioBuf & buf );
  AudioBuf blocking_record( duration_t duration );
  /** Record the echo of buf */
  AudioBuf recordback( AudioBuf buf );

  PaStreamParameters out_params, in_params;
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
  unsigned int rec_dev;
  unsigned int play_dev;

  /** CALIBRATION FUNCTIONS */
  /** Prompt the user to find the best ping frequency.
      Generally, we want to choose a frequency that is both low enough to
      register on the (probably cheap) audio equipment but high enough to be
      inaudible. */
  void choose_ping_freq();
  /** Choose the variance threshold for presence detection by prompting
      the user.
      NB: ping_freq must already be set!*/
  void choose_ping_threshold();
  /** Ask the user whether or not to report anonymous statistics */
  void choose_phone_home();
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
		   duration_t window_size=WINDOW_SIZE );

typedef struct{
  float mean;
  float variance;
} Statistics;
/** Returns the mean and variance of the intensities of a given frequency
    in the audio buffer sampled in windows spread throughout the recording. */
Statistics measure_stats( const AudioBuf & buf, frequency freq );
/** cleans up and terminates the program when SIGTERM is received */
void term_handler( int signum, int frame );
/** returns the time at which the program was first run, as indicated in
    the logfile. */
long log_start_time( std::string log_filename );
/** This is the main program loop.  It checks for a user and powers down
    the display if it's reasonably confident that no one is there */
void power_management( frequency freq, float threshold );
int main( int argc, char **argv );
