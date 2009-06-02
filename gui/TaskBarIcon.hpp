#include <wx/wx.h>
#include <wx/taskbar.h>
#include <wx/icon.h>
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

