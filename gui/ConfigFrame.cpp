
#include <wx/sizer.h>

#include "ConfigFrame.hpp"
#include "SonarThread.hpp" // for echo test thread
#include "../audio.hpp" // for listing audio devices

using namespace std;

BEGIN_EVENT_TABLE( ConfigFrame, wxDialog )
EVT_BUTTON( BUTTON_SAVE, ConfigFrame::onSave )
EVT_BUTTON( BUTTON_CANCEL, ConfigFrame::onCancel )
EVT_BUTTON( BUTTON_ECHOTEST, ConfigFrame::onEchoTest )
//EVT_BUTTON( recordingDoneCommandEvent, ConfigFrame::onRecordingDone )
END_EVENT_TABLE()

ConfigFrame::ConfigFrame( Frame* p, const wxString & title,
                          int width, int height ) :
  wxDialog( p,-1,title,wxDefaultPosition,wxSize(width,height),
            wxDEFAULT_DIALOG_STYLE )
/*        wxFRAME_NO_TASKBAR | wxSYSTEM_MENU | wxCAPTION
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX //| wxRESIZE_BORDER
	   | wxFULL_REPAINT_ON_RESIZE ) */
{
  this->parent = p;
  // add controls
  this->panel = new wxPanel( this, wxID_ANY, wxDefaultPosition,
                             this->GetClientSize());
  this->buttonSave = new wxButton( this->panel, BUTTON_SAVE, _T("save"),
				   wxDefaultPosition, wxDefaultSize );
  this->buttonCancel = new wxButton( this->panel, BUTTON_CANCEL, _T("cancel"),
				   wxDefaultPosition, wxDefaultSize );
  this->buttonEchoTest = new wxButton( this->panel, BUTTON_ECHOTEST, 
                  _T("echo test"), wxDefaultPosition, wxDefaultSize );
  this->buttonFreqResponse = new wxButton( this->panel, BUTTON_FREQRESPONSE,
                  _T("frequency response"), wxDefaultPosition, wxDefaultSize );


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
  }

  // create sizers for layout
  wxBoxSizer* sizer4 = new wxBoxSizer( wxHORIZONTAL );
  sizer4->Add( this->buttonEchoTest, 1, wxALL | wxEXPAND, 5 );
  sizer4->Add( this->buttonFreqResponse, 1, wxALL | wxEXPAND, 5 );

  wxBoxSizer* sizer3 = new wxBoxSizer( wxHORIZONTAL );
  sizer3->Add( this->buttonSave, 1, wxALL | wxEXPAND, 5 );
  sizer3->Add( this->buttonCancel, 1, wxALL | wxEXPAND, 5 );
  
  wxBoxSizer* sizer2 = new wxBoxSizer( wxVERTICAL );
  sizer2->Add( this->phoneHome, 1, wxALL | wxEXPAND, 5 );
  sizer2->Add( new wxStaticText( panel, wxID_ANY,
               _T("playback audio device:") ) );
  sizer2->Add( this->playDev, 1, wxALL | wxEXPAND, 5 );
  sizer2->Add( new wxStaticText( panel, wxID_ANY,
               _T("recording audio device:") ) );
  sizer2->Add( this->recDev, 1, wxALL | wxEXPAND, 5 );

  sizer2->Add( new wxStaticText( panel, wxID_ANY,
                                 _T("\nAudio testing features:")));
  sizer2->Add( sizer4, 1, wxALL | wxEXPAND );
  sizer2->Add( new wxStaticText( panel, wxID_ANY,
                              _T("\nTo continue choose one of the following:")));
  sizer2->Add( sizer3, 1, wxALL | wxEXPAND );
    panel->SetSizer( sizer2 );
  sizer2->SetSizeHints( this->panel ); // set size hints to honour min size

  wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
  sizer1->Add( this->panel );
  this->SetSizer( sizer1 );
  sizer1->SetSizeHints( this );
}

ConfigFrame::~ConfigFrame(){}


void ConfigFrame::onSave( wxCommandEvent& event ){
  // change configuration based on widget settings
  this->conf.rec_dev = this->recDev->GetSelection();
  this->conf.play_dev = this->playDev->GetSelection();
  this->conf.allow_phone_home = this->phoneHome->IsChecked();

  // save changes to file
  this->conf.write_config_file();

  this->Close();
  ///this->parent->startSonar();
}

void ConfigFrame::onCancel( wxCommandEvent& event ){
  this->Close();
  ///this->parent->startSonar();
}

void ConfigFrame::onEchoTest( wxCommandEvent& event ){
  // pop up status window window
  wxDialog *playback = new wxDialog( this,-1,_T("Playing back recording..."),
                                   wxDefaultPosition,wxDefaultSize,
                                   wxSTAY_ON_TOP | wxCAPTION );
  wxDialog *recording = new wxDialog( this,-1,_T("Recording from microphone..."),
                                   wxDefaultPosition,wxDefaultSize,
                                   wxSTAY_ON_TOP | wxCAPTION );

  // start echo test thread, linked to status window
  EchoThread* thread = new EchoThread( recording, playback,
                                      this->conf.rec_dev, this->conf.play_dev );
  if( thread->Create() == wxTHREAD_NO_ERROR )
    thread->Run( );
}
