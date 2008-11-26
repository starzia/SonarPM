sonar: sonar.cpp sonar.hpp
	g++ -lfftw3 -lportaudio -lm sonar.cpp -o sonar

test: sonar
	./sonar