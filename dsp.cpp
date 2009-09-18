#include "dsp.hpp"
#include <vector>
#define _USE_MATH_DEFINES //for msvc++ to recognize M_PI
#include <cmath>
#include <iostream>
#include <iomanip> // for tweaked printing

#ifdef PLATFORM_WINDOWS
#include <windows.h> //for rand
#else
#include <stdlib.h> //for rand
#endif

#define WINDOW_SIZE (0.1) // sliding window size
#define BARTLETT_WINDOWS (10) // num windows in bartlett's method
#define SCALING_FACTOR (1.02) // freq bin scaling factor for freq response
#define LOWEST_FREQ (15000) // lowest freq for spectral analysis
#define HIGHEST_FREQ (30000) // highest freq for spectral analysis

using namespace std;

AudioBuf tone( duration_t duration, frequency freq, duration_t delay, 
	       unsigned int fade_samples ){
  // create empty buffer
  AudioBuf buf = AudioBuf( duration );
  unsigned int i, end_i = buf.get_num_samples();
  for( i=0; i < end_i; i++ ){
    buf[i] = sin( 2*M_PI * freq * i / SAMPLE_RATE );
  }
  // fade in and fade out to prevent 'click' sound
  for( i=0; i<fade_samples; i++ ){
    float attenuation = 1.0*i/fade_samples;
    buf[i] = attenuation * buf[i];
    buf[end_i-1-i] = attenuation * buf[end_i-1-i];
  }
  return buf;
}

AudioBuf gaussian_white_noise( duration_t duration ){
  AudioBuf buf = AudioBuf( duration );   // create empty buffer
  unsigned int i, end_i = buf.get_num_samples();

  /* inner loop taken from http://www.musicdsp.org/showone.php?id=113 */
  for( i=0; i < end_i; i++ ){
    float R1 = (float) rand() / (float) RAND_MAX;
    float R2 = (float) rand() / (float) RAND_MAX;
    buf[i] = (float) sqrt( -2.0f * log( R1 )) * cos( 2.0f * M_PI * R2 );
  }
  return buf;
}

AudioBuf ultrasonic_noise( duration_t duration ){
  AudioBuf buf = tone( duration, 0 );   // create empty buffer;
  // below, casts are needed to prevent ambiguity for msvc++
  unsigned int num_tones = (log((float)HIGHEST_FREQ)-log((float)LOWEST_FREQ)) 
                            / log((float)SCALING_FACTOR);
  frequency f;
  for( f = LOWEST_FREQ; f < HIGHEST_FREQ; f *= SCALING_FACTOR ){
    buf.mix( tone( duration, f ) * (1.0/num_tones) );
  }
  return buf;
}


AudioBuf high_pass( const AudioBuf & buf, frequency half_power_freq ){
  AudioBuf filtered = AudioBuf( buf.get_length() );
  double rc = 1.0 / (2*M_PI*half_power_freq);
  double dt = 1.0 / SAMPLE_RATE;
  double alpha = rc / (rc + dt);
  cout << "rc="<<rc<<" dt="<<dt<<" alpha="<<alpha<<endl;
  
  unsigned int i;
  filtered[0] = buf[0];
  for( i=1; i<filtered.get_num_samples(); i++ ){
    filtered[i] = alpha * (filtered[i-1] + buf[i] - buf[i-1]);
  }
  return filtered;
}

/** statistical mean */
template<class precision> precision mean( const vector<precision> & arr ){
  unsigned int i;
  precision mean=0;
  for( i=0; i<arr.size(); i++ )
    mean += arr[i];
  return mean / arr.size();
}
/** statistical variance */
template<class precision> precision variance( const vector<precision> & arr ){
  unsigned int i;
  precision mu = mean( arr );
  precision var = 0;
  for( i=0; i<arr.size(); i++ )
    var += ( arr[i] - mu ) * ( arr[i] - mu );
  return var;
}
/** average inter-sample absolute difference */
template<class precision> precision delta( const vector<precision> & arr ){
  unsigned int i;
  precision delta=0;
  for( i=1; i<arr.size(); i++ )
    delta += abs( arr[i] - arr[i-1] );
  return delta / arr.size();
}

ostream& operator<<(ostream& os, const Statistics& s){
  os << scientific << setprecision(3) << s.mean << " " << s.delta;
  /*
  os<< "stat m:" <<s.mean<< "\tv:" <<s.variance<< "\td:" <<s.delta
    << "\tf:" << FEATURE(s);
  */
  return os;
}
ostream& operator<< (ostream& os, Statistics& s){
  os << scientific << setprecision(3) << s.mean << " " << s.delta;
  /*
  os<< "stat m:" <<s.mean<< "\tv:" <<s.variance<< "\td:" <<s.delta
    << "\tf:" << FEATURE(s);
  */
  return os;
}

/** Geortzel's algorithm for finding the energy of a signal at a certain
    frequency.  Note that end_index is one past the last valid value. 
    Normalized frequency is measured in cycles per sample: (F/sample_rate)*/
template<class indexable,class precision> 
precision goertzel( const indexable & arr, unsigned int start_index, 
		    unsigned int end_index, precision norm_freq ){
  precision s_prev = 0;
  precision s_prev2 = 0;
  precision coeff = 2 * cos( 2 * M_PI * norm_freq );
  unsigned int i;
  for( i=start_index; i<end_index; i++ ){
    precision s = arr[i] + coeff*s_prev - s_prev2;
    s_prev2 = s_prev;
    s_prev = s;
  }
  // return the energy (power)
  return s_prev2*s_prev2 + s_prev*s_prev - coeff*s_prev2*s_prev;
}

#ifdef GCC
//16 byte vector of 4 floats
typedef float v4sf __attribute__ ((vector_size(16)));
//16 byte vector of 4 uints
typedef unsigned int v4sd __attribute__ ((vector_size(16))); 

typedef union f4vector { // wrapper for two alternative vector representations
  v4sf v;
  float f[4];
} f4v; 

typedef union d4vector { // wrapper for two alternative vector representations
  v4sd v;
  unsigned int f[4];
} d4v;

/** vectorized version of goertzel that operates on four consecutive windows
    simultaneously.  ie, operates on arr in the index range:
    [ start_index, start_index + 4*window_size ) */
template<class indexable>
f4v quad_goertzel( const indexable & arr, unsigned int start_index, 
		   unsigned int window_size, float norm_freq ){
  const v4sd one = {1, 1, 1, 1};
  const v4sf zero = {0.0, 0.0, 0.0, 0.0};

  f4v ret, s_prev, s_prev2;
  s_prev.v = s_prev2.v = zero;
  f4v coeff;
  coeff.f[0]=coeff.f[1]=coeff.f[2]=coeff.f[3]= 2 * cos( 2 * M_PI * norm_freq );

  d4v i; // set starting indices
  i.f[0] = start_index + 0*window_size;
  i.f[1] = start_index + 1*window_size;
  i.f[2] = start_index + 2*window_size;
  i.f[3] = start_index + 3*window_size;
  for(; i.f[0]<start_index+window_size; i.v = i.v+one ){
    f4v s,a;
    a.f[0] = arr[i.f[0]];
    a.f[1] = arr[i.f[1]];
    a.f[2] = arr[i.f[2]];
    a.f[3] = arr[i.f[3]];
    s.v = a.v + coeff.v * s_prev.v - s_prev2.v;
    s_prev2 = s_prev;
    s_prev = s;
  }
  // return the energy (power)
  ret.v = s_prev2.v * s_prev2.v + s_prev.v * s_prev.v 
    - coeff.v * s_prev2.v * s_prev.v;
  return ret;
}
#endif

/** Bartlett's method is the mean of several runs of Goertzel's algorithm in
    consecutive windows. */
template<class indexable,class precision> 
precision bartlett( const indexable & arr, unsigned int start_index, 
		    unsigned int end_index, precision norm_freq, 
		    unsigned int num_windows ){
  vector<float> energies;
  unsigned int i, window_size = (end_index-start_index)/num_windows;

  // for each window
  for( i=0; i<num_windows; i++ ){
    /*
    if( i > num_windows-4 ){ // TODO: decide which is more efficient
    */
      // if less than four windows left, do one at a time.
      // be careful not to go beyond end_index
      unsigned int j = (end_index<(i+1)*window_size)? 
	end_index: start_index+(i+1)*window_size;
      energies.push_back( goertzel( arr, start_index + i*window_size, 
				    j, norm_freq) );
      /*
    }else{
      // otherwise do four at once.
      f4v res = quad_goertzel( arr, start_index + i*window_size,
			       window_size, norm_freq );
      energies.push_back( res.f[0] );
      energies.push_back( res.f[1] );
      energies.push_back( res.f[2] );
      energies.push_back( res.f[3] );
      i+=3; //skip ahead
    }  
      */
  }
  return mean( energies );
}
template<class precision> 
precision bartlett( const AudioBuf & arr, precision norm_freq ){
  return bartlett(arr, 0, arr.get_num_samples(), norm_freq, BARTLETT_WINDOWS);
}


Statistics measure_stats( const AudioBuf & buf, frequency freq ){
  vector<float> energies;
  
  unsigned int start, N = (unsigned int)ceil(WINDOW_SIZE*SAMPLE_RATE);
  // Calculate echo intensities: use a sliding non-overlapping window
  // TODO: parallelize window computations using SIMD instructions.
  for( start=0; start + N < buf.get_num_samples(); start += N ){
    energies.push_back( bartlett(buf,start,start+N,
				 (float)freq/SAMPLE_RATE,BARTLETT_WINDOWS) );
  }
  
  // calculate statistics
  Statistics ret;
  ret.mean = mean( energies );
  ret.variance = variance( energies );
  ret.delta = delta( energies );
  return ret;
}

ostream& operator<<(ostream& os, const freq_response& f){
  os << "freq_response{" <<endl;
  unsigned int i;
  for( i=0; i<f.size(); i++ ){
    os << f[i].first <<"Hz  \t"<< f[i].second <<endl;
  }
  os << '}' <<endl;
  return os;
}

freq_response operator/(freq_response& a, freq_response& b){
  freq_response ret;
  unsigned int i;
  for( i=0; i<a.size(); i++ ){
    float quotient = a[i].second / b[i].second;
    ret.push_back( make_pair( a[i].first, quotient) );
  }
  return ret;
}

freq_response test_freq_response( AudioDev & audio ){
  const duration_t test_period = 10;
  cout << "Please wait while the system is calibrated."<<endl
       << "This will take approximately "<<test_period*2<<" seconds."<<endl
       << "During this time you may hear some noise."<<endl;
  
  // record silence (as a reference point)
  AudioBuf silence = tone( test_period, 0 );
  AudioBuf silence_rec = audio.recordback( silence );
  // record white noise
  AudioBuf noise = ultrasonic_noise(test_period);
  AudioBuf noise_rec = audio.recordback( noise );
  
  // now record ratio of stimulated to silent energy for a range of frequencies
  freq_response noise_response = spectrum( noise_rec );
  freq_response silence_response = spectrum( silence_rec );
  freq_response gain = noise_response / silence_response;
  cout << gain <<endl;
  return gain;
}

freq_response spectrum( AudioBuf & buf ){
  freq_response response;
  frequency freq;
  for( freq = LOWEST_FREQ; freq <= HIGHEST_FREQ; freq *= SCALING_FACTOR ){
    float energy = measure_stats(buf,freq).mean;
    response.push_back( make_pair( freq, energy ) );
  }
  return response;
}
