#pragma once
#include <wx/wx.h>
#include "Frame.hpp"

class CloseConfirmFrame : public wxDialog
{
public:
  CloseConfirmFrame( Frame* parent );
  ~CloseConfirmFrame();
  void onExit( wxCommandEvent& event );
  void onMinimize( wxCommandEvent& event );

  DECLARE_EVENT_TABLE()
private:
  Frame* parent;

  wxPanel *panel; // container for all controls

  // controls
  wxButton *buttonExit;
  wxButton *buttonMinimize;
};

//=============================== CONSTANTS ============================
const int  BUTTON_EXIT = wxID_HIGHEST + 201;
const int  BUTTON_MINIMIZE = wxID_HIGHEST + 202;
