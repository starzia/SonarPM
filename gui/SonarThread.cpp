#include "SonarThread.hpp"
#include "../sonar.hpp"

SonarThread::SonarThread( Frame* mf ) : 
  wxThread(), mainFrame( mf ){}

void* SonarThread::Entry(){
  while(1){
  }
  return 0;
}
