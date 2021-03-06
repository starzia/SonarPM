#include "audio.hpp"
#include <vector>

/** duration is in seconds and freq is the  tone pitch.  
    Delay, in seconds, is silent time added to before tone.
    FADE_SAMPLES is the length of the fade in and out periods. */
AudioBuf tone( duration_t duration=0.5, frequency freq=440, duration_t delay=0, 
	       unsigned int fade_samples=441 );

/** duration is in seconds. */
AudioBuf gaussian_white_noise( duration_t duration=1 );
/** ultrasonic noise is created by adding ultrasonic sine tones */
AudioBuf ultrasonic_noise( duration_t duration );

/** first order high-pass filter 
@return a new audiobuf */
AudioBuf high_pass( const AudioBuf & buf, frequency half_power_freq );

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

/**The following macro allows us to easily change the sonar feature of interest*/
#define FEATURE(s) (s.delta/s.mean)

/** Returns the mean and variance of the intensities of a given frequency
    in the audio buffer sampled in windows spread throughout the recording. */
Statistics measure_stats( const AudioBuf & buf, frequency freq );
/** returns the time at which the program was first run, as indicated in
    the logfile. */

typedef std::vector< std::pair<frequency,float> > freq_response;
std::ostream& operator<<(std::ostream& os, const freq_response& f);
freq_response operator/(freq_response& a, freq_response& b);

/** tests the frequency response of the audio hardware while playing the 
    provided stimulus audio */
freq_response test_freq_response( AudioDev & audio, AudioBuf & stimulus );

/** tests the frequency response of the audio hardware to both ultrasonic noise and
    the background levels (silence). 
    @return pair<noise response, silence response> */
std::pair<freq_response,freq_response> test_ultrasonic_response( AudioDev & audio );

/** @return the spectrum of the passed buffer */
freq_response spectrum( AudioBuf & buf );
