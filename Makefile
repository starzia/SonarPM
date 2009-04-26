FLAGS=-O3 -Wall
#     -msse2 -ggdb

#PLATFORM=LINUX
#PLATFORM=MAC
PLATFORM=WINDOWS

sonar: sonar.cpp sonar.hpp
ifeq ($(PLATFORM),LINUX)
	g++ $(FLAGS) -DPLATFORM_LINUX -lXss -lportaudio -lm sonar.cpp -o sonar
endif
ifeq ($(PLATFORM),MAC)
	g++ $(FLAGS) -DPLATFORM_MAC -lportaudio -lm sonar.cpp -o sonar
endif
ifeq ($(PLATFORM),WINDOWS)
	$(CXX) $(FLAGS) -DPLATFORM_WINDOWS -lm sonar.cpp libportaudio.a -lwinmm -o sonar.exe
endif

test: sonar
	./sonar --debug --poll

clean:
	rm -f sonar *~