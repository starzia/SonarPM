FLAGS = `wx-config --cxxflags` -Wno-write-strings

OBJS = App.o Frame.o TaskBarIcon.o PlotPane.o SonarThread.o PlotEvent.o

App: $(OBJS)
	g++ `wx-config --libs` $(FLAGS) $(OBJS) -o $@

# libary of objects
gui.a: $(OBJS)
	ar rvs $@ $(OBJS)

%.o : %.cpp %.hpp
	$(CXX) $(FLAGS) -c $< -o $@

clean:
	rm -f App gui.a $(OBJS) *~