#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include "Frame.hpp"
#include "../sonar.hpp"
#include "../dsp.hpp"
#include "../audio.hpp"

#define ASSERT( b ) if( !(b) ){ std::cerr<<"ERROR: Assertion "<<#b<<" violated, "<<__FILE__<<':'<<__LINE__<<std::endl; }
/*
#define LOCK( l ) std::cerr << "Locking at " <<__FILE__<<':'<< __LINE__ <<"..."; \
                  wxMutexLocker locker( l ); \
                  std::cerr << "acquired" <<std::endl; \
                  ASSERT( locker.IsOk() );
*/
#define LOCK( l ) wxMutexLocker locker( l );

typedef enum {
  MODE_POWER_MANAGEMENT,
  MODE_POLLING,
  MODE_FREQ_RESPONSE,
  MODE_ECHO_TEST
} sonar_mode;

class SonarThread : public wxThread{
public:
  SonarThread( Frame* mainFrame, sonar_mode mode );

  // starts some work
  void* Entry();
  /** called when the thread exits - whether it terminates normally or is
    * stopped with Delete() (but not when it is Kill()ed!) */
  void OnExit();

  //====================== CONSTANTS =======================================
  // duration of each recording window
  static const duration_t WINDOW_LENGTH;
  // number of windows to consider when calculating echo delta
  static const unsigned int SLIDING_WINDOW;
  // if there was input activity within IDLE_TIME, don't run sonar
  static const duration_t IDLE_TIME;
  static const duration_t SLEEP_LENGTH;
  // if there is input activity within this window of a sleep, then call it a
  // false negative
  static const duration_t FALSE_NEG;
  // how rapidly does dynamic threshold move
  static const float DYN_THRESH_FACTOR;
  // time after which threshold is reset
  static const duration_t RECALIBRATION_INTERVAL;
  // default timeout for naive power managment policy, should never be used in practice
  static const duration_t DISPLAY_TIMEOUT;
  static const frequency DEFAULT_PING_FREQ;
  // just less than min Windows screensaver activation time of one minute
  static const duration_t DUMMY_INPUT_INTERVAL;
  // this is how much higher we expect active state readings to be than absent
  // state.  This determines where the initial threshold is set.
  static const float ACTIVE_GAIN;
  // stop logging after this length of time
  static const duration_t STUDY_LENGTH;
private:
  //======= DATA MEMBERS =======
  Frame* mainFrame;
  sonar_mode mode; // power management, polling, etc.
  float threshold; // sonar threshold
  /** timeout for emulated timeout policy, to be set to OS's value */
  duration_t displayTimeout;

  /** sliding window of sonar readings, index 0 is most recent */
  std::deque<float> windowHistory;
  float windowAvg; // average of window

  /* various events are triggered periodically */
  long lastCalibration;
  long lastUserInputTime;
  long lastDummyInputTime;

  /** the audio stream for playing the ping */
  AudioRequest* pingStrm;

  //======== FUNCTIONS ========
  void poll();
  void power_management();

  // GUI helpers
  void recordAndProcessAndUpdateGUI();
  /** @param setStatus determines whether window's status text is updated with
   *   this reading's values */
  void updateGUI( float echo_delta, float window_avg, float thresh,
                  bool setStatus );
  void reset(); // create a gap in plot and clears sonar history window
  void plotGap(); // helper function for above

  /** sets the screen blanking threshold.
   * @return true if successful false if interrupted by thread cancellation */
  bool updateThreshold();
  void setThreshold( float thresh );
  /** query OS to get display timeout, so that OS policy can be emulated 
    faithfully */
  void setDisplayTimeout();

  /** runs any pending period tasks such as phone home or recalibration
   * @param log_start_time is when the app was first run (when logfile created).
   *   passing this as a param prevents accessing the log file each time.
   * @return true if successful false if interrupted by thread cancellation */
  bool scheduler( long log_start_time );
  
  /** these two functions are similar to those in the SysInterface class, but
  * additionally run TestDestroy() periodically to respond to cancellation
  * @return false iff interrupted by thread cancellation event */
  bool waitUntilIdle();
  bool waitUntilActive();

  /** starts the Ping, if it is not already playing */
  bool resumePing();
  /** stops the Ping, if it is playing */
  bool pausePing();

  /** send a dummy input event to the OS to keep screensaver off */
  void sendDummyInput();
  /** wrapper for SysInterface::idle_seconds().  However, this version accounts
   for dummy keyboard events generated by this program which cause the system's
   idleness timer to be reset.  */
  duration_t true_idle_seconds();

  AudioDev audio;
  Config conf;
  Logger logger;
};

/** performs a test of the audio system while updating a status window */
class EchoThread : public wxThread{
public:
  EchoThread( wxDialog* echoDialog,
              unsigned int rec_dev, unsigned int play_dev );
  ~EchoThread();
  void* Entry();
private:
  // status notification dialogs for playback and recording
  wxDialog *echoDialog;
  unsigned int rec_dev, play_dev;
};
