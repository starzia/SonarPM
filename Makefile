export WX_CONFIG_LINUX=/usr/bin/wx-config
export WX_CONFIG_W32=/usr/local/wx-2.8.10-mingw32/bin/wx-config

FLAGS=-O3 -Wall -ggdb
#-ggdb -pg
#     -msse2 -ggdb

# note that there are two versions of each object file; capital 'O' for windows
OBJS = audio.o dsp.o sonar.o
W32_OBJS = audio.O dsp.O sonar.O
# also two versions of each binary: suffix '.exe' for windows
BINS = sonar_tui sonar_gui sonar_tui.exe sonar_gui.exe

############################# LINKING ####################################

sonar_tui: ${OBJS} sonar_tui.o
	$(CXX) -o $@ ${FLAGS} $^ -lXss -lportaudio -lm
## Mac version?
#${CXX} $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar

sonar_gui: ${OBJS} gui/gui.a
	$(CXX) -o $@ $^ \
          $(shell $(WX_CONFIG_LINUX) --libs) -lXss -lportaudio -lm

sonar_tui.exe: ${W32_OBJS} sonar_tui.O
	$(CXX) -o $@ $^ -lm libportaudio.a -lwinmm -lwininet
	$(STRIP) $@

# for some reason, gui.A cannot be used directly, so I list its contents and
# use those filenames instead.  This is a bit of a hack.
sonar_gui.exe: ${W32_OBJS} gui/gui.A
	$(CXX) -o $@ ${W32_OBJS} \
          $(addprefix gui/, $(shell $(AR) -t gui/gui.A)) \
          libportaudio.a -lm -lwinmm -lwininet $(shell $(WX_CONFIG_W32) --libs)
	$(STRIP) $@

# the following target lets me see STDOUT and STDERR in windows
sonar_gui_debug.exe: ${W32_OBJS} gui/gui.A
	/usr/bin/i686-pc-mingw32-c++ -o sonar_gui.exe \
 audio.O dsp.O sonar.O gui/App.O gui/Frame.O gui/TaskBarIcon.O gui/PlotPane.O gui/SonarThread.O gui/PlotEvent.O gui/ConfigFrame.O gui/CloseConfirmFrame.O \
 libportaudio.a -lm -lwinmm -lwininet -L/usr/local/wx-2.8.10-mingw32/lib  -mthreads  -Wl,--subsystem,windows -mwindows /usr/local/wx-2.8.10-mingw32/lib/libwx_msw_richtext-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_msw_aui-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_msw_xrc-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_msw_qa-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_msw_html-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_msw_adv-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_msw_core-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_base_xml-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_base_net-2.8.a /usr/local/wx-2.8.10-mingw32/lib/libwx_base-2.8.a -lwxregex-2.8 -lwxexpat-2.8 -lwxtiff-2.8 -lwxjpeg-2.8 -lwxpng-2.8 -lwxzlib-2.8 -lrpcrt4 -loleaut32 -lole32 -luuid -lwinspool -lwinmm -lshell32 -lcomctl32 -lcomdlg32 -lctl3d32 -ladvapi32 -lwsock32 -lgdi32

############################## COMPILING #################################

gui/gui.a: FORCE_LOOK
	cd gui; $(MAKE) $(MFLAGS) gui.a

gui/gui.A: FORCE_LOOK
	cd gui; $(MAKE) $(MFLAGS) gui.A

FORCE_LOOK:
	true

%.o : %.cpp
	$(CXX) $(FLAGS) -DPLATFORM_LINUX -c $< -o $@
%.O : %.cpp
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -c $< -o $@

############################# MISC #######################################

clean:
	rm -f ${BINS} ${OBJS} ${W32_OBJS} *~ sonar.tar.gz
	cd gui; $(MAKE) clean

sonar.tar.gz:
	rm -Rf sonar_dist
	mkdir sonar_dist
	cp $(shell svn list) sonar_dist/
	mkdir sonar_dist/gui
	cp $(shell svn list gui) sonar_dist/gui/
	tar -czvf sonar.tar.gz sonar_dist
	rm -Rf sonar_dist

############################ DEPENDENCIES #################################

audio.o: audio.cpp audio.hpp
dsp.o: dsp.cpp dsp.hpp audio.hpp
sonar.o: sonar.cpp sonar.hpp audio.hpp dsp.hpp

audio.O: audio.cpp audio.hpp
dsp.O: dsp.cpp dsp.hpp audio.hpp
sonar.O: sonar.cpp sonar.hpp audio.hpp dsp.hpp

