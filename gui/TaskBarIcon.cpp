#include "TaskBarIcon.hpp"

#include "kuser.xpm" // the icon file
BEGIN_EVENT_TABLE(TaskBarIcon, wxTaskBarIcon)
EVT_MENU(PU_RESTORE, TaskBarIcon::OnMenuRestore)
EVT_MENU(PU_EXIT,    TaskBarIcon::OnMenuExit)
EVT_MENU(PU_NEW_ICON,TaskBarIcon::OnMenuSetNewIcon)
EVT_TASKBAR_LEFT_DCLICK  (TaskBarIcon::OnLeftButtonDClick)
END_EVENT_TABLE()

TaskBarIcon::TaskBarIcon( wxFrame* f ) : theFrame(f){
  this->SetIcon( wxICON(kuser), _T("Sonar") );
}

void TaskBarIcon::OnMenuRestore(wxCommandEvent& ){
  theFrame->Iconize(false);
  theFrame->Show(true);
}

void TaskBarIcon::OnMenuExit(wxCommandEvent& ){
  theFrame->Close(true);
}

void TaskBarIcon::OnMenuSetNewIcon(wxCommandEvent&){
  /*
  wxIcon icon(smile_xpm);

  if (!SetIcon(icon, wxT("wxTaskBarIcon Sample - a different icon")))
    wxMessageBox(wxT("Could not set new icon."));
  */
}

// Overridables
wxMenu *TaskBarIcon::CreatePopupMenu(){
  wxMenu *menu = new wxMenu;

  menu->Append(PU_RESTORE, wxT("&Sonar status window"));
  ///menu->Append(PU_NEW_ICON,wxT("&Set New Icon"));
  menu->Append(PU_EXIT,    wxT("E&xit"));

  return menu;
}

void TaskBarIcon::OnLeftButtonDClick(wxTaskBarIconEvent&){
  theFrame->Iconize(false);
  theFrame->Show(true);
}
