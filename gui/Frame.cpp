#include "Frame.hpp"
#include "SonarThread.hpp"
#include <wx/sizer.h>
#include <sstream>
#include <wx/event.h>
#include "ConfigFrame.hpp"
#include "CloseConfirmFrame.hpp"
#include <wx/textdlg.h>
#include "../SysInterface.hpp"
#include "App.hpp" // for VERSION
#include <sstream>
#ifndef PLATFORM_WINDOWS
  #include "bat_32.xpm" // the icon file
#endif
using namespace std;


BEGIN_EVENT_TABLE( Frame, wxFrame )
EVT_SIZE( Frame::onSize )
EVT_ICONIZE( Frame::onIconize )
EVT_SONAR( wxID_ANY, Frame::onSonarEvent )
EVT_BUTTON( BUTTON_PAUSE, Frame::onPause )
EVT_BUTTON( BUTTON_CONFIG, Frame::onConfig )
EVT_BUTTON( BUTTON_ABOUT, Frame::onAbout )
EVT_CHOICE( CHOICE_MODE, Frame::onModeSwitch )
EVT_CLOSE( Frame::onClose )
END_EVENT_TABLE()

Frame::Frame( const wxString & title, int width, int height ) : 
  wxFrame( (wxFrame*)NULL,-1,title,wxDefaultPosition,wxSize(width,height),
	   wxSYSTEM_MENU | wxCAPTION 
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX //| wxRESIZE_BORDER
	   | wxFULL_REPAINT_ON_RESIZE ), 
  sThread(NULL), threadLock(wxMUTEX_DEFAULT)
{ 
  // add controls
  this->CreateStatusBar();
  this->SetStatusText(_T("Sonar is stopped"));
  this->panel = new wxPanel( this, wxID_ANY, wxDefaultPosition,
                             this->GetClientSize());
  this->buttonPause = new wxButton( panel, BUTTON_PAUSE, _T("start"),
				    wxDefaultPosition, wxDefaultSize );
  const wxString choices[2] = {_T("power management"), _T("continuous test")};
  this->choiceMode = new wxChoice( panel, CHOICE_MODE, wxDefaultPosition,
				   wxDefaultSize, 2, choices );
  this->choiceMode->SetSelection( 0 ); // power management by default
  this->buttonConfig = new wxButton( panel, BUTTON_CONFIG, _T("configure"),
				     wxDefaultPosition, wxDefaultSize );
  this->buttonAbout = new wxButton( panel, BUTTON_ABOUT, _T("about"),
				     wxDefaultPosition, wxDefaultSize );

  // add taskbar icon
  this->tbIcon = new TaskBarIcon( this );
  // set window icon
#ifdef PLATFORM_WINDOWS
  this->SetIcon( wxIcon( _T("bat_32.ico"), wxBITMAP_TYPE_ICO ) );
#else
  this->SetIcon( bat_32_xpm );
#endif

  // add sonar Plot
  this->sonarHistory = new PlotPane( panel, wxDefaultPosition,
				     panel->GetClientSize() );
  this->sonarHistory->setHistoryLength( PlotPane::HISTORY_WINDOW );
  
  // create sizers for layout
  wxBoxSizer* sizer3 = new wxBoxSizer( wxVERTICAL );
  sizer3->Add( new wxStaticText( panel, wxID_ANY, _T("operating mode:")));
  sizer3->Add( this->choiceMode, 1, wxALL | wxEXPAND, 5 );
  sizer3->Add( this->buttonPause, 1, wxALL | wxEXPAND, 5 );
  sizer3->Add( this->buttonConfig, 1, wxALL | wxEXPAND, 5 );
  sizer3->Add( this->buttonAbout, 1, wxALL | wxEXPAND, 5 );

  wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );
  sizer2->Add( this->sonarHistory,
	      1, /* vertically stretchable */
	      wxEXPAND ); /* horizontally stretchable */
  sizer2->Add( sizer3, 0, wxALL, 10 ); // 10pt border
  panel->SetSizer(sizer2);
  sizer2->SetSizeHints(panel); // set sze hints to honour min size

  wxBoxSizer* sizer1 = new wxBoxSizer( wxHORIZONTAL );
  sizer1->Add( this->panel );
  this->SetSizer(sizer1);
  sizer1->SetSizeHints(this);

  // start sonar by queuing up an event
  wxCommandEvent evt = wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED,
                                       BUTTON_PAUSE );
  this->GetEventHandler()->ProcessEvent( evt );
  /* NOTE: above, it seems that we should be using EvtHandler::AddPendingEvent
   * rather than ProcessEvent, but the former was causing crash under Windows */
}

Frame::~Frame(){
  // Under windows, if we don't do the following, process will never exit!
  delete this->tbIcon;
}

void Frame::startSonar( ){
  if( this->sThread ) return; // cancel if already started

  // if configuration file does not exist, then prompt for cofiguration
  Config conf;
  bool firstTime = !conf.load();
  while( !conf.load() ){
    this->restore(); // bring window to front
    ConfigFrame* conf = new ConfigFrame( this,_T("First-time configuration") );
    int choice;
    choice = conf->ShowModal();
  }

  if( firstTime ){
    this->firstTime();
  }

  // The following lock prevents multiple new threads from starting.
  {
    LOCK( this->threadLock );

    if( this->sThread ){
      return; // cancel if already started
    }

    // start sonar processing thread
    this->SetStatusText(_T("Sonar is started"));
    if( this->choiceMode->GetCurrentSelection() == 0 ){
      this->sThread = new SonarThread( this, MODE_POWER_MANAGEMENT );
    }else{
      this->sThread = new SonarThread( this, MODE_POLLING );
    }
   
    if( this->sThread->Create( ) == wxTHREAD_NO_ERROR ){
      this->sThread->Run( );
      this->buttonPause->SetLabel( _T( "pause" ) );
    }
  }
}

/** note that the gui's response to the thread stopping doesn't happen here but
 * after the thread sends back a SonarEvent(SONAR_DONE) event */
void Frame::stopSonar(){  
  // stop sonar processing thread
  LOCK( this->threadLock );
  if( this->sThread ){ // stop only if started
    this->buttonPause->Disable(); // gray-out pause control
    this->sThread->Delete();
    //if( this->sThread->Delete() == wxTHREAD_NO_ERROR ){
  }
}

/** this function seems to be buggy */
void Frame::threadWait(){
  bool ready = false;
  while( !ready ){
    SysInterface::sleep( 0.2 );
    wxSafeYield(); // let the GUI update
    LOCK( this->threadLock );
    if( !this->sThread ) ready = true;
  }
}

void Frame::firstTime(){
  // Give some instructions/warning
  int ret = wxMessageBox(
    _T("In order for this software to function correctly, you must set your "
       "audio volume level to a normal listening level and unplug any "
       "headphones so that the speakers are used." ),
    _T("Instructions"), wxOK | wxCANCEL, this );
  if( ret == wxCANCEL ) this->Close(true); // quit if disagree

  // prompt for model name
  wxString modelName = wxGetTextFromUser(
        _T("Please enter the manufacturer and model name of your computer."),
        _T("Computer description"), _T("Generic"), this );
  Logger::log_basic( "model " + string(modelName.mb_str()) );

  // log program version 
  Logger::log_basic( "version " + string( VERSION ) );

  // log frequency response
  ret = wxMessageBox(
    _T("Next, you may hear some high frequency noise while the system is calibrated.  "
       "This should take less than thirty seconds.  "
       "Afterwards, sonar power management will be active.  "
       "At first, the sonar system may be too aggressive and "
         "it may shut off your display when you are actually present.  "
       "If this happens, simply wake up the display by moving the mouse.  "),
    _T("Calibration"), wxOK, this );

  // log program version
  ostringstream log_msg;
  log_msg << "version " << VERSION;
  this->logger.log( log_msg.str() );

  {
    LOCK( this->threadLock );
    if( !this->sThread ){
      this->sThread = new SonarThread( this, MODE_FREQ_RESPONSE );
      if( this->sThread->Create() == wxTHREAD_NO_ERROR ){
        this->sThread->Run();
        this->SetStatusText(_T("Measuring frequency response... Please wait."));
        this->buttonPause->SetLabel( _T( "pause" ) );
      }
    }
  }
}

void Frame::onClose( wxCloseEvent& event ){
  // system-generated close events can not be intercepted (vetoed)
  if( event.CanVeto() ){
    event.Veto();
    // prompt user to either close or minimize
    CloseConfirmFrame* confirm = new CloseConfirmFrame(this);
    // Modal dialog has a return value, in this case, returns 1 if 'exit'
    int choice = confirm->ShowModal();
    if( choice == BUTTON_EXIT ){
      this->Close(true); // recur, disallowing veto
    }else if( choice == BUTTON_MINIMIZE ){
      // minimize
      this->Show(false);
    } // else cancel
  }else{
    /*======= CLEANUP CODE =======*/
    // clean up taskbar
    this->tbIcon->RemoveIcon();
    // stop sonar thread
    this->stopSonar();
    this->Destroy(); // close window
  }
}

void Frame::onIconize( wxIconizeEvent& event ){
  // remove from taskbar when minimized
  this->Show(!event.Iconized());
}

void Frame::onSize( wxSizeEvent& event ){
  //this->Refresh();
  //this->Update();
}

void Frame::onSonarEvent(SonarEvent& event){
  if( event.getType() == PLOT_EVENT_POINT ){
    this->sonarHistory->addPoint( event.a, event.b, event.c );
  }else if( event.getType() == PLOT_EVENT_THRESHOLD ){
    wxString s;
    s.Printf( wxT("Threshold updated to: %f"), event.a );
    this->SetStatusText(s);
  }else if( event.getType() == PLOT_EVENT_GAP ){
    this->sonarHistory->addPoint( NAN, NAN, NAN ); //NaN
  }else if( event.getType() == SONAR_DONE ){
    // note that previously we were nullifying in the thread's OnExit.
    // However, this was causing locking problems in Windows.
    this->sThread = NULL;

    // update gui controls
    this->buttonPause->SetLabel( _T( "start" ) );
    this->SetStatusText(_T("Sonar is stopped.  Click start to begin."));

    this->Enable(); // un-gray-out the controls in this window
    this->buttonPause->Enable(); // un-gray-out pause control
  }else if( event.getType() == STATUS_MSG ){
    wxString str = wxString::FromAscii( event.msg.c_str() );
    this->SetStatusText( str );
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

void Frame::onConfig(wxCommandEvent& event){
  // pop up config window
  ConfigFrame* conf = new ConfigFrame( this,_T("Configuration") );
  int choice = conf->ShowModal();
  
  // if user saved new configuration & sonar running, then restart sonar
  if( choice == BUTTON_SAVE && this->sThread ){
    this->stopSonar();
    this->startSonar();
  }
}

void Frame::onAbout(wxCommandEvent& event){
  // pop up about window
  wxMessageBox(
    _T("This software was written by Stephen Tarzia in 2009.  "
       "Design guidance came from Peter Dinda, Robert Dick and Gokhan Memik.\n\n"
       "To download the latest version or source code, submit bug reports, or to read technical details see:\n"
       "http://stevetarzia.com/sonar"),
    _T("About"), wxOK, this );
}

void Frame::onModeSwitch( wxCommandEvent& event ){
  // if sonar is running, stop it so that new operating mode can be used.
  this->stopSonar();
}

bool Frame::isSonarRunning(){
  return (this->sThread != NULL);
}

void Frame::restore(){
  this->Iconize(false);
  this->Show(true);
  this->Raise(); // make it the top-most window
  this->SetFocus();
}

#ifdef PLATFORM_WINDOWS
/* this code checks Windows messages for suspend/resume messages */
WXLRESULT Frame::MSWWindowProc( WXUINT message,
                                WXWPARAM wParam, WXLPARAM lParam ){
  if( message == WM_POWERBROADCAST ){
    if( wParam == PBT_APMRESUMEAUTOMATIC ){
      Logger::log_basic( "resume" );
    }else if( wParam == PBT_APMSUSPEND ){
      Logger::log_basic( "suspend" );
    }else if( wParam == PBT_APMPOWERSTATUSCHANGE ){
      Logger::logPowerStatus();
    }
  }
  // pass message on to base class version for standard processing.
  return wxFrame::MSWWindowProc( message, wParam, lParam );
}
#endif
