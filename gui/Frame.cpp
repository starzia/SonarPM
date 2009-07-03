#include "Frame.hpp"
#include "SonarThread.hpp"
#include <wx/sizer.h>
#include <sstream>
#include <wx/event.h>
#include "PlotEvent.hpp"
#include "ConfigFrame.hpp"
using namespace std;


BEGIN_EVENT_TABLE( Frame, wxFrame )
///EVT_MENU    (wxID_EXIT, Frame::OnExit)
///EVT_MENU    (DO_TEST,   Frame::DoTest)
EVT_SIZE( Frame::OnSize )
EVT_ICONIZE( Frame::OnIconize )
EVT_PLOT( wxID_ANY, Frame::onPlotEvent )
EVT_BUTTON( BUTTON_PAUSE, Frame::onPause )
END_EVENT_TABLE()

Frame::Frame( const wxString & title, int width, int height ) : 
  wxFrame( (wxFrame*)NULL,-1,title,wxDefaultPosition,wxSize(width,height),
	   wxFRAME_NO_TASKBAR | wxSYSTEM_MENU | wxCAPTION 
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX //| wxRESIZE_BORDER
	   | wxFULL_REPAINT_ON_RESIZE )
{
  // add controls
  this->CreateStatusBar();
  this->SetStatusText(_T("Hello World"));
  this->panel = new wxPanel( this, wxID_ANY, wxDefaultPosition,
                             this->GetClientSize());
/*
  this->buttonPause = new wxButton( panel, BUTTON_PAUSE, _T("start"),
				    wxDefaultPosition, wxDefaultSize );
  const wxString choices[2] = {_T("polling"),_T("power management")};
  this->choiceMode = new wxChoice( panel, CHOICE_MODE, wxDefaultPosition,
				   wxDefaultSize, 2, choices );
*/
  // add taskbar icon
  this->tbIcon = new TaskBarIcon( this );

  // add sonar Plot
  this->sonarHistory = new PlotPane( panel, wxDefaultPosition,
				     panel->GetClientSize() );
  this->sonarHistory->setHistoryLength( 30 );
  
  // create sizers for layout
/*
  wxBoxSizer* sizer3 = new wxBoxSizer( wxVERTICAL );
  sizer3->Add( new wxStaticText( panel, wxID_ANY, _T("operating mode:")));
  sizer3->Add( this->choiceMode, 1, wxALL | wxEXPAND, 5 );
  sizer3->Add( this->buttonPause, 1, wxALL | wxEXPAND, 5 );
*/
  wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );
  sizer2->Add( this->sonarHistory,
	      1, /* vertically stretchable */
	      wxEXPAND ); /* horizontally stretchable */
/*
  sizer2->Add( sizer3, 0, wxALL, 10 ); // 10pt border
 */
  panel->SetSizer(sizer2);
  sizer2->SetSizeHints(panel); // set sze hints to honour min size

  wxBoxSizer* sizer1 = new wxBoxSizer( wxHORIZONTAL );
  sizer1->Add( this->panel );
  this->SetSizer(sizer1);
  sizer1->SetSizeHints(this);

  this->sThread=NULL; // prevent initially dangling pointer
  ///this->startSonar();

  // pop up config window
  ConfigFrame* conf = new ConfigFrame( this,_T("Configuration"),500,500);
  conf->Show(true);
}

Frame::~Frame(){
  // clean up taskbar
  this->tbIcon->RemoveIcon();
}

void Frame::nullifyThread(){
  wxCriticalSectionLocker lock( this->threadLock );
  this->sThread = NULL;
}

void Frame::startSonar( ){
  // The following lock prevents multiple new threads from starting.
  // It is released automatically by its destructor.
  wxCriticalSectionLocker locker( this->threadLock );
  if( !this->sThread ){ //start only if stopped
    // start sonar processing thread
    this->sThread = new SonarThread( this );
    if( this->sThread->Create( ) == wxTHREAD_NO_ERROR ){
      this->sThread->Run( );
      this->buttonPause->SetLabel( _T( "pause" ) );
    }
  }
}

void Frame::stopSonar(){
  // stop sonar processing thread
  wxCriticalSectionLocker locker( this->threadLock );
  if( this->sThread ){ // stop only if started
    if( this->sThread->Delete() == wxTHREAD_NO_ERROR ){
      this->buttonPause->SetLabel( _T( "continue" ) );
    }
  }
}

void Frame::OnIconize( wxIconizeEvent& event ){
  // is this necessary?
  this->Show(!event.Iconized());
}

void Frame::OnSize( wxSizeEvent& event ){
  //this->Refresh();
  //this->Update();
}

void Frame::onPlotEvent(PlotEvent& event){
  if( event.getType() == PLOT_EVENT_POINT ){
    this->sonarHistory->addPoint( event.getVal() );
    
    wxString s;
    s.Printf( wxT("Last reading: %f"), event.getVal() );
    this->SetStatusText(s);
    
  }else if( event.getType() == PLOT_EVENT_THRESHOLD ){
    this->sonarHistory->setThreshold( event.getVal() );
    wxString s;
    s.Printf( wxT("Threshold updated to: %f"), event.getVal() );
    this->SetStatusText(s);
  }
  this->Refresh();
  this->Update();
}

void Frame::onPause(wxCommandEvent& event){
  if( this->buttonPause->GetLabel() == _T("pause") ){
    this->stopSonar();
  }else{ // "continue" or "start"
    this->startSonar();
  }
}
