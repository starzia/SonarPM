#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
///#include <wx/event.h> // for wxThreadEvent
#include "TaskBarIcon.hpp"
#include "PlotPane.hpp"
#include "PlotEvent.hpp"

class Frame : public wxFrame
{
public:
  Frame( const wxString & title, int width, int height );
  ~Frame();
  void onIconize( wxIconizeEvent& event );
  void onSize( wxSizeEvent& event );
  void onClose( wxCloseEvent& event );
  void onModeSwitch( wxCommandEvent& event );

  void onPlotEvent(PlotEvent& event);
  void onPause( wxCommandEvent& event );
  void nullifyThread();

  void startSonar();
  void stopSonar();

  wxThread*  sThread; // need access from App
  DECLARE_EVENT_TABLE()
private:
  // update plot with a new point
  void addPoint( float p );

  // controls
  wxPanel *panel; // container for all controls
  wxButton* buttonPause;
  wxChoice* choiceMode;

  TaskBarIcon* tbIcon;
  PlotPane* sonarHistory;
  wxCriticalSection threadLock; // to protect access to thread pointer
};


//=============================== CONSTANTS ============================
const int  BUTTON_PAUSE = wxID_HIGHEST + 1;
const int  CHOICE_MODE = wxID_HIGHEST + 2;


