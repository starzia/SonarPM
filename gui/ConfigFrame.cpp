
#include <wx/sizer.h>

#include "ConfigFrame.hpp"
#include "../audio.hpp"

using namespace std;

BEGIN_EVENT_TABLE( ConfigFrame, wxFrame )
EVT_BUTTON( BUTTON_SAVE, ConfigFrame::onSave )
EVT_BUTTON( BUTTON_CANCEL, ConfigFrame::onCancel )
END_EVENT_TABLE()

ConfigFrame::ConfigFrame( const wxString & title, int width, int height ) :
  wxFrame( (wxFrame*)NULL,-1,title,wxDefaultPosition,wxSize(width,height),
	   wxFRAME_NO_TASKBAR | wxSYSTEM_MENU | wxCAPTION
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX //| wxRESIZE_BORDER
	   | wxFULL_REPAINT_ON_RESIZE )
{
  // add controls
  this->panel = new wxPanel( this, wxID_ANY, wxDefaultPosition,
                             this->GetClientSize());
  this->buttonSave = new wxButton( this->panel, BUTTON_SAVE, _T("save"),
				   wxDefaultPosition, wxDefaultSize );
  this->buttonCancel = new wxButton( this->panel, BUTTON_CANCEL, _T("cancel"),
				   wxDefaultPosition, wxDefaultSize );

  const wxString choices[2] = {_T("0"),_T("1")};
  this->playDev = new wxChoice( this->panel, CHOICE_PLAYDEV, wxDefaultPosition,
				wxDefaultSize, 2, choices );
  this->recDev = new wxChoice( this->panel, CHOICE_RECDEV, wxDefaultPosition,
				wxDefaultSize, 2, choices );
  this->modelName;
  this->phoneHome = new wxCheckBox( this->panel, PHONEHOME_ENABLE,
                    _T("enable phone home"), wxDefaultPosition );
  this->phoneHome->SetValue(true);

  // create sizers for layout
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

}

void ConfigFrame::onCancel( wxCommandEvent& event ){
    this->Close();
}
