#include "Logger.hpp"
#include "SysInterface.hpp"
#include "dsp.hpp" // for freq_response
#include <iostream>
#include <fstream> // for logfile writing
#include <sstream>
#include <limits> // for numeric_limit

#if defined PLATFORM_WINDOWS
  #undef NOMINMAX
  #define NOMINMAX true
  #include <windows.h>
  #include <wininet.h> //  for ftp
#else
  #include <stdlib.h> //system, getenv, etc.
#endif

using namespace std;

// CONSTANTS
const char* Logger::FTP_SERVER = "belmont.eecs.northwestern.edu";
const char* Logger::FTP_USER = "sonar";
const char* Logger::FTP_PASSWD = "ppiinngg";


Logger::Logger(): conf(NULL), lastLogTime(0), numFailedAttempts(0) {
  this->filename = ""; // update this on setConfig()
}

void Logger::setFilename(){
  ostringstream filenameStream;
  filenameStream << SysInterface::config_dir() << this->getFilenameNoPath();
  this->filename = filenameStream.str();
}

string Logger::getFilenameNoPath(){
  ostringstream filenameStream;
  filenameStream << this->conf->GUID << '.' << this->conf->log_id << ".log";
  return filenameStream.str();
}

void Logger::setConfig( Config* c ){
  this->conf = c;
  this->setFilename();
  // flush log buffer
  ofstream logstream( this->filename.c_str(), ios::app ); // append mode
  logstream << this->buffer.str();
  logstream.close();
  this->buffer.str(""); // clear buffer ostringstream
}

template <class msg>
bool Logger::log( msg message ){
  bool ret=true; // return value
  if( this->filename.length() > 0 && this->conf ){
    // if !allow_phone_home do not log anything.  This prevents energy
    // inefficiency due to hard disk access
    if( this->conf->allow_phone_home ){
      // log to file
      ofstream logstream( this->filename.c_str(), ios::app ); // append mode
      ret = log( message, logstream );
      logstream.close();
    }
  }else{
    // log to buffer
    ret = log( message, this->buffer );
  }
  // phone home?
  if( this->conf && this->conf->allow_phone_home ){
    // test to see whether age of log is greater than phone home interval plus
    // FAILURE_BACKOFF time since the last failed upload attempt, if any.
    if( SysInterface::current_time() - this->get_log_start_time() >
              Logger::PHONEHOME_INTERVAL
              + this->numFailedAttempts * Logger::FAILURE_BACKOFF){
      this->phone_home();
    }
  }
  return ret;
}

template <class msg, class ostream>
bool Logger::log( msg message, ostream& logstream ){
  unsigned long logTime, currentTime = SysInterface::current_time();
  if( this->lastLogTime == 0 ){
    // if this is the first log message we're writing, use absolute time
    logTime = currentTime;
  }else{
    // otherwise use time differential
    logTime = currentTime - this->lastLogTime;
  }
  this->lastLogTime = currentTime;
  logstream << logTime << " " << message << endl;
  return true; // TODO: test success
}

long Logger::get_log_start_time( ){
  ifstream logfile( this->filename.c_str() );
  // default to very large value so that if error, phonehome does not occur.
  long time = numeric_limits<long>::max();
  if( logfile.good() ){
    logfile >> time;
  }
  return time;
}

// explicitly instantiate log function for type Statistics (not sure why needed)
template bool Logger::log(Statistics s);
template bool Logger::log(char const* s);

/** in this function zipping of the logs is handled by an external gzip binary,
 * both in linux and windows */
bool Logger::phone_home(){
  cerr << "Sending log file to Northwestern University server."<<endl;
  bool success;
#if defined PLATFORM_WINDOWS
  // gzip the log
  STARTUPINFO info;
  memset( &info, 0, sizeof(info) );
  info.cb = sizeof(info);
  // set startupinfo such that gzip will be hidden.
  info.dwFlags = STARTF_USESHOWWINDOW;
  info.wShowWindow = SW_HIDE; 
  PROCESS_INFORMATION pinfo;
  char cmd[128];
  strcpy( cmd, ("gzip.exe -9f \"" + this->filename + "\"").c_str() );
  success = CreateProcess( NULL, cmd, NULL, NULL, false,
                           NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &pinfo );
  if( success ){
    // wait for gzip process to exit
    WaitForSingleObject( pinfo.hProcess, INFINITE );

    // ftp upload
    HINTERNET hnet = InternetOpen( "sonar", INTERNET_OPEN_TYPE_PRECONFIG,
                                   NULL,NULL,NULL);
    hnet = InternetConnect( hnet, FTP_SERVER,
                            INTERNET_DEFAULT_FTP_PORT, FTP_USER, FTP_PASSWD,
                            INTERNET_SERVICE_FTP, NULL, NULL );
    success = FtpPutFile( hnet, (this->filename + ".gz").c_str(),
                          (this->getFilenameNoPath() + ".gz").c_str(),
                          FTP_TRANSFER_TYPE_BINARY, NULL );
    if( !success ){
      // gunzip log so that we can continue logging to it
      strcpy( cmd, ("gzip.exe -df \"" + this->filename + ".gz\"").c_str() );
      CreateProcess( NULL, cmd, NULL, NULL, false,
                     NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &pinfo );
      WaitForSingleObject( pinfo.hProcess, INFINITE ); // wait for gunzip

    }
    InternetCloseHandle( hnet );
  }
#else
  // gzip it
  success = ( system( ("gzip -9f " + this->filename).c_str() ) == EXIT_SUCCESS );
  if( success ){
    // ftp upload
    string command = "curl -T " + this->filename + ".gz ftp://"+FTP_USER+
            ':'+FTP_PASSWD+'@'+FTP_SERVER+'/'+this->getFilenameNoPath()+".gz";
    success = ( system( command.c_str() ) == EXIT_SUCCESS );
    if( !success ){
       // gunzip it to continue logging
      system( ("gzip -df " + this->filename + ".gz").c_str() );
    }
  }
#endif
  // on success
  if( success ){
    conf->log_id++; // increment log_id
    conf->write_config_file(); // save new log_id
    this->setFilename(); // update filename to reflect new log_id
    this->lastLogTime = 0; // force the absolute time to be logged next time
                           // this is important since it will be first in new file
    this->numFailedAttempts = 0; // reset failure counter
    this->log( "phonehome" );
  }else{
    this->numFailedAttempts++; // count this failure
  }
  return success;
}


bool Logger::log_freq_response( AudioDev & audio ){
  pair<freq_response,freq_response> response_pair =
    test_ultrasonic_response( audio );
  freq_response noise = response_pair.first;
  freq_response silence = response_pair.second;

  ostringstream log_msg;
  log_msg << "sample_rate " << SAMPLE_RATE;
  this->log( log_msg.str() );

  log_msg.str("");  //reset stringstream
  log_msg << "response ";
  unsigned int i;
  for( i=0; i<noise.size(); i++ ){
    log_msg << noise[i].first << ':' << noise[i].second 
            << ',' << silence[i].second << ' ';
  }
  return this->log( log_msg.str() );
}

bool Logger::log_model(){
  cout << endl << "Please type a one-line description of your computer model." << endl
       << "Be as precise as reasonably possible."<<endl
       << "For example: 'Dell Inspiron 8600 laptop'" <<endl<<endl
       << "your description: ";
  string description;
  cin.ignore();
  getline( cin, description );
  return this->log( "model " + description );
}

bool Logger::log_basic( string msg ){
  Logger logger;
  Config conf;
  if( conf.load() ){
    logger.setConfig( &conf );
    logger.log( msg );
    return true;
  }else{
    cerr << "logging <<" << msg << ">> failed!" << endl;
    return false;
  }
}
