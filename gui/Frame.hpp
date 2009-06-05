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
  void OnIconize( wxIconizeEvent& event );
  void OnSize( wxSizeEvent& event );

  void onPlotEvent(PlotEvent& event);
  // list controls here
  ///wxTextCtrl *theText;
  DECLARE_EVENT_TABLE()
private:
  // update plot with a new point
  void addPoint( float p );

  TaskBarIcon* tbIcon;
  PlotPane* sonar_history;
  wxThread*  sThread;
};

