FLAGS=-O3 -Wall -ggdb 
#     -msse2 -ggdb

# note that there are two versions of each object file; capital 'O' for windows
OBJS = audio.o dsp.o sonar.o
WINOBJS = audio.O dsp.O sonar.O sonar_tui.O

sonar_tui: ${OBJS} sonar_tui.o
	${CXX} $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm ${OBJS} sonar_tui.o -o $@
## Mac version:
#${CXX} $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar

sonar_gui: ${OBJS} gui/gui.a
	${CXX} $(FLAGS) -DPLATFORM_LINUX `wx-config --libs` -lXss -lportaudio -lm ${OBJS} gui/gui.a -o sonar_gui

sonar_static: ${OBJS}
	${CXX} -static $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm ${OBJS} -o $@

sonar.exe: ${WINOBJS}
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -lm ${WINOBJS} libportaudio.a \
           -lwinmm -lwininet -o $@

test: sonar
	./sonar --debug --poll

gui/gui.a: FORCE_LOOK
	cd gui; $(MAKE) $(MFLAGS) gui.a

FORCE_LOOK:
	true

clean:
	rm -f sonar_tui sonar_gui sonar.exe ${OBJS} ${WINOBJS} *~ sonar.tar.gz

%.o : %.cpp
	$(CXX) $(FLAGS) -DPLATFORM_LINUX -c $< -o $@
%.O : %.cpp
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -c $< -o $@

sonar.tar.gz:
	rm -Rf sonar_dist
	mkdir sonar_dist
	cp $(shell svn list) sonar_dist/
	tar -czvf sonar.tar.gz sonar_dist
	rm -Rf sonar_dist


# DEPENDENCIES

audio.o: audio.cpp audio.hpp
dsp.o: dsp.cpp dsp.hpp audio.hpp
sonar.o: sonar.cpp sonar.hpp audio.hpp dsp.hpp

audio.O: audio.cpp audio.hpp
dsp.O: dsp.cpp dsp.hpp audio.hpp
sonar.O: sonar.cpp sonar.hpp audio.hpp dsp.hpp

