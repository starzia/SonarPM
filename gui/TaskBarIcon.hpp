#include <wx/taskbar.h>
#include <wx/icon.h>
#include "kuser.xpm" // the icon file
// taken from http://my.safaribooksonline.com/0131473816/ch12lev1sec6

//=========================== FORWARD DECLARATIONS ===========================
class TaskBarIcon: public wxTaskBarIcon{
public:
  // f is the frame to which this icon is linked.
  TaskBarIcon( wxFrame* f );

  void OnLeftButtonDClick(wxTaskBarIconEvent&);
  void OnMenuRestore(wxCommandEvent&);
  void OnMenuExit(wxCommandEvent&);
  void OnMenuSetNewIcon(wxCommandEvent&);

  virtual wxMenu *CreatePopupMenu();
  DECLARE_EVENT_TABLE()

private:
  wxFrame* theFrame;
};

enum {
  PU_RESTORE = 10001,
  PU_NEW_ICON,
  PU_EXIT,
};

//=========================== IMPLEMENTATIONS ================================
BEGIN_EVENT_TABLE(TaskBarIcon, wxTaskBarIcon)
EVT_MENU(PU_RESTORE, TaskBarIcon::OnMenuRestore)
EVT_MENU(PU_EXIT,    TaskBarIcon::OnMenuExit)
EVT_MENU(PU_NEW_ICON,TaskBarIcon::OnMenuSetNewIcon)
EVT_TASKBAR_LEFT_DCLICK  (TaskBarIcon::OnLeftButtonDClick)
END_EVENT_TABLE()

TaskBarIcon::TaskBarIcon( wxFrame* f ) : theFrame(f){
  this->SetIcon( wxICON(kuser), _T("ToolTip") );
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

  menu->Append(PU_RESTORE, wxT("&Restore TBTest"));
  menu->Append(PU_NEW_ICON,wxT("&Set New Icon"));
  menu->Append(PU_EXIT,    wxT("E&xit"));

  return menu;
}

void TaskBarIcon::OnLeftButtonDClick(wxTaskBarIconEvent&){
  theFrame->Iconize(false);
  theFrame->Show(true);
}

