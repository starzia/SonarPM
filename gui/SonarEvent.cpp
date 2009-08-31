#include "SonarEvent.hpp"

DEFINE_EVENT_TYPE( SONAR_EVENT )

SonarEvent::SonarEvent(enum SonarEventType t, wxEventType commandType, int id )
:  wxCommandEvent(commandType, id), type(t), a(0), b(0), c(0) {}
 
SonarEvent::SonarEvent( const SonarEvent &event )
    :  wxCommandEvent(event) { 
  this->a = event.a;
  this->b = event.b;
  this->c = event.c;
  this->type = event.getType();
  this->msg = std::string( event.msg );
}
 
wxEvent* SonarEvent::Clone() const {
  return new SonarEvent(*this);
}
 
void SonarEvent::setVal( float a, float b, float c ) {
  this->a = a;
  this->b = b; 
  this->c = c;
}

void SonarEvent::setMsg( std::string m ){
  this->msg = m;
}

enum SonarEventType SonarEvent::getType() const{
  return type;
}

