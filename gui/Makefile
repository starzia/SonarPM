FLAGS = -Wno-write-strings

OBJS = App.o Frame.o TaskBarIcon.o PlotPane.o SonarThread.o PlotEvent.o
W32_OBJS = App.O Frame.O TaskBarIcon.O PlotPane.O SonarThread.O PlotEvent.O

App: $(OBJS)
	g++ `wx_config --libs --cppflags` $(FLAGS) $(OBJS) -o $@

# libary of objects
gui.a: $(OBJS)
	$(AR) rvs $@ $^

gui.A: $(W32_OBJS)
	$(AR) rvs $@ $^

%.o : %.cpp %.hpp
	$(CXX) $(shell $(WX_CONFIG_LINUX) --cppflags) $(FLAGS) -DPLATFORM_LINUX -c $< -o $@

%.O : %.cpp %.hpp
	$(CXX) $(shell $(WX_CONFIG_W32) --cppflags) $(FLAGS) -DPLATFORM_WINDOWS -c $< -o $@

clean:
	rm -f App gui.a gui.A $(OBJS) *~