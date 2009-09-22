#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
///#include <wx/event.h> // for wxThreadEvent
#include "TaskBarIcon.hpp"
#include "PlotPane.hpp"
#include "SonarEvent.hpp"
#include "../Logger.hpp"

class Frame : public wxFrame
{
public:
  Frame( const wxString & title, int width, int height );
  ~Frame();

#ifdef PLATFORM_WINDOWS
  /** windows message handling. We use this for logging suspend/resume events */
  WXLRESULT MSWWindowProc( WXUINT message, WXWPARAM wParam, WXLPARAM lParam );
#endif

  void onIconize( wxIconizeEvent& event );
  void onSize( wxSizeEvent& event );
  void onClose( wxCloseEvent& event );
  void onModeSwitch( wxCommandEvent& event );
  void onConfig( wxCommandEvent& event );

  void onSonarEvent(SonarEvent& event);
  void onPause( wxCommandEvent& event );

  void startSonar();
  void stopSonar();
  bool isSonarRunning();

  /** routines to run if this is the first time the app is run */
  void firstTime();
  /** unminimize window, bring to front, and focus on it */
  void restore();

  DECLARE_EVENT_TABLE()
private:
  // update plot with a new point
  void addPoint( float echo_delta, float window_avg );

  // wait for sonar thread to stop running
  void threadWait();

  // controls
  wxPanel *panel; // container for all controls
  wxButton* buttonPause;
  wxChoice* choiceMode;
  wxButton* buttonConfig;

  TaskBarIcon* tbIcon;
  PlotPane* sonarHistory;
  wxThread*  sThread;
  wxMutex threadLock; // to protect access to thread pointer
  Logger logger;
};


//=============================== CONSTANTS ============================
const int  BUTTON_PAUSE = wxID_HIGHEST + 1;
const int  CHOICE_MODE = wxID_HIGHEST + 2;
const int  BUTTON_CONFIG = wxID_HIGHEST + 3;

