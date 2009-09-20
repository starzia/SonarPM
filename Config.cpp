#include "Config.hpp"
#include "SysInterface.hpp"
#include "SimpleIni.h" // for config files
#include <iostream>
#include <sstream>
#ifdef PLATFORM_WINDOWS
// for UUID, use rpcrt4.lib
#include <windows.h>
#include <rpcdce.h>
#endif

using namespace std;

const char* Config::CONFIG_FILENAME = "sonarPM.cfg";
const frequency Config::DEFAULT_PING_FREQ = 22000;

Config::Config(){
  // these default values will be overwritten if a config file is loaded
  this->ping_freq = this->DEFAULT_PING_FREQ;
  this->generate_GUID();
  this->log_id = 0;
  this->start_time = SysInterface::current_time();
}

/** call calibration functions to create a new configuration */
bool Config::load( string filename ){
  this->filename = filename;
  // try to load from a data file
  CSimpleIniA ini(false,false,false);
  SI_Error rc = ini.LoadFile( filename.c_str() );
  if (rc < 0){
    // if file open unsuccessful, then run calibration
    cerr<< "Unable to open config file "<<filename<<endl;
    return false;
  }else{
    // if file open successful, then get the config key values from it.
    try{
      istringstream ss;
      ss.clear();
      ss.str( ini.GetValue("general","phone_home" ) );
      ss >> this->allow_phone_home;
      ss.clear();
      ss.str( ini.GetValue("general","recording_device" ) );
      ss >> this->rec_dev;
      ss.clear();
      ss.str( ini.GetValue("general","playback_device" ) );
      ss >> this->play_dev;
      ss.clear();
      ss.str( ini.GetValue("calibration","frequency" ) );
      ss >> this->ping_freq;
      ss.clear();
      ss.str( ini.GetValue("state","GUID" ) );
      ss >> this->GUID;
      ss.clear();
      ss.str( ini.GetValue("state","log_id" ) );
      ss >> this->log_id;
      ss.clear();
      ss.str( ini.GetValue("state","start_time" ) );
      ss >> this->start_time;
      cerr<< "Loaded config file "<<filename<<endl;
    }catch( const exception& e ){
      cerr <<"Error loading data from Config file "<<filename<<endl
	   <<"Please check the file for errors and correct or delete it"<<endl;
      exit(-1);
    }
    return true;
  }
}

bool Config::load(){
  return this->load( SysInterface::config_dir() + Config::CONFIG_FILENAME );
}

bool Config::write_config_file(){
  CSimpleIniA ini(false,false,false);
  // set the key values
  ostringstream ss;
  string str;
  ss.str(""); // reset stringstream
  ss << this->allow_phone_home;
  ini.SetValue("general","phone_home", ss.str().c_str());
  ss.str("");
  ss << this->rec_dev;
  ini.SetValue("general","recording_device", ss.str().c_str());
  ss.str("");
  ss << this->play_dev;
  ini.SetValue("general","playback_device", ss.str().c_str());
  ss.str("");
  ss << this->ping_freq;
  ini.SetValue("calibration","frequency", ss.str().c_str());
  ss.str("");
  ss << this->GUID;
  ini.SetValue("state","GUID", ss.str().c_str());
  ss.str("");
  ss << this->log_id;
  ini.SetValue("state","log_id", ss.str().c_str());
  ss.str("");
  ss << this->start_time;
  ini.SetValue("state","start_time", ss.str().c_str());

  // write to file
  SI_Error rc = ini.SaveFile( this->filename.c_str() );
  if (rc < 0){
    cerr<< "Error saving config file "<<this->filename<<endl;
    return false;
  }else{
    cerr<< "Saved config file "<<this->filename<<endl;
  }
  return true;
}

void Config::disable_phone_home(){
  this->allow_phone_home = false;
  this->write_config_file();
}

void Config::generate_GUID(){
  ostringstream guid;
#ifdef PLATFORM_WINDOWS
  unsigned char* sTemp;
  UUID* pUUID = new UUID();
  if( pUUID != NULL ){
    HRESULT hr;
    hr = UuidCreate(pUUID);
    if (hr == RPC_S_OK){
      hr = UuidToString(pUUID, &sTemp);
      if (hr == RPC_S_OK){
	guid << sTemp;
	RpcStringFree(&sTemp);
      }
    }
    delete pUUID;
  }
#else
  // TODO: improve GUID generation in Linux
  guid << "linux" << SysInterface::current_time();
#endif
  this->GUID = guid.str();
}
