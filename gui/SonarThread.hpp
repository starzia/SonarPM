#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include "Frame.hpp"

class SonarThread : public wxThread{
public:
  SonarThread( Frame* mainFrame );
  // starts some work
  void* Entry();
private:
  Frame* mainFrame;
};

