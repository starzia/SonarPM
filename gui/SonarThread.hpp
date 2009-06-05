#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include "Frame.hpp"

// how to declare a custom event. this can go in a header
DECLARE_EVENT_TYPE(PLOT_EVENT, wxID_ANY)

class SonarThread : public wxThread{
public:
  SonarThread( Frame* mainFrame );
  // starts some work
  void* Entry();
private:
  Frame* mainFrame;
};

