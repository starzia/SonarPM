#pragma once
#include <wx/wx.h>
#include "../sonar.hpp" // for config file
#include "Frame.hpp"

class ConfigFrame : public wxDialog
{
public:
  ConfigFrame( Frame* parent, const wxString & title );
  ~ConfigFrame();
  void onSave( wxCommandEvent& event );
  void onCancel( wxCommandEvent& event );
  void onEchoTest( wxCommandEvent& event );
  void onDefaults( wxCommandEvent& event );


  DECLARE_EVENT_TABLE()
private:
  /** load default configuration and save it */
  void loadDefaults();
  /** save settings that are currently set in widgets
   * @return true if all settings were valid and file was saved */
  bool saveSettings();

  Config conf;
  Frame* parent;

  wxPanel *panel; // container for all controls

  // controls
  ///wxTextCtrl *modelName;
  wxCheckBox *phoneHome;
  wxChoice *playDev;
  wxChoice *recDev;
  wxButton *buttonSave;
  wxButton *buttonCancel;
  wxButton *buttonEchoTest;
  wxButton *buttonFreqResponse;
  wxButton *buttonDefaults;
};

//=============================== CONSTANTS ============================
const int  BUTTON_SAVE = wxID_HIGHEST + 101;
const int  BUTTON_CANCEL = wxID_HIGHEST + 102;
const int  PHONEHOME_ENABLE = wxID_HIGHEST + 103;
const int  CHOICE_PLAYDEV = wxID_HIGHEST + 104;
const int  CHOICE_RECDEV = wxID_HIGHEST + 105;
const int  BUTTON_ECHOTEST = wxID_HIGHEST + 106;
const int  BUTTON_FREQRESPONSE = wxID_HIGHEST + 107;
const int  BUTTON_DEFAULTS = wxID_HIGHEST + 109;
const int  TEXT_MODEL = wxID_HIGHEST + 111;

// TODO: add advanced controls
const int  SLIDER_PERIOD = wxID_HIGHEST + 112;
const int  SLIDER_THESH_FACTOR = wxID_HIGHEST + 113;
const int  SLIDER_FREQ = wxID_HIGHEST + 114;