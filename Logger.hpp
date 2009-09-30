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
  /** static function to log a string.  This is inefficient since it loads the
   * configuration file each time, however it is useful for callback functions
   * which have no reference to the active logger object */
  static bool log_basic( std::string message );

  // phone home constants
  static const int PHONEHOME_INTERVAL = 3600; // in seconds
  /** time to wait after a phonehome upload failure before attempting again */
  static const int FAILURE_BACKOFF = 3600;
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
  /** @return current log filename, excluding path */
  std::string getFilenameNoPath();

  Config* conf; // conf is needed for phonehome flag and log_id
  std::string filename; // filename including path
  /** queue up messages here when we are not ready to write to the logfile */
  std::ostringstream buffer;
  long lastLogTime; // time of the last log entry, used for calulating diff
  int numFailedAttempts; // number of consecutive phonehome failures
};
