#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include "Frame.hpp"
#include "../sonar.hpp"
#include "../dsp.hpp"
#include "../audio.hpp"

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

private:
  void poll();
  void power_management();
  // GUI helpers
  void recordAndProcessAndUpdateGUI();
  void updateGUI( float echo_delta, float window_avg, float thresh );
  void updateGUIThreshold( float thresh );
  void reset(); // create a gap in plot and clears sonar history window


  Frame* mainFrame;
  sonar_mode mode; // power management, polling, etc.

  /** sliding window of sonar readings, index 0 is most recent */
  std::deque<float> windowHistory;
  float windowAvg; // average of window
  
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
