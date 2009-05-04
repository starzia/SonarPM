FLAGS=-O3 -Wall
#     -msse2 -ggdb

sonar: sonar.cpp sonar.hpp
	g++ $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm sonar.cpp -o sonar
	## Mac version:
	#g++ $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar

sonar.exe: sonar.cpp sonar.hpp
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -lm sonar.cpp libportaudio.a \
           -lwinmm -o sonar.exe

test: sonar
	./sonar --debug --poll

clean:
	rm -f sonar *~