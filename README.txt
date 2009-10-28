Sonar Power Manager
Author: Stephen Tarzia
        steve@stevetarzia.com

This code was written at Northwestern university in collaboration with:
 Robert Dick (U. Michigan)
 Peter Dinda (Northwestern)
 Gokhan Memik (Northwestern).

For technical details see this publication:
 S. P. Tarzia, R. P. Dick, P. A. Dinda, G. Memik. Sonar-based Measurement of 
 User Presence and Attention. In Proc. 11th Intl. Conf. on Ubiquitous 
 Computing (UbiComp'09). September-October 2009. pages 89-92.
========================================================================

LICENSE
-------
This source code is released under the "MIT license".  Please see LICENSE.txt
for details.  Optionally, please notify us of projects insipired by this 
project so that we can link you.


LINUX PREREQUISITES
-------------------
Under Linux, there are two main libraries required to build this code:
- Portaudio cross platform audio library V19 (V18 won't work)
    This is available in Fedora as package "portaudio-devel"
    and in Debian as package "portaudio19-dev" (or "libportaudio-dev"?)
- X Screen Saver extension library
    This is available in Fedora as package "libScrnSaver-devel"
    and in Debian as package "libxss-dev"
- curl is required for FTP uploading of the log file.
- wxWidgets 2.8


LINUX COMPILATION
-----------------
Run 'make sonar_gui' or 'make sonar_tui'.

You must also manually create the directory ~/.sonarPM/ before the program
can be run.


LINUX USAGE
-----------
'./sonar_gui' or './sonar_tui'


ADVANCED USAGE
--------------
You can change the ping frequency by editing the config file: 
~/.sonarPM/sonarPM.cfg

Erase ~/.sonarPM/* to eliminate the current configuration and start afresh.

'./sonar_tui --poll' is useful for piping the output into a script or for
storing a simple log of sonar readings.


WINDOWS PREREQUISITES
---------------------
Compiling for Windows is a bit trickier.  Portaudio must be downloaded and 
moved into a 'portaudio' directory and wxWidgets library must be installed
(and Makefile.vc edited to reflect its installation directory).


WINDOWS COMPILATION
-------------------
You can compile either with mingw using the standard Makefile:
'make sonar_gui.exe' or 'make sonar_tui.exe'

or with the MS Visual Studio commandline (this method preferred):
'nmake -f Makefile.vc'

In either case, you must manually create the folder:
'[Application Data]/sonarPM'
Normally the installer would do this, but the installer is not presently
included in the source tarball.  Also, be sure that gzip.exe is located in the
same folder as sonar_gui.exe.


CHANGELOG
---------
Version 0.8
- added "about" window
- fixed sonar clicking
- fixed logging
  - log power status on thread start
  - log application version
  - removed duplicate "sleep timeout" messages
  - log un-sleep (re-active)
0 added Macintosh compilation instructions


WISH LIST
---------
These are items that should be added to future releases
- Mac OS support
- User control of:
  - the sleep threshold
  - the delay before sonar readings begin
  - sonar ping frequency
- expanded frequency response measurement
- Adaptive sonar readings (take fewer readings when user is unlikely to change presence status, ie after a long stream of "present" readings).
