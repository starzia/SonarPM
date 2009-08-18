#pragma once
#include "Logger.hpp"


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
  /** returns the name of configuration directory, creating if nonexistant */
  static std::string config_dir();
  /** register fcn to clean up on terminattion */
  static void register_term_handler( Logger & logger );
};