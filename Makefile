PLATFORM=LINUX
#PLATFORM=MAC
#PLATFORM=WINDOWS

sonar: sonar.cpp sonar.hpp
ifeq ($(PLATFORM),LINUX)
	g++ -ggdb -DPLATFORM_LINUX -Wall -lXss -lportaudio -lm sonar.cpp -o sonar
endif
ifeq ($(PLATFORM),MAC)
	g++ -DPLATFORM_MAC -Wall -lportaudio -lm sonar.cpp -o sonar
endif

test: sonar
	./sonar --debug --poll

clean:
	rm -f sonar *~