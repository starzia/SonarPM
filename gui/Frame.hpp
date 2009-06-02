#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include "TaskBarIcon.hpp"
#include "PlotPane.hpp"
//#include "SonarThread.hpp"


class Frame : public wxFrame
{
public:
  Frame( const wxString & title, int width, int height );
  ~Frame();
  void OnIconize( wxIconizeEvent& event );
  void OnSize( wxSizeEvent& event );

  // update plot with a new point
  void addPoint( float p );
  // list controls here
  ///wxTextCtrl *theText;
  DECLARE_EVENT_TABLE()
private:
  TaskBarIcon* tbIcon;
  PlotPane* sonar_history;
  wxThread*  sThread;
};

