#pragma once
#include <wx/wx.h>

class ConfigFrame : public wxFrame
{
public:
  ConfigFrame( const wxString & title, int width, int height );
  ~ConfigFrame();
  void onSave( wxCommandEvent& event );
  void onCancel( wxCommandEvent& event );

  DECLARE_EVENT_TABLE()
private:
  wxPanel *panel; // container for all controls

  // controls
  wxTextCtrl *modelName;
  wxCheckBox *phoneHome;
  wxChoice *playDev;
  wxChoice *recDev;
  wxButton *buttonSave;
  wxButton *buttonCancel;
};

//=============================== CONSTANTS ============================
const int  BUTTON_SAVE = wxID_HIGHEST + 101;
const int  BUTTON_CANCEL = wxID_HIGHEST + 102;
const int  PHONEHOME_ENABLE = wxID_HIGHEST + 103;
const int  CHOICE_PLAYDEV = wxID_HIGHEST + 104;
const int  CHOICE_RECDEV = wxID_HIGHEST + 105;

// TODO: add advanced controls
const int  TEXT_MODEL = wxID_HIGHEST + 111;
const int  SLIDER_PERIOD = wxID_HIGHEST + 112;
const int  SLIDER_THESH_FACTOR = wxID_HIGHEST + 113;
const int  SLIDER_FREQ = wxID_HIGHEST + 114;