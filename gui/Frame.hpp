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
  // list controls here
  ///wxTextCtrl *theText;

  DECLARE_EVENT_TABLE()
private:
  // redraw window
  void draw();

  TaskBarIcon* tbIcon;
  PlotPane* sonar_history;
  wxThread*  sThread;
};

