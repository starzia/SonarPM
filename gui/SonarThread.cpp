#include "SonarThread.hpp"

SonarThread::SonarThread( Frame* mf ) : 
  wxThread(), mainFrame( mf ){}

void* SonarThread::Entry(){
  while(1){
  }
  return 0;
}
