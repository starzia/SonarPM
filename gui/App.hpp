#pragma once
#include <wx/wx.h>
#include "Frame.hpp"
#include "ConfigFrame.hpp"

class App : public wxApp{
 public:
  virtual bool OnInit();
  virtual int OnExit();
private:
  Frame* frame;
  ConfigFrame* confFrame;
};
 
DECLARE_APP(App)

