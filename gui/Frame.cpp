#include "Frame.hpp"
#include "SonarThread.hpp"
#include <wx/sizer.h>
#include <sstream>
#include <wx/event.h>
#include "PlotEvent.hpp"
using namespace std;


BEGIN_EVENT_TABLE( Frame, wxFrame )
///EVT_MENU    (wxID_EXIT, Frame::OnExit)
///EVT_MENU    (DO_TEST,   Frame::DoTest)
///EVT_BUTTON  (BUTTON1,   Frame::OnButton1)
EVT_SIZE( Frame::OnSize )
EVT_ICONIZE( Frame::OnIconize )
EVT_PLOT( wxID_ANY, Frame::onPlotEvent)
END_EVENT_TABLE()

Frame::Frame( const wxString & title, int width, int height ) : 
  wxFrame( (wxFrame*)NULL,-1,title,wxDefaultPosition,wxSize(width,height),
	   wxFRAME_NO_TASKBAR | wxSYSTEM_MENU | wxCAPTION 
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX //|wxRESIZE_BORDER 
	   | wxFULL_REPAINT_ON_RESIZE )
{
  // create sizer for layout
  wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

  // add controls
  this->CreateStatusBar();
  this->SetStatusText(_T("Hello World"));

  // add taskbar icon
  this->tbIcon = new TaskBarIcon( this );

  // add sonar Plot
  this->sonar_history = new PlotPane( this, wxDefaultPosition, 
				      this->GetClientSize() );
  this->sonar_history->setHistoryLength( 30 );

  sizer->Add( this->sonar_history,
	      1, // vertically stretchable
	      wxEXPAND ); // horizontally stretchable
  this->SetSizer(sizer);
  sizer->SetSizeHints(this); // set sze hints to honour min size

  // start sonar processing thread
  this->sThread = new SonarThread(this);
  if( this->sThread->Create() == wxTHREAD_NO_ERROR )
    this->sThread->Run();
}

Frame::~Frame(){
  // clean up taskbar
  this->tbIcon->RemoveIcon();
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
    this->sonar_history->addPoint( event.getVal() );
    
    wxString s;
    s.Printf( wxT("Last reading: %f"), event.getVal() );
    this->SetStatusText(s);
    
  }else if( event.getType() == PLOT_EVENT_THRESHOLD ){
    this->sonar_history->setThreshold( event.getVal() );
    wxString s;
    s.Printf( wxT("Threshold updated to: %f"), event.getVal() );
    this->SetStatusText(s);
  }
  this->Refresh();
  this->Update();
}

