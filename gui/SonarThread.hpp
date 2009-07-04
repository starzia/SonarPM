#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include "Frame.hpp"
#include "../sonar.hpp"
#include "../dsp.hpp"
#include "../audio.hpp"

class SonarThread : public wxThread{
public:
  SonarThread( Frame* mainFrame, bool doPowerManagement );

  // starts some work
  void* Entry();
  /** called when the thread exits - whether it terminates normally or is
    * stopped with Delete() (but not when it is Kill()ed!) */
  void OnExit();
private:
  void poll( AudioDev & audio, Config & conf );
  void power_management( AudioDev & audio, Config & conf );
  // GUI helpers
  void updateGUIThreshold( float thresh );
  void updateGUIDelta( float echo_delta );

  Frame* mainFrame;
  bool doPowerManagement; // if true do power management, else just poll.

 /** these two functions are similar to those in the SysInterface class, but
  * additionally run TestDestroy() periodically to respond to cancellation
  * @return false iff interrupted by thread cancellation event */
  bool waitUntilIdle();
  bool waitUntilActive();
};

/** performs a test of the audio system while updating a status window */
class EchoThread : public wxThread{
public:
  EchoThread( wxDialog* recDialog, wxDialog* playDialog,
              unsigned int rec_dev, unsigned int play_dev );
  ~EchoThread();
  void* Entry();
private:
  // status notification dialogs for playback and recording
  wxDialog *recDialog, *playDialog;
  unsigned int rec_dev, play_dev;
};

/*
// event types for updating status window
const wxEventType recordingDoneCommandEvent = wxNewEventType();
 */