/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel
 */
#include <string>
#include <vector>
#ifdef PLATFORM_WINDOWS
#include "/usr/include/portaudio.h"
#else
#include <portaudio.h>
#endif

typedef float duration_t; /* durations are expressed as seconds, with floats */
typedef float frequency;
typedef float sample_t;
/* Note that we are using floats not integers for audio samples because it makes
   the signal processing code easier. */

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
  sample_t& operator[]( unsigned int index ) const;
  void prepend_silence( duration_t silence_duration );
  duration_t get_length() const;
  unsigned int get_num_samples() const;
  /** return audio samples as an array for signal processing */
  sample_t* get_array() const;
  /** return a trimmed audio buffer starting at start seconds with a given
      length */
  AudioBuf* window( duration_t length, duration_t start=0 ) const;
  /** return a given number of repetitions of this audio buffer */
  AudioBuf repeat( int repetitions=2 ) const;
  bool write_to_file( std::string filename ) const;
private:
  unsigned int num_samples;
  sample_t* data;
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

typedef std::vector< std::pair<frequency,float> > freq_response;

class AudioDev{
public:
  /** Default constructor prompts user for HW devices, if multiple exist */
  AudioDev();
  /** Alternative constructor specifies which HW devices to use */
  AudioDev( unsigned int rec_dev, unsigned int play_dev );

  /** Destructor closes the recording device */
  ~AudioDev();

  /** this is an interactive function that asks the user which playback and 
      recording hardware device to use.
      Returns recording and playback device numbers.*/
  std::pair<unsigned int,unsigned int> prompt_device();
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
  /** this is similar to player_callback except that the audio is looped
      indefinitely */
  static int oscillator_callback( const void *inputBuffer, void *outputBuffer,
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
  PaStream* nonblocking_play( const AudioBuf & buf );
  /** the looping version of play repeats the audio buffer indefinitely */
  PaStream* nonblocking_play_loop( const AudioBuf & buf );
  AudioBuf blocking_record( duration_t duration );
  /** Record the echo of buf */
  AudioBuf recordback( const AudioBuf & buf );

  /** tests the frequency response of the audio hardware and returns the most 
      reponsive ultrasonic frequency */
  freq_response test_freq_response();

  PaStreamParameters out_params, in_params;
  /** prints PortAudio error message, if any */
  static inline void check_error( PaError err );
};

/** stores and elicits the program configuration */
class Config{
public:
  /** empty constructor */
  Config();
  /** load() first tries to read the configuration from the passed 
      filename.  If unsuccessful, then call the calibration functions and
      write a new config file to use next time.
  @return true iff a new configuration was created (config file didn't exist)*/
  bool load( AudioDev & audio, std::string filename );
  bool write_config_file();
  /** this would be called after we've already phoned home */
  void disable_phone_home();
  /** tests the frequency response and records it in the log file */
  void log_freq_response( AudioDev & audio );

  frequency ping_freq;
  float threshold;
  bool allow_phone_home;
  unsigned int rec_dev;
  unsigned int play_dev;

private:
  std::string filename;
  /** CALIBRATION FUNCTIONS */
  /** Prompt the user to find the best ping frequency.
      Generally, we want to choose a frequency that is both low enough to
      register on the (probably cheap) audio equipment but high enough to be
      inaudible. */
  void choose_ping_freq( AudioDev & audio );
  /** Choose the variance threshold for presence detection by prompting
      the user. */
  void choose_ping_threshold( AudioDev & audio, frequency freq );
  /** Ask the user whether or not to report anonymous statistics */
  void choose_phone_home();
  /** Plays a series of loud tones to help users adjust their speaker volume */
  void warn_audio_level( AudioDev & audio );
};

class SysInterface{
 public:
  static bool sleep_monitor();
  static duration_t idle_seconds();
  /** blocks the process for the specified time */
  static void sleep( duration_t duration );
  /** returns shortly after HID activity */
  static void wait_until_active();
  /** returns after there has been no HID for some time */ 
  static void wait_until_idle();
  /** returns glibc style time, ie epoch seconds */
  static long current_time();
  /** appends message to log */
  static bool log( std::string message );
  /** returns the name of configuration directory, creating if nonexistant */
  static std::string config_dir();
  /** register fcn to clean up on terminattion */
  static void register_term_handler();
  /** sends log back to us using FTP */
  static bool phone_home();
};

/** duration is in seconds and freq is the  tone pitch.  
    Delay, in seconds, is silent time added to before tone.
    FADE_SAMPLES is the length of the fade in and out periods. */
AudioBuf tone( duration_t duration=0.5, frequency freq=440, duration_t delay=0, 
	       unsigned int fade_samples=441 );

/** duration is in seconds. */
AudioBuf gaussian_white_noise( duration_t duration=1 );


/** Geortzel's algorithm for finding the energy of a signal at a certain
    frequency.  Note that end_index is one past the last valid value. 
    Normalized frequency is measured in cycles per sample: (F/sample_rate)*/
template<class indexable,class precision> 
precision goertzel( const indexable & arr, unsigned int start_index, 
		    unsigned int end_index, precision norm_freq );

/** Bartlett's method is the mean of several runs of Goertzel's algorithm in
    consecutive windows. */
template<class indexable,class precision> 
precision bartlett( const indexable & arr, unsigned int start_index, 
		    unsigned int end_index, precision norm_freq, 
		    unsigned int num_windows );
template<class precision> 
precision bartlett( const AudioBuf & arr, precision norm_freq );


struct Statistics{
  float mean;
  float variance;
  float delta;
};
std::ostream& operator<<(std::ostream& os, Statistics& s);


/** Returns the mean and variance of the intensities of a given frequency
    in the audio buffer sampled in windows spread throughout the recording. */
Statistics measure_stats( const AudioBuf & buf, frequency freq );
/** returns the time at which the program was first run, as indicated in
    the logfile. */
long get_log_start_time( );
/** This is the main program loop.  It checks for a user and powers down
    the display if it's reasonably confident that no one is there */
void power_management( AudioDev & audio, Config & conf );
int main( int argc, char **argv );
