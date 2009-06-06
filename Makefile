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
	${CXX} $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm $^ -o $@
## Mac version:
#${CXX} $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar

sonar_gui: ${OBJS} gui/gui.a
	${CXX} $(FLAGS) -DPLATFORM_LINUX $(shell $(WX_CONFIG_LINUX) --libs) -lXss -lportaudio -lm $^ -o $@

sonar_tui.exe: ${W32_OBJS} sonar_tui.O
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -lm $^ libportaudio.a -lwinmm -lwininet -o $@

sonar_gui.exe: ${W32_OBJS} gui/gui.A
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS $(shell $(WX_CONFIG_W32) --libs) -lm $^ libportaudio.a -lwinmm -lwininet -o $@

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

