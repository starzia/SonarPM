# note that we are using fftw3f, the single-precision version of fftw
sonar: sonar.cpp sonar.hpp
	g++ -ggdb -Wall -lfftw3f -lportaudio -lm sonar.cpp -o sonar
#	g++ -ggdb -Wall -lXss -lfftw3f -lportaudio -lm sonar.cpp -o sonar

test: sonar
	./sonar

clean:
	rm -f sonar *~