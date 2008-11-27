sonar: sonar.cpp sonar.hpp
	g++ -Wall -lfftw3 -lportaudio -lm sonar.cpp -o sonar

test: sonar
	./sonar

clean:
	rm -f sonar *~