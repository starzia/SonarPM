#pragma once
#include <wx/wx.h>
#include "Frame.hpp"
#include "ConfigFrame.hpp"

#define VERSION "0.73"

class App : public wxApp{
 public:
  virtual bool OnInit();
  virtual int OnExit();
  
  DECLARE_EVENT_TABLE()
private:
  void onShutdown( wxCloseEvent& event );

  Frame* frame;
};
 
DECLARE_APP(App)
