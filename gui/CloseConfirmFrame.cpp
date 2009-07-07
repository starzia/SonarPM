
#include <wx/sizer.h>

#include "CloseConfirmFrame.hpp"

using namespace std;

BEGIN_EVENT_TABLE( CloseConfirmFrame, wxDialog )
EVT_BUTTON( BUTTON_EXIT, CloseConfirmFrame::onExit )
EVT_BUTTON( BUTTON_MINIMIZE, CloseConfirmFrame::onMinimize )
END_EVENT_TABLE()

CloseConfirmFrame::CloseConfirmFrame( Frame* p ) :
  wxDialog( p,-1,_T("Really Quit?"),wxDefaultPosition,wxDefaultSize, //wxSize(width,height),
            wxDEFAULT_DIALOG_STYLE ), parent(p)
{
  // add controls
  this->panel = new wxPanel( this, wxID_ANY, wxDefaultPosition,
                             this->GetClientSize());
  this->buttonExit = new wxButton( this->panel, BUTTON_EXIT, _T("exit"),
				   wxDefaultPosition, wxDefaultSize );
  this->buttonMinimize = new wxButton( this->panel, BUTTON_MINIMIZE, _T("minimize"),
				   wxDefaultPosition, wxDefaultSize );

  // create sizers for layout
  wxBoxSizer* sizer3 = new wxBoxSizer( wxHORIZONTAL );
  sizer3->Add( this->buttonMinimize, 1, wxALL | wxEXPAND, 5 );
  sizer3->Add( this->buttonExit, 1, wxALL | wxEXPAND, 5 );
  
  wxBoxSizer* sizer2 = new wxBoxSizer( wxVERTICAL );
  wxString prompt = 
          _T("Choose 'minimize' to allow sonar to run in the background.\n"
          "To quit the application, choose 'exit':");
  sizer2->Add( new wxStaticText( panel, wxID_ANY, prompt ) );
  sizer2->Add( sizer3, 1, wxALL | wxEXPAND );
    panel->SetSizer( sizer2 );
  sizer2->SetSizeHints( this->panel ); // set size hints to honour min size

  wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
  sizer1->Add( this->panel );
  this->SetSizer( sizer1 );
  sizer1->SetSizeHints( this );
}

// TODO: deallocate buttons, textfields, to prevent mem leakage
CloseConfirmFrame::~CloseConfirmFrame(){}

void CloseConfirmFrame::onMinimize( wxCommandEvent& event ){
  this->parent->Show(false);
  this->EndModal(1);
}

void CloseConfirmFrame::onExit( wxCommandEvent& event ){
  this->parent->Destroy();
  this->EndModal(1);
}