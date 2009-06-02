FLAGS = `wx-config --cxxflags` -Wno-write-strings

OBJS = App.o Frame.o TaskBarIcon.o PlotPane.o SonarThread.o

App: $(OBJS)
	g++ `wx-config --libs` $(FLAGS) $(OBJS) -o $@

%.o : %.cpp %.hpp
	$(CXX) $(FLAGS) -c $< -o $@

clean:
	rm -f App $(OBJS) *~