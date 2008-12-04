sonar: sonar.cpp sonar.hpp
	g++ -ggdb -Wall -lportaudio -lm sonar.cpp -o sonar
#	g++ -ggdb -Wall -lXss -lportaudio -lm sonar.cpp -o sonar

test: sonar
	./sonar

clean:
	rm -f sonar *~