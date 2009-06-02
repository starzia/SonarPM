#include "Frame.hpp"
#include "SonarThread.hpp"

Frame::Frame( const wxString & title, int width, int height ) : 
  wxFrame( (wxFrame*)NULL,-1,title,wxDefaultPosition,wxSize(width,height),
	   wxFRAME_NO_TASKBAR | wxSYSTEM_MENU | wxCAPTION 
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX) //wxRESIZE_BORDER
{
  // add controls
  this->CreateStatusBar();
  this->SetStatusText(_T("Hello World"));

  // add taskbar icon
  this->tbIcon = new TaskBarIcon( this );

  // add sonar Plot
  this->sonar_history = new PlotPane( this, wxDefaultPosition, 
				      this->GetClientSize() );
  this->sonar_history->setHistoryLength( 10 );
  this->sonar_history->addPoint( 1.2 ); 
  this->sonar_history->addPoint( 1.2 ); 
  this->sonar_history->addPoint( 1.8 );
  this->sonar_history->addPoint( 0.2 );

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
  this->draw();
}

void Frame::draw(){
  this->Refresh(); //sonar_history->paintNow();
}

BEGIN_EVENT_TABLE( Frame, wxFrame )
///EVT_MENU    (wxID_EXIT, Frame::OnExit)
///EVT_MENU    (DO_TEST,   Frame::DoTest)
///EVT_BUTTON  (BUTTON1,   Frame::OnButton1)
EVT_SIZE( Frame::OnSize )
EVT_ICONIZE( Frame::OnIconize )
END_EVENT_TABLE()
