#include "SysInterface.hpp"

// One of the following should be defined to activate platform-specific code.
// It is best to make this choice in the Makefile, rather than uncommenting here
//#define PLATFORM_LINUX
//#define PLATFORM_WINDOWS
//#define PLATFORM_MAC

#include <iostream>
#include <ctime>
#include <cmath> // floor
#if defined PLATFORM_LINUX
/* The following headers are provided by these packages on a Redhat system:
 * libX11-devel, libXext-devel, libScrnSaver-devel
 * There are also some additional dependencies for libX11-devel */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/scrnsaver.h>
#elif defined PLATFORM_WINDOWS
#define IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS \
  CTL_CODE(FILE_DEVICE_VIDEO, 0x125, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#include <userenv.h> // home path query
#include <shlobj.h> // for CSIDL, SHGetFolderPath

#ifdef MINGW
// microsoft's headers define these, but not mingw's 
#include <ddk/ntddvdeo.h> // backlight control
typedef enum {
  SHGFP_TYPE_CURRENT = 0,
  SHGFP_TYPE_DEFAULT = 1,
} SHGFP_TYPE; // this should be in <shlobj.h> anyway
#endif

#endif

#ifndef PLATFORM_WINDOWS
#include <stdlib.h> //system, getenv, etc.
#include <signal.h>
#endif

using namespace std;

bool SysInterface::sleep_monitor(){
#if defined PLATFORM_LINUX
  system( "xset dpms force standby" );
  return true;
#elif defined PLATFORM_WINDOWS
  // send monitor off message
  PostMessage( HWND_TOPMOST, WM_SYSCOMMAND, SC_MONITORPOWER, 2 );
  //                               -1 for "on", 1 for "low power", 2 for "off".
  //SendMessage( h, WM_SYSCOMMAND, SC_SCREENSAVE, NULL ); // activate scrnsaver
  SysInterface::sleep( 0.5 ); // give system time to process message
  return true;
#elif 0 // the following is Windows LCD brightness control code
  //open LCD device handle
  HANDLE lcd_handle = CreateFile(
	"\\\\.\\LCD",  /*__in      LPCTSTR lpFileName=@"\\.\LCD"*/
	GENERIC_READ | GENERIC_WRITE,/*__in      DWORD dwDesiredAccess*/
	0,             /*__in      DWORD dwShareMode*/
	NULL,          /*__in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes*/
	OPEN_EXISTING, /*__in      DWORD dwCreationDisposition*/
	FILE_ATTRIBUTE_NORMAL, /*__in      DWORD dwFlagsAndAttributes*/
	NULL ); /*__in_opt  HANDLE hTemplateFile*/
  if( lcd_handle == INVALID_HANDLE_VALUE ){
    cerr << "ERROR: could not open LCD handle"<<endl;
    return false;
  }
  // read LCD's brightness range
  char buf[256];
  DWORD num_levels=0;
  if( DeviceIoControl( lcd_handle,                // handle to device
		       IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS,//dwIoControlCode
		       NULL,                      // lpInBuffer
		       0,                         // nInBufferSize
		       buf,                       // output buffer
		       256,                       // size of output buffer
		       &num_levels,               // number of bytes returned
		       NULL )                     // OVERLAPPED structure
      == FALSE ){
    return false;
  }
  if( num_levels==0 ){
    cerr<<"ERROR: hardware does not support LCD brightness control!"<<endl;
  }
  unsigned int i;
  for( i=0; i<num_levels; i++ ){
    cout << "level["<<i<<"]="<<buf[i]<<endl;
  }
  /*
  DeviceIoControl( lcd_handle,                 // handle to device
		  IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS, // dwIoControlCode
		  (LPVOID) lpInBuffer,         // input buffer
		  (DWORD) nInBufferSize,       // size of the input buffer
		  NULL,                        // lpOutBuffer
		  0,                           // nOutBufferSize
		  (LPDWORD) lpBytesReturned,   // number of bytes returned
		  (LPOVERLAPPED) lpOverlapped  // OVERLAPPED structure
		  );
  */
  return true;
#else
  cout << "Monitor sleep unimplemented for this platform."<<endl;
  return false;
#endif
}

duration_t SysInterface::idle_seconds(){
#if defined PLATFORM_LINUX
  Display *dis;
  XScreenSaverInfo *info;
  dis = XOpenDisplay((char *)0);
  Window win = DefaultRootWindow(dis);
  info = XScreenSaverAllocInfo();
  XScreenSaverQueryInfo( dis, win, info );
  duration_t ret = (info->idle)/1000.0;
  XCloseDisplay( dis );
  return ret;
#elif defined PLATFORM_WINDOWS
  LASTINPUTINFO info;
  info.cbSize = sizeof(LASTINPUTINFO); // prepare info struct
  if( !GetLastInputInfo( &info ) )
    cerr<<"ERROR: could not getLastInputInfo"<<endl;
  unsigned int idleTicks = GetTickCount() - info.dwTime; // compute idle time
  return (idleTicks/1000); // returns idle time in seconds
#else
  return 60;
#endif
}

void SysInterface::sleep( duration_t duration ){
  // use portaudio's convenient portable sleep function
  Pa_Sleep( (int)(duration*1000) );
  return;

// the following is alternative code
#ifdef PLATFORM_WINDOWS
#else
  struct timespec sleep_time, remaining_time;
  sleep_time.tv_sec =   (int)(floor(duration)*1000000000);
  sleep_time.tv_nsec = (long)((duration-floor(duration))*1000000000);
  nanosleep( &sleep_time, &remaining_time );
#endif
}

long SysInterface::current_time(){
#if defined PLATFORM_WINDOWS
  SYSTEMTIME st;
  FILETIME ft; // in 100 ns increments
  GetSystemTime( &st );
  SystemTimeToFileTime( &st, &ft );
  unsigned long long high_bits = ft.dwHighDateTime;
  unsigned long long t = ft.dwLowDateTime+(high_bits<<32);
  return t/10000000;
#else
  time_t ret;
  return time(&ret);
#endif
}

string SysInterface::config_dir(){
#if defined PLATFORM_WINDOWS
  char buf[MAX_PATH];
  SHGetFolderPath( HWND_TOPMOST, CSIDL_APPDATA,
		   NULL, SHGFP_TYPE_CURRENT, buf );
  return string( buf ) + '\\' + "sonarPM" +'\\';
  //return "C:\\";
#else
  return string( getenv("HOME") ) + "/.sonarPM/";
#endif
}


#ifdef PLATFORM_WINDOWS
BOOL windows_term_handler( DWORD fdwCtrlType ) {
  switch( fdwCtrlType ) {
  case CTRL_C_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    Logger::log_basic( "end" );
    AudioDev::terminate();
    exit( 0 );
    return true;
  // Pass other signals to the next handler.  (by returning false)
  default:
    return false;
  }
}
#else
void posix_term_handler( int signum ){
  Logger::log_basic( "end" );
  AudioDev::terminate();
  exit( 0 );
}
#endif


void SysInterface::register_term_handler(){
#ifdef PLATFORM_WINDOWS
  if(!SetConsoleCtrlHandler( (PHANDLER_ROUTINE) windows_term_handler, TRUE ))
    cerr << "register handler failed!" <<endl;
#else
  signal( SIGINT, posix_term_handler );
  signal( SIGQUIT, posix_term_handler );
  signal( SIGTERM, posix_term_handler );
  signal( SIGKILL, posix_term_handler );
#endif
}
