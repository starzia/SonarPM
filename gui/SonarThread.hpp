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
  static const duration_t WINDOW_LENGTH = (0.5);
  // number of windows to consider when calculating echo delta
  static const unsigned int SLIDING_WINDOW = (10);
  // if there was input activity within IDLE_TIME, don't run sonar
  static const duration_t IDLE_TIME = (1);
  static const duration_t SLEEP_LENGTH = (0.2);
  // if there is input activity within this window of a sleep, then call it a
  // false negative
  static const duration_t FALSE_NEG = (5);
  // how rapidly does dynamic threshold move
  static const float dynamicThreshFactor = 0.8;
  // time between phone home events
  static const duration_t RECALIBRATION_INTERVAL = (3600);
  // timeout for naive power managment policy
  static const duration_t DISPLAY_TIMEOUT = (600); // ten minutes
  static const frequency DEFAULT_PING_FREQ = (22000);

private:
  void poll();
  void power_management();

  // GUI helpers
  void recordAndProcessAndUpdateGUI();
  void updateGUI( float echo_delta, float window_avg, float thresh );
  void reset(); // create a gap in plot and clears sonar history window


  Frame* mainFrame;
  sonar_mode mode; // power management, polling, etc.
  float threshold; // sonar threshold

  /** sliding window of sonar readings, index 0 is most recent */
  std::deque<float> windowHistory;
  float windowAvg; // average of window

  /** sets the screen blanking threshold, if it is unset or stale.
   * @return true if successful false if interrupted by thread cancellation */
  bool updateThreshold();
  void setThreshold( float thresh );

  /** runs any pending period tasks such as phone home or recalibration
   * @param log_start_time is when the app was first run (when logfile created).
   *   passing this as a param prevents accessing the log file each time.
   * @return true if successful false if interrupted by thread cancellation */
  bool scheduler( long log_start_time );

  /* various events are triggered periodically */
  long lastCalibration;

  /** these two functions are similar to those in the SysInterface class, but
  * additionally run TestDestroy() periodically to respond to cancellation
  * @return false iff interrupted by thread cancellation event */
  bool waitUntilIdle();
  bool waitUntilActive();

  AudioDev audio;
  Config conf;
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
