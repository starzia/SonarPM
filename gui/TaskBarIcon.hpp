#pragma once
#include <wx/wx.h>
#include <wx/taskbar.h>
#include <wx/icon.h>
// template taken from http://my.safaribooksonline.com/0131473816/ch12lev1sec6

//=========================== FORWARD DECLARATIONS ===========================
class TaskBarIcon: public wxTaskBarIcon{
public:
  // f is the frame to which this icon is linked.
  TaskBarIcon( wxFrame* f );

  void OnLeftButtonDClick(wxTaskBarIconEvent&);
  void OnMenuRestore(wxCommandEvent&);
  void OnMenuExit(wxCommandEvent&);
  void OnMenuPause(wxCommandEvent&);
  void OnMenuSetNewIcon(wxCommandEvent&);

  /** this function is called when the user right-clicks the icon */
  virtual wxMenu *CreatePopupMenu();
  DECLARE_EVENT_TABLE()

private:
  void restore();
  wxFrame* theFrame;
};

enum {
  PU_RESTORE = 10001,
  PU_EXIT,
  PU_PAUSE
};

