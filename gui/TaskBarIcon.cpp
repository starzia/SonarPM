#include "TaskBarIcon.hpp"
#include "Frame.hpp"

//#include "bat_32.xpm" // the icon file
BEGIN_EVENT_TABLE(TaskBarIcon, wxTaskBarIcon)
EVT_MENU(PU_RESTORE, TaskBarIcon::OnMenuRestore)
EVT_MENU(PU_PAUSE,   TaskBarIcon::OnMenuPause)
EVT_MENU(PU_EXIT,    TaskBarIcon::OnMenuExit)
EVT_TASKBAR_LEFT_DCLICK  (TaskBarIcon::OnLeftButtonDClick)
END_EVENT_TABLE()

TaskBarIcon::TaskBarIcon( wxFrame* f ) : theFrame(f){
//  this->SetIcon( bat_32_xpm, _T("Sonar") );
  this->SetIcon( wxIcon( _T("bat_32.ico"), wxBITMAP_TYPE_ICO ) );
}

void TaskBarIcon::restore(){
  theFrame->Iconize(false);
  theFrame->Show(true);
  theFrame->Raise(); // make it the top-most window
  theFrame->SetFocus();
}

void TaskBarIcon::OnMenuRestore(wxCommandEvent& ){
  this->restore();
}

void TaskBarIcon::OnMenuExit(wxCommandEvent& ){
  theFrame->Close(true);
}

void TaskBarIcon::OnMenuPause(wxCommandEvent&){
  // send a pause-button-clicked event
  wxCommandEvent evt = wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED,
                                       BUTTON_PAUSE );
  theFrame->GetEventHandler()->ProcessEvent( evt );
}

// Overridables
wxMenu *TaskBarIcon::CreatePopupMenu(){
  wxMenu *menu = new wxMenu;

  menu->Append(PU_RESTORE, wxT("&Sonar status window"));
  if( ((Frame*)theFrame)->isSonarRunning() ){
    menu->Append(PU_PAUSE,wxT("&Pause sonar"));
  }else{
    menu->Append(PU_PAUSE,wxT("&Continue sonar"));
  }
  menu->Append(PU_EXIT,    wxT("E&xit"));

  return menu;
}

void TaskBarIcon::OnLeftButtonDClick(wxTaskBarIconEvent&){
  this->restore();
}
