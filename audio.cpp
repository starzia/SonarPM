#include "audio.hpp"
#include <iostream>
#include <cmath>

#ifdef PLATFORM_WINDOWS
#include <windows.h> //for malloc
#else
#include <stdlib.h> //for malloc
#endif

#define FRAMES_PER_BUFFER (32768) // PortAudio buf size.  The examples use 256.
int SAMPLE_RATE;

using namespace std;

AudioBuf::AudioBuf( ){
  this->num_samples=0;
  this->data = NULL;
}

AudioBuf::AudioBuf( string filename ){
  cerr << "audio file read unimplemented"<<endl;
}

AudioBuf::AudioBuf( duration_t length ){
  this->num_samples = (unsigned int)ceil( length * SAMPLE_RATE );
  this->data = (sample_t*)malloc( sizeof(sample_t)*this->num_samples );
}

AudioBuf::~AudioBuf(){
  if( this->data ) free( this->data ); // free sample array
}

sample_t& AudioBuf::operator[]( unsigned int index ) const{
  return this->data[index];
}

void AudioBuf::prepend_silence( duration_t silence_duration ){
  cerr << "prepend_silence unimplemented"<<endl;
}

duration_t AudioBuf::get_length() const{
  return ( this->num_samples / SAMPLE_RATE );
}
  
unsigned int AudioBuf::get_num_samples() const{ 
  return this->num_samples;
}

sample_t* AudioBuf::get_array() const{
  return this->data;
}

AudioBuf* AudioBuf::window( duration_t length, duration_t start ) const{
  unsigned int start_index;
  start_index = (unsigned int)floor( start * SAMPLE_RATE );
  AudioBuf* ret = new AudioBuf( length );
  unsigned int i;
  for( i=0; i < ret->get_num_samples(); i++ ){
    (*ret)[i] = this->data[start_index+i];
  }
  return ret;
}
  
AudioBuf AudioBuf::repeat( int repetitions ) const{
  AudioBuf ret = AudioBuf( this->get_length() * repetitions );
  unsigned int i;
  for( i=0; i < ret.get_num_samples(); i++ ){
    ret[i] = this->data[ i % this->get_num_samples() ];
  }
  return ret;
}

bool AudioBuf::write_to_file( string filename ) const{
  cerr << "audio file writing unimplemented"<<endl;
  return true;
}

AudioRequest::AudioRequest( const AudioBuf & buf ){
  this->progress_index = 0; // set to zero so playback starts at beginning
  this->audio = buf;
}

/** this constructor is used for recording, so we have to allocate an new 
    audio buffer */
AudioRequest::AudioRequest( duration_t len ){
  this->progress_index = 0; // set to zero so playback starts at beginning
  this->audio = *(new AudioBuf( len ));  
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

  // set global SAMPLE_RATE
  int i;
  const int rates[6] = {96000, 48000, 44100, 22050, 16000, 8000};
  for( i=0; i<6; i++ ){
    if( Pa_IsFormatSupported( &this->in_params,NULL,rates[i] ) == paNoError &&
	Pa_IsFormatSupported( NULL,&this->out_params,rates[i] ) == paNoError ){
      SAMPLE_RATE=rates[i];
      cerr << "Sample rate = "<<SAMPLE_RATE<<"Hz"<<endl;
      return;
    }
  }
  cerr<<"ERROR: no supported sample rate found!"<<endl;
}

pair<unsigned int,unsigned int> AudioDev::prompt_device(){
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
  cout << endl << "Please enter an input device number: " << endl;
  cin >> in_dev_num;
  cout << endl << "Please enter an output device number: " << endl;
  cin >> out_dev_num;

  return make_pair( in_dev_num, out_dev_num );
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
      *out++ = req->audio[i];  /* left */
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

/** Similiar to player_callback, but "wrap around" buffer indices */
int AudioDev::oscillator_callback( const void *inputBuffer, void *outputBuffer,
				   unsigned long framesPerBuffer,
				   const PaStreamCallbackTimeInfo* timeInfo,
				   PaStreamCallbackFlags statusFlags,
				   void *userData ){
  /* Cast data passed through stream to our structure. */
  AudioRequest *req = (AudioRequest*)userData; 
  sample_t *out = (sample_t*)outputBuffer;
  (void) inputBuffer; /* Prevent unused variable warning. */

  unsigned int i, total_samples = req->audio.get_num_samples();
  for( i=0; i<framesPerBuffer; i++ ){
    *out++ = req->audio[ ( req->progress_index + i ) % total_samples ]; // left
    *out++ = 0; // right
  }
  // update progress index
  req->progress_index = ( req->progress_index + i ) % total_samples; 
  return 0; // returning 0 makes the stream stay alive.
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
      req->audio[i] = *in++;  /* there is only one channel */
    }
  }
  req->progress_index = i; // update progress index
  // we would return 1 when playback is complete (ie when we want the stream
  // to die), otherwise return 0
  return req->done();
}

PaStream* AudioDev::nonblocking_play( const AudioBuf & buf ){
  PaStream *stream;
  AudioRequest *play_request = new AudioRequest( buf );
  /* Open an audio I/O stream. */
  check_error( Pa_OpenStream( 
	 &stream,
	 NULL,          /* no input channels */
	 &(this->out_params),
	 SAMPLE_RATE,
	 FRAMES_PER_BUFFER,
	 paNoFlag,
	 AudioDev::player_callback, /* this is your callback function */
	 play_request ) ); /*This is a pointer that will be passed to
			     your callback*/
  // start playback
  check_error( Pa_StartStream( stream ) );
  // caller is responsible for freeing stream
  return stream;
}

PaStream* AudioDev::nonblocking_play_loop( const AudioBuf & buf ){
  PaStream *stream;
  AudioRequest *play_request = new AudioRequest( buf );
  /* Open an audio I/O stream. */
  check_error( Pa_OpenStream( 
	 &stream,
	 NULL,          /* no input channels */
	 &(this->out_params),
	 SAMPLE_RATE,
	 FRAMES_PER_BUFFER,   /* frames per buffer */
	 paNoFlag,
	 AudioDev::oscillator_callback, /* this is your callback function */
	 play_request ) ); /*This is a pointer that will be passed to
			     your callback*/
  // start playback
  check_error( Pa_StartStream( stream ) );
  // caller is responsible for freeing stream
  return stream;
}

AudioBuf AudioDev::blocking_record( duration_t duration ){
  PaStream *stream;
  AudioRequest *rec_request = new AudioRequest( duration );
  /* Open an audio I/O stream. */
  check_error( Pa_OpenStream (
	 &stream, 
	 &(this->in_params),
	 NULL,       /* no output channels */
	 SAMPLE_RATE,
	 FRAMES_PER_BUFFER,        /* frames per buffer */
	 paNoFlag, 
	 AudioDev::recorder_callback, /* this is your callback function */
	 rec_request ) ); /*This is a pointer that will be passed to
			     your callback*/
  // start recording
  check_error( Pa_StartStream( stream ) );
  // wait until done, sleeping 100ms at a time
  while( !rec_request->done() ) Pa_Sleep( 100 );
  check_error( Pa_CloseStream( stream ) ); // free up stream resource
  return rec_request->audio;
}

/** This function could probably be more accurately (re synchronization)
    implemented by creating a new duplex stream.  For sonar purposes,
    it shouldn't be necessary*/
AudioBuf AudioDev::recordback( const AudioBuf & buf ){
  // play audio
  PaStream* s = nonblocking_play( buf );
  // record echo
  AudioBuf ret = blocking_record( buf.get_length() );
  AudioDev::check_error( Pa_CloseStream( s ) ); // close stream to free up dev
  return ret;
}

void AudioDev::check_error( PaError err ){
  if( err != paNoError )
    cerr << "PortAudio error: " << Pa_GetErrorText( err ) << endl;
}

