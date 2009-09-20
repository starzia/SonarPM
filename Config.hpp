#pragma once
#include "audio.hpp"

/** stores and elicits the program configuration */
class Config{
public:
  /** empty constructor */
  Config();
  /** load() first tries to read the configuration from the passed
      filename.  If unsuccessful, then call new_config() and
      write a new config file to use next time.
  @return true if the configuration file was found and loaded */
  bool load( std::string filename );
  /** load using default filename */
  bool load();
  /** run the setup and calibration console prompts */
  void new_config( AudioDev & audio );
  /** save configuration to a file in the user's home directory */
  bool write_config_file();
  /** this would be called after we've already phoned home */
  void disable_phone_home();
  /** generate a GUID to uniquely identidy this installation */
  void generate_GUID();

  /* configuration data members */
  frequency ping_freq;
  static const frequency DEFAULT_PING_FREQ;
  static const char* CONFIG_FILENAME;
  bool allow_phone_home;
  int rec_dev;
  int play_dev;
  std::string GUID;
  unsigned int log_id; // this is the number of logs phoned home so far
  long start_time; // time when sonar was first run

  /** Choose the delta threshold for presence detection by recording */
  void choose_ping_threshold( AudioDev & audio, frequency freq );
private:
  std::string filename;
  /* TUI CALIBRATION FUNCTIONS */
  /** Prompt the user to find the best ping frequency.
      Generally, we want to choose a frequency that is both low enough to
      register on the (probably cheap) audio equipment but high enough to be
      inaudible. */
  void choose_ping_freq( AudioDev & audio );
  /** Ask the user whether or not to report anonymous statistics */
  void choose_phone_home();
  /** Plays a series of loud tones to help users adjust their speaker volume */
  void warn_audio_level( AudioDev & audio );
};
