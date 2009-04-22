FLAGS=-O3 -msse2 -ggdb -Wall

PLATFORM=LINUX
#PLATFORM=MAC
#PLATFORM=WINDOWS

sonar: sonar.cpp sonar.hpp
ifeq ($(PLATFORM),LINUX)
	g++ $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm sonar.cpp -o sonar
endif
ifeq ($(PLATFORM),MAC)
	g++ $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar
endif

test: sonar
	./sonar --debug --poll

clean:
	rm -f sonar *~