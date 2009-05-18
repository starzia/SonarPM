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
Just run 'make'.


USAGE
-----
To run the full-blown power managment algorithm run:
 ./sonar

ADVANCED USAGE
--------------
To just get the sonar readings run:
 ./sonar --poll [freq]
where [freq] optionally sets the sonar ping frequency

To run an echo test on your audio hardware run:
 ./sonar --echo

To get the "frequency response" of your audio hardware run:
 ./sonar --response

Configuration data is stored in ~/.sonarPM.cfg.  Erase this file to revert to
the default settings (or edit the settings manually).

