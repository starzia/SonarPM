#include "Frame.hpp"
#include "SonarThread.hpp"
#include <wx/sizer.h>
#include <sstream>
#include <wx/event.h>
#include "PlotEvent.hpp"
#include "ConfigFrame.hpp"
#include "CloseConfirmFrame.hpp"
#include <wx/textdlg.h>
#include "PlotEvent.hpp"
using namespace std;


BEGIN_EVENT_TABLE( Frame, wxFrame )
///EVT_MENU    (wxID_EXIT, Frame::OnExit)
///EVT_MENU    (DO_TEST,   Frame::DoTest)
EVT_SIZE( Frame::onSize )
EVT_ICONIZE( Frame::onIconize )
EVT_PLOT( wxID_ANY, Frame::onPlotEvent )
EVT_BUTTON( BUTTON_PAUSE, Frame::onPause )
EVT_BUTTON( BUTTON_CONFIG, Frame::onConfig )
EVT_CHOICE( CHOICE_MODE, Frame::onModeSwitch )
EVT_CLOSE( Frame::onClose )
END_EVENT_TABLE()

Frame::Frame( const wxString & title, int width, int height ) : 
  wxFrame( (wxFrame*)NULL,-1,title,wxDefaultPosition,wxSize(width,height),
	   wxFRAME_NO_TASKBAR | wxSYSTEM_MENU | wxCAPTION 
	   | wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX //| wxRESIZE_BORDER
	   | wxFULL_REPAINT_ON_RESIZE ), sThread(NULL)
{
  // init lock
  this->threadLock = new wxMutex();
  
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

  // add taskbar icon
  this->tbIcon = new TaskBarIcon( this );

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
  this->GetEventHandler()->AddPendingEvent( evt );
}

Frame::~Frame(){
  // Under windows, if we don't do the following, process will never exit!
  delete this->tbIcon;
}

void Frame::nullifyThread(){
  this->threadLock->Lock();
  this->sThread = NULL;
  this->threadLock->Unlock();
}

void Frame::startSonar( ){
  if( this->sThread ) return; // cancel if already started

  // if configuration file does not exist, then prompt for cofiguration
  Config conf;
  bool firstTime = !conf.load( SysInterface::config_dir()+CONFIG_FILENAME );
  while( !conf.load( SysInterface::config_dir()+CONFIG_FILENAME ) ){
    ConfigFrame* conf = new ConfigFrame( this,_T("First-time configuration") );
    int choice = conf->ShowModal();
  }

  if( firstTime ){
    this->firstTime();
  }

  // The following lock prevents multiple new threads from starting.
  {
    this->threadLock->Lock();

    if( this->sThread ){
      this->threadLock->Unlock();
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
    this->threadLock->Unlock();
  }
}

void Frame::stopSonar(){
  // stop sonar processing thread
  this->threadLock->Lock();
  if( this->sThread ){ // stop only if started
    if( this->sThread->Delete() == wxTHREAD_NO_ERROR ){
      this->buttonPause->SetLabel( _T( "continue" ) );
      this->SetStatusText(_T("Sonar is stopped"));
    }
  }
  this->threadLock->Unlock();
}

void Frame::threadWait(){
  bool ready = false;
  while( !ready ){
    SysInterface::sleep( 0.5 );
    wxSafeYield(); // let the GUI update
    this->threadLock->Lock();
    if( !this->sThread ) ready = true;
    this->threadLock->Unlock();
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
  SysInterface::log( "model " + string(modelName.mb_str()) );

  // log frequency response
  ret = wxMessageBox(
    _T("Next, you may hear some high frequency noise while the system is calibrated.  "
       "This should take less than thirty seconds.  "
       "Afterwards, sonar power management will be active.  "
       "At first, the sonar system may be too aggressive and "
         "it may shut off your display when you are actually present.  "
       "If this happens, simply wake up the display by moving the mouse.  "),
    _T("Calibration"), wxOK, this );
  this->threadLock->Lock();
  if( !this->sThread ){
    this->sThread = new SonarThread( this, MODE_FREQ_RESPONSE );
    if( this->sThread->Create() == wxTHREAD_NO_ERROR ){
      this->sThread->Run();
    }
  }
  this->threadLock->Unlock();

  // wait for freq_response thread to complete.
  this->SetStatusText(_T("Measuring frequency response... Please wait."));
  this->threadWait();
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
  // is this necessary?
  this->Show(!event.Iconized());
}

void Frame::onSize( wxSizeEvent& event ){
  //this->Refresh();
  //this->Update();
}

void Frame::onPlotEvent(PlotEvent& event){
  if( event.getType() == PLOT_EVENT_POINT ){
    this->sonarHistory->addPoint( event.a, event.b, event.c );
    
    wxString s;
    s.Printf( wxT("Last reading: %f, avg: %f"), event.a, event.b );
    this->SetStatusText(s);
    
  }else if( event.getType() == PLOT_EVENT_THRESHOLD ){
    wxString s;
    s.Printf( wxT("Threshold updated to: %f"), event.a );
    this->SetStatusText(s);
  }else if( event.getType() == PLOT_EVENT_GAP ){
    this->sonarHistory->addPoint( 0.0/0.0, 0.0/0.0, 0.0/0.0 ); //NaN
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

void Frame::onModeSwitch( wxCommandEvent& event ){
  // if sonar is running, stop it so that new operating mode can be used.
  this->stopSonar();
}
