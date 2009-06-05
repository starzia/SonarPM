#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include "Frame.hpp"
#include "../sonar.hpp"
#include "../dsp.hpp"
#include "../audio.hpp"

class SonarThread : public wxThread{
public:
  SonarThread( Frame* mainFrame );
  // starts some work
  void* Entry();
private:
  void poll( AudioDev & audio, Config & conf );
  void power_management( AudioDev & audio, Config & conf );
  Frame* mainFrame;
};

