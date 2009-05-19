/* Stephen Tarzia
 * Northwestern University, EECC Dept.
 * Nov 25 2008
 *
 * Under Fedora Linux, package requirements are portaudio-devel
 */
#include "audio.hpp"

/** stores and elicits the program configuration */
class Config{
public:
  /** empty constructor */
  Config();
  /** load() first tries to read the configuration from the passed 
      filename.  If unsuccessful, then call the calibration functions and
      write a new config file to use next time.
  @return true iff a new configuration was created (config file didn't exist)*/
  bool load( AudioDev & audio, std::string filename );
  bool write_config_file();
  /** this would be called after we've already phoned home */
  void disable_phone_home();
  /** tests the frequency response and records it in the log file */
  void log_freq_response( AudioDev & audio );

  frequency ping_freq;
  float threshold;
  bool allow_phone_home;
  unsigned int rec_dev;
  unsigned int play_dev;

private:
  std::string filename;
  /** CALIBRATION FUNCTIONS */
  /** Prompt the user to find the best ping frequency.
      Generally, we want to choose a frequency that is both low enough to
      register on the (probably cheap) audio equipment but high enough to be
      inaudible. */
  void choose_ping_freq( AudioDev & audio );
  /** Choose the variance threshold for presence detection by prompting
      the user. */
  void choose_ping_threshold( AudioDev & audio, frequency freq );
  /** Ask the user whether or not to report anonymous statistics */
  void choose_phone_home();
  /** Plays a series of loud tones to help users adjust their speaker volume */
  void warn_audio_level( AudioDev & audio );
};

class SysInterface{
 public:
  static bool sleep_monitor();
  static duration_t idle_seconds();
  /** blocks the process for the specified time */
  static void sleep( duration_t duration );
  /** returns shortly after HID activity */
  static void wait_until_active();
  /** returns after there has been no HID for some time */ 
  static void wait_until_idle();
  /** returns glibc style time, ie epoch seconds */
  static long current_time();
  /** appends message to log */
  template <class msg>
  static bool log( msg message );
  /** returns the name of configuration directory, creating if nonexistant */
  static std::string config_dir();
  /** register fcn to clean up on terminattion */
  static void register_term_handler();
  /** sends log back to us using FTP */
  static bool phone_home();
};

long get_log_start_time( );
/** This is the main program loop.  It checks for a user and powers down
    the display if it's reasonably confident that no one is there */
void power_management( AudioDev & audio, Config & conf );
int main( int argc, char **argv );
