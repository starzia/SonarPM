#pragma once
#include "Config.hpp"
#include <sstream>

class Logger{
public:
  Logger();
  void setConfig( Config* conf );
  /** appends message to log
   * @return true on success */
  template <class msg>
   bool log( msg message );
  long get_log_start_time( );
  /** sends log back to us using FTP */
  bool phone_home();

  /** tests the frequency response and records it in the log file */
  bool log_freq_response( AudioDev & audio );
  /** asks the user to describe their computer model and logs this */
  bool log_model();

  // phone home constants
  static const long PHONEHOME_INTERVAL = (3600); // in seconds
  static const char* FTP_SERVER;
  static const char* FTP_USER;
  static const char* FTP_PASSWD;
private:
  /** this is a prototype logging function for use with either a sstream or
   *  a file */
  template <class msg, class ostream>
   bool log( msg message, ostream& output );
  /** sets filename data member based on log_id stored in conf */
  void setFilename();

  Config* conf; // conf is needed for phonehome flag and log_id
  std::string filename;
  /** queue up messages here when we are not ready to write to the logfile */
  std::ostringstream buffer;
  unsigned long lastLogTime; // time of the last log entry, used for calulating diff
};