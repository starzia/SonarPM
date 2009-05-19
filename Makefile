FLAGS=-O3 -Wall -ggdb 
#     -msse2 -ggdb

# note that there are two versions of each object file; capital 'O' for windows
OBJS = audio.o dsp.o sonar.o
WINOBJS = audio.O dsp.O sonar.O

sonar: ${OBJS}
	${CXX} $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm ${OBJS} -o sonar
## Mac version:
#${CXX} $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar

sonar_static: ${OBJS}
	${CXX} -static $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm ${OBJS} -o $@

sonar.exe: ${WINOBJS}
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -lm ${WINOBJS} libportaudio.a \
           -lwinmm -lwininet -o $@

test: sonar
	./sonar --debug --poll

clean:
	rm -f sonar sonar.exe ${OBJS} ${WINOBJS} *~ sonar.tar.gz

%.o : %.cpp
	$(CXX) $(FLAGS) -DPLATFORM_LINUX -c $< -o $@
%.O : %.cpp
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -c $< -o $@

audio.o: audio.cpp audio.hpp
dsp.o: dsp.cpp dsp.hpp audio.hpp
sonar.o: sonar.cpp sonar.hpp audio.hpp dsp.hpp

audio.O: audio.cpp audio.hpp
dsp.O: dsp.cpp dsp.hpp audio.hpp
sonar.O: sonar.cpp sonar.hpp audio.hpp dsp.hpp

sonar.tar.gz:
	rm -Rf sonar_dist
	mkdir sonar_dist
	cp $(shell svn list) sonar_dist/
	tar -czvf sonar.tar.gz sonar_dist
	rm -Rf sonar_dist