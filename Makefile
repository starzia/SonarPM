FLAGS=-O3 -Wall
#     -msse2 -ggdb

sonar: sonar.cpp sonar.hpp
	g++ $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm sonar.cpp -o sonar
## Mac version:
#g++ $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar

sonar_static: sonar.cpp sonar.hpp
	g++ -static $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm sonar.cpp -o sonar_static

sonar.exe: sonar.cpp sonar.hpp
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -lm sonar.cpp libportaudio.a \
           -lwinmm -lwininet -o sonar.exe

test: sonar
	./sonar --debug --poll

clean:
	rm -f sonar sonar.exe *~

sonar.tar.gz:
	rm -Rf sonar_dist
	mkdir sonar_dist
	cp $(shell svn list) sonar_dist/
	tar -czvf sonar.tar.gz sonar_dist
	rm -Rf sonar_dist