#include "Logger.hpp"
#include "SysInterface.hpp"
#include "dsp.hpp" // for freq_response
#include <iostream>
#include <fstream> // for logfile writing
#include <sstream>
#include <limits> // for numeric_limit
#include <stdlib.h> //system, getenv, etc.

#if defined PLATFORM_WINDOWS
#include <wininet.h> //  for ftp
#else
#include <stdlib.h> //system, getenv, etc.
#endif

using namespace std;

// CONSTANTS
const char* Logger::FTP_SERVER = "belmont.eecs.northwestern.edu";
const char* Logger::FTP_USER = "sonar";
const char* Logger::FTP_PASSWD = "ppiinngg";


Logger::Logger(): conf(NULL){
  this->filename = ""; // update this on setConfig()
}

void Logger::setFilename(){
  ostringstream filenameStream;
  filenameStream << SysInterface::config_dir() << this->conf->GUID << '.'
          << this->conf->log_id << ".log";
  this->filename = filenameStream.str();
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
  bool ret; // return value
  if( this->filename.length() > 0 ){
    // log to file
    ofstream logstream( this->filename.c_str(), ios::app ); // append mode
    ret = log( message, logstream );
    logstream.close();
  }else{
    // log to buffer
    ret = log( message, this->buffer );
  }
  // phone home?
  if( this->conf && this->conf->allow_phone_home ){
    if( SysInterface::current_time() - this->get_log_start_time() >
            this->PHONEHOME_INTERVAL ){
      this->phone_home();
    }
  }
  return ret;
}

template <class msg, class ostream>
bool Logger::log( msg message, ostream& logstream ){
  logstream << SysInterface::current_time() << ": " << message << endl;
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

bool Logger::phone_home(){
  cerr << "Sending log file to Northwestern University server."<<endl;
  bool ret;
#if defined PLATFORM_WINDOWS
  HINTERNET hnet = InternetOpen( "sonar", INTERNET_OPEN_TYPE_PRECONFIG,
				 NULL,NULL,NULL);
  hnet = InternetConnect( hnet, FTP_SERVER,
			  INTERNET_DEFAULT_FTP_PORT, FTP_USER, FTP_PASSWD,
			  INTERNET_SERVICE_FTP, NULL, NULL );
  ret = FtpPutFile( hnet, this->filename, this->filename,
                    FTP_TRANSFER_TYPE_BINARY, NULL );
  InternetCloseHandle( hnet );
#else
  string command = "curl -T " + this->filename +
    " ftp://"+FTP_USER+':'+FTP_PASSWD+'@'+FTP_SERVER+'/'+this->filename;
  system( command.c_str() );
  ret = true;
#endif
  conf->log_id++; // increment log_id
  conf->write_config_file(); // save new log_id
  this->setFilename(); // update filename to reflect new log_id
  this->log( "phonehome" );
  return ret;
}


bool Logger::log_freq_response( AudioDev & audio ){
  freq_response f = test_freq_response( audio );
  ostringstream log_msg;
  log_msg << "sample_rate " << SAMPLE_RATE;
  this->log( log_msg.str() );

  log_msg.str("");  //reset stringstream
  log_msg << "response ";
  unsigned int i;
  for( i=0; i<f.size(); i++ ){
    log_msg << f[i].first << ':' << f[i].second << ' ';
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
