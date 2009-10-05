Sonar Power Manager
Author: Stephen Tarzia
        steve@stevetarzia.com

This code was written at Northwestern university in collaboration with
Robert Dick (U. Michigan), Peter Dinda (Northwestern), and Gokhan Memik 
(Northwestern).
========================================================================

LINUX PREREQUISITES
-------------------
Under Linux, there are two main libraries required to build this code:
- Portaudio cross platform audio library
    This is available in Fedora as package "portaudio-devel"
    and in Debian as package "libportaudio-dev"
- X Screen Saver extension library
    This is available in Fedora as package "libScrnSaver-devel"
    and in Debian as package "libxss-dev"
- curl is required for FTP uploading of the log file.


LINUX COMPILATION
-----------------
Run 'make sonar_gui' or 'make sonar_tui'.

You must also manually create the directory ~/sonarPM/ before the program
can be run.


LINUX USAGE
-----------
'./sonar_gui' or './sonar_tui'


ADVANCED USAGE
--------------
You can change the ping frequency by editing the config file: 
~/sonarPM/sonarPM.cfg

Erase ~/sonarPM/* to eliminate the current configuration and start afresh.

'./sonar_tui --poll' is useful for piping the output into a script or for
storing a simple log of sonar readings.


WINDOWS PREREQUISITES
---------------------
Compiling for Windows is a bit trickier.  Portaudio is included in the 
source tarball, but wxWidgets library must be installed (and the Makefile 
edited to reflect its installation directory).


WINDOWS COMPILATION
-------------------
You can compile either with mingw using the standard Makfile:
'make sonar_gui.exe' or 'make sonar_tui.exe'

or with the MS Visual Studio commandline:
'nmake -f Makefile.vc'

In either case, you must manually create the folder:
'[Application Data]/sonarPM'
Normally the installer would do this, but the installer is not presently
included in the source tarball.
