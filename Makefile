export WX_CONFIG_LINUX=/usr/bin/wx-config
export WX_CONFIG_W32=/usr/local/wx-2.8.10-mingw32/bin/wx-config

FLAGS=-O3 -Wall -ggdb 
#     -msse2 -ggdb

# note that there are two versions of each object file; capital 'O' for windows
OBJS = audio.o dsp.o sonar.o
W32_OBJS = audio.O dsp.O sonar.O
# also two versions of each binary: suffix '.exe' for windows
BINS = sonar_tui sonar_gui sonar_tui.exe sonar_gui.exe

############################# LINKING ####################################

sonar_tui: ${OBJS} sonar_tui.o
	$(CXX) -o $@ $^ -lXss -lportaudio -lm
## Mac version?
#${CXX} $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar

sonar_gui: ${OBJS} gui/gui.a
	$(CXX) -o $@ $^ \
          $(shell $(WX_CONFIG_LINUX) --libs) -lXss -lportaudio -lm

sonar_tui.exe: ${W32_OBJS} sonar_tui.O
	$(CXX) -o $@ $^ -lm libportaudio.a -lwinmm -lwininet

# for some reason, gui.A cannot be used directly, so I list its contents and
# use those filenames instead.  This is a bit of a hack.
sonar_gui.exe: ${W32_OBJS} gui/gui.A
	$(CXX) -o $@ ${W32_OBJS} \
          $(addprefix gui/, $(shell $(AR) -t gui/gui.A)) \
          libportaudio.a -lm -lwinmm -lwininet $(shell $(WX_CONFIG_W32) --libs)

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

