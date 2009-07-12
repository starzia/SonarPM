
#include <wx/sizer.h>

#include "ConfigFrame.hpp"
#include "SonarThread.hpp" // for echo test thread
#include "../audio.hpp" // for listing audio devices

using namespace std;

BEGIN_EVENT_TABLE( ConfigFrame, wxDialog )
EVT_BUTTON( BUTTON_SAVE, ConfigFrame::onSave )
EVT_BUTTON( BUTTON_CANCEL, ConfigFrame::onCancel )
EVT_BUTTON( BUTTON_ECHOTEST, ConfigFrame::onEchoTest )
EVT_BUTTON( BUTTON_DEFAULTS, ConfigFrame::onDefaults )
END_EVENT_TABLE()

ConfigFrame::ConfigFrame( Frame* p, const wxString & title ) :
  wxDialog( p,-1,title,wxDefaultPosition,wxSize(500,500),
            wxDEFAULT_DIALOG_STYLE )
/*        wxFRAME_NO_TASKBAR | wxSYSTEM_MENU | wxCAPTION
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX //| wxRESIZE_BORDER
	   | wxFULL_REPAINT_ON_RESIZE ) */
{
  this->parent = p;
  // add controls
  this->panel = new wxPanel( this, wxID_ANY, wxDefaultPosition,
                             this->GetClientSize());
  this->modelName = new wxTextCtrl( this->panel, TEXT_MODEL, _T(""),
                                    wxDefaultPosition, wxDefaultSize );
  this->buttonSave = new wxButton( this->panel, BUTTON_SAVE, _T("save"),
				   wxDefaultPosition, wxDefaultSize );
  this->buttonCancel = new wxButton( this->panel, BUTTON_CANCEL, _T("cancel"),
				   wxDefaultPosition, wxDefaultSize );
  this->buttonEchoTest = new wxButton( this->panel, BUTTON_ECHOTEST, 
                  _T("echo test"), wxDefaultPosition, wxDefaultSize );
  this->buttonEchoTest->Disable(); // initially disabled
  this->buttonFreqResponse = new wxButton( this->panel, BUTTON_FREQRESPONSE,
                  _T("frequency response"), wxDefaultPosition, wxDefaultSize );
  this->buttonFreqResponse->Disable(); // initially disabled
  this->buttonDefaults = new wxButton( this->panel, BUTTON_DEFAULTS,
                  _T("reset to defaults"), wxDefaultPosition, wxDefaultSize );



  // build a wxString array with names of audio devices
  AudioDev audio;
  vector<string> devices = audio.list_devices();
  wxString* dev_arr = new wxString[ devices.size() ];
  int i; for( i=0; i<devices.size(); i++ ){
      dev_arr[i] = wxString(devices[i].c_str(),wxConvUTF8);
  }

  //const wxString choices[2] = {_T("0"),_T("1")};
  this->playDev = new wxChoice( this->panel, CHOICE_PLAYDEV, wxDefaultPosition,
				wxDefaultSize, devices.size(), dev_arr );
  this->recDev = new wxChoice( this->panel, CHOICE_RECDEV, wxDefaultPosition,
				wxDefaultSize, devices.size(), dev_arr );
  delete [] dev_arr;
  this->modelName;
  this->phoneHome = new wxCheckBox( this->panel, PHONEHOME_ENABLE,
                    _T("enable phone home"), wxDefaultPosition );
  this->phoneHome->SetValue(true);

  // load previous configuration from file, if available
  if( this->conf.load( SysInterface::config_dir()+CONFIG_FILENAME ) ){
    this->recDev->SetSelection( this->conf.rec_dev );
    this->playDev->SetSelection( this->conf.play_dev );
    this->phoneHome->SetValue( this->conf.allow_phone_home );

    // enable test button(s)
    this->buttonEchoTest->Enable(true);
  }else{
    //load defaults
    this->loadDefaults();
  }

  // create sizers for layout
  wxBoxSizer* sizer5 = new wxStaticBoxSizer(wxVERTICAL, panel, _T("Settings"));
  sizer5->Add( new wxStaticText( panel, wxID_ANY,
           _T("Computer description.  For example: 'Dell Inspiron 8600'") ) );
  sizer5->Add( this->modelName, 1, wxALL | wxEXPAND, 5 );
  sizer5->Add( this->phoneHome, 1, wxALL | wxEXPAND, 5 );
  sizer5->Add( new wxStaticText( panel, wxID_ANY,
               _T("playback audio device:") ) );
  sizer5->Add( this->playDev, 1, wxALL | wxEXPAND, 5 );
  sizer5->Add( new wxStaticText( panel, wxID_ANY,
               _T("recording audio device:") ) );
  sizer5->Add( this->recDev, 1, wxALL | wxEXPAND, 5 );

  wxBoxSizer* sizer4 = new wxBoxSizer( wxHORIZONTAL );
  sizer4->Add( this->buttonEchoTest, 1, wxALL | wxEXPAND, 5 );
  sizer4->Add( this->buttonFreqResponse, 1, wxALL | wxEXPAND, 5 );

  wxBoxSizer* sizer3 = new wxBoxSizer( wxHORIZONTAL );
  sizer3->Add( this->buttonSave, 1, wxALL | wxEXPAND, 5 );
  sizer3->Add( this->buttonCancel, 1, wxALL | wxEXPAND, 5 );
  
  wxBoxSizer* sizer2 = new wxBoxSizer( wxVERTICAL );
  sizer2->Add( this->buttonDefaults, 0, wxALL | wxEXPAND, 5 );
  sizer2->Add( sizer5, 0, wxALL, 10 );
  sizer2->Add( new wxStaticText( panel, wxID_ANY,
                                 _T("\nAudio testing features:")));
  sizer2->Add( sizer4, 0, wxALL | wxEXPAND );
  sizer2->Add( new wxStaticText( panel, wxID_ANY,
                              _T("\nTo continue choose one of the following:")));
  sizer2->Add( sizer3, 0, wxALL | wxEXPAND );
    panel->SetSizer( sizer2 );
  sizer2->SetSizeHints( this->panel ); // set size hints to honour min size

  wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
  sizer1->Add( this->panel, 1, wxALL, 10 );
  this->SetSizer( sizer1 );
  sizer1->SetSizeHints( this );
}

ConfigFrame::~ConfigFrame(){}


void ConfigFrame::onSave( wxCommandEvent& event ){
  // try to save settings
  if( this->saveSettings() ){
    this->EndModal(BUTTON_SAVE);
  }else{
    this->EndModal(BUTTON_CANCEL); // emulate a cancel button click
  }
}

bool ConfigFrame::saveSettings(){
  // check to see that nothing was left blank
  if( this->conf.rec_dev == wxNOT_FOUND || this->conf.play_dev == wxNOT_FOUND )
      return false;

  // test validity of audio device choices
  AudioDev test_audio;
  if( !test_audio.choose_device( this->recDev->GetSelection(),
                                 this->playDev->GetSelection() ) ){
      return false;
      // TODO: message box "invalid choice"
    // TODO: reset to defaults.
  }

  // change configuration based on widget settings
  this->conf.rec_dev = this->recDev->GetSelection();
  this->conf.play_dev = this->playDev->GetSelection();
  this->conf.allow_phone_home = this->phoneHome->IsChecked();
  this->conf.ping_freq = DEFAULT_PING_FREQ; // note that this has no widget yet.

  // save changes to file
  return this->conf.write_config_file();
}

void ConfigFrame::onCancel( wxCommandEvent& event ){
  this->EndModal(BUTTON_CANCEL);
  ///this->parent->startSonar();
}

void ConfigFrame::onDefaults( wxCommandEvent& event ){
  this->loadDefaults();
}

void ConfigFrame::loadDefaults(){
  unsigned int defaultInput = Pa_GetDefaultInputDevice();
  unsigned int defaultOutput = Pa_GetDefaultOutputDevice();
  if( defaultInput == paNoDevice ) defaultInput = 0;
  if( defaultOutput == paNoDevice ) defaultOutput = 0;
  // update widgets
  this->recDev->SetSelection( defaultInput );
  this->playDev->SetSelection( defaultOutput );
  this->phoneHome->SetValue(true);
  // save config
  this->saveSettings();
}

void ConfigFrame::onEchoTest( wxCommandEvent& event ){
  // pop up status window window
  wxDialog *echoDiag = new wxDialog( this,-1,
          _T("Recording from mic, then playing back"),
                                   wxDefaultPosition,wxDefaultSize,
                                   wxSTAY_ON_TOP | wxCAPTION );
  echoDiag->ShowModal(); //(true);

  // start echo test thread, linked to status window
  EchoThread* thread = new EchoThread( echoDiag,
                                      this->conf.rec_dev, this->conf.play_dev );
  if( thread->Create() == wxTHREAD_NO_ERROR )
    thread->Run( );
}
