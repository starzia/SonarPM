PREREQUISITES
-------------
Under Linux, there are two main libraries required to build this code:
- Portaudio cross platform audio library
    This is available in Fedora as package "portaudio-devel"
    and in Debian as package "libportaudio-dev"
- X Screen Saver extension library
    This is available in Fedora as package "libScrnSaver-devel"
    and in Debian as package "libxss-dev"
- curl is required for FTP uploading of the log file.


COMPILATION
-----------
Run 'make'.

You must also manually create the directory ~/sonarPM/ before the program
can be run.


USAGE
-----
 ./sonar_gui


ADVANCED USAGE
--------------
You can change the ping frequency by editing the config file: 
~/sonarPM/sonarPM.cfg

Erase ~/sonarPM/* to eliminate the current configuration and start afresh.

