#pragma once
#include <wx/event.h>
#include <vector>

// see http://wiki.wxwidgets.org/Custom_Events#Creating_a_Custom_Event_-_Method_4

// plot event can be either a new point or new threshold value.
enum SonarEventType{
  PLOT_EVENT_POINT,
  PLOT_EVENT_THRESHOLD,
  PLOT_EVENT_GAP,
  SONAR_DONE, // also use this event class to signal sonar thread exits
  STATUS_MSG, // new text for main frame's status bar.
};

// Could have been DECLARE_EVENT_TYPE( SONAR_EVENT, -1 )
// Not needed if you only use the event-type in one .cpp file
//extern expdecl const wxEventType PLOT_EVENT;
DECLARE_EVENT_TYPE( SONAR_EVENT, -1 )

// A custom event that transports a whole wxString.
class SonarEvent: public wxCommandEvent
{
public:
  SonarEvent( enum SonarEventType type,
	         wxEventType commandType = SONAR_EVENT, int id = 0 );
 
  // You *must* copy here the data to be transported
  SonarEvent( const SonarEvent &event );
 
  // Required for sending with wxPostEvent()
  wxEvent* Clone() const;
 
  // accessors
  void setVal( float a, float b, float c );
  void setMsg( std::string m );
  enum SonarEventType getType() const;

  // TODO, make this private and make get() accessors
  float a,b,c; // up to three values may be delivered
  std::string msg;
private:
  enum SonarEventType type;
};
 
typedef void (wxEvtHandler::*SonarEventFunction)(SonarEvent &);
 
// This #define simplifies the one below, and makes the syntax less
// ugly if you want to use Connect() instead of an event table.
#define SonarEventHandler(func)                                         \
  (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)\
  wxStaticCastEvent(SonarEventFunction, &func)
 
// Define the event table entry. Yes, it really *does* end in a comma.
#define EVT_SONAR(id, fn)                                            \
  DECLARE_EVENT_TABLE_ENTRY( SONAR_EVENT, id, wxID_ANY,  \
    (wxObjectEventFunction)(wxEventFunction)                     \
    (wxCommandEventFunction) wxStaticCastEvent(                  \
				 SonarEventFunction, &fn ), (wxObject*) NULL ),
