#ifndef AUDIO_H
#define AUDIO_H
#include <string>
#include <vector>

#ifdef MINGW
#include "/usr/include/portaudio.h"
#else
#include <portaudio.h>
#endif

extern int SAMPLE_RATE;

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
  /** apply a gain */
  AudioBuf operator*( float gain );
  void prepend_silence( duration_t silence_duration );
  duration_t get_length() const;
  unsigned int get_num_samples() const;
  /** return audio samples as an array for signal processing */
  sample_t* get_array() const;
  /** return a trimmed audio buffer starting at start seconds with a given
      length */
  AudioBuf* window( duration_t length, duration_t start=0 ) const;
  /** mixes the passed AudioBuf with this (pairwise adds samples).
   If b is longer than this, then excess is ignored.*/
  void mix( const AudioBuf& b );
  /** return a given number of repetitions of this audio buffer */
  AudioBuf repeat( int repetitions=2 ) const;
  bool write_to_file( std::string filename ) const;
private:
  unsigned int num_samples;
  sample_t* data;
};

/** concatenate two AudioBufs */
AudioBuf* operator+( const AudioBuf& a, const AudioBuf& b );


/** this class stores all data relevant to an Audio callback function */
class AudioRequest{
public:
  /** this constructor creates a playback request */
  AudioRequest( const AudioBuf & buf );
  /** this constructor creates a record request */
  AudioRequest( duration_t len );
  /** has this request been fully serviced? */
  bool done();
  /** fade volume to a given level over a short time period. */
  void fade( float final_level );

  AudioBuf audio;
  unsigned int progress_index;
  /** in [0,1] scales down playback audio buffers */
  float currentVolumeLevel, targetVolumeLevel;
  float volumeStepFactor;
  /** pointer to the stream playing this request, if any */
  PaStream* stream;
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
      recording hardware device to use.
      Returns recording and playback device numbers.*/
  std::pair<unsigned int,unsigned int> prompt_device();
  /** or if we already know which devices to use we can specify
    @return true iff device numbers specified are available */
  bool choose_device( unsigned int in_dev_num, unsigned int out_dev_num );
  /** returns a list with all of the available audio device names,
      ordered by their corresponding device id */
  std::vector<std::string> list_devices();

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

  AudioRequest* nonblocking_play( const AudioBuf & buf );
  /** the looping version of play repeats the audio buffer indefinitely */
  AudioRequest* nonblocking_play_loop( const AudioBuf & buf );
  void blocking_play( const AudioBuf & buf );
  AudioBuf blocking_record( duration_t duration );
  /** Record the echo of buf */
  AudioBuf recordback( const AudioBuf & buf );

  /** prints description of PortAudio error message, if any */
  static PaError check_error( PaError err );

  /** must be called ONCE by the program to set up portaudio */
  static void init();
  /** must be called after ALL portaudio usage is complete */
  static void terminate();

private:
  PaStreamParameters out_params, in_params;
};

#endif //ndef AUDIO_H
