FLAGS = -Wno-write-strings -ggdb

OBJS = App.o Frame.o TaskBarIcon.o PlotPane.o SonarThread.o PlotEvent.o \
 ConfigFrame.o
W32_OBJS = App.O Frame.O TaskBarIcon.O PlotPane.O SonarThread.O PlotEvent.O \
 ConfigFrame.O

App: $(OBJS)
	$(shell $(WX_CONFIG_LINUX) --cxx --cppflags --libs) $(FLAGS) $(OBJS) -o $@

# libary of objects
gui.a: $(OBJS)
	$(AR) rvs $@ $^

gui.A: $(W32_OBJS)
	$(AR) rvs $@ $^

%.o : %.cpp %.hpp
	$(shell $(WX_CONFIG_LINUX) --cxx --cppflags) $(FLAGS) -DPLATFORM_LINUX -c $< -o $@

%.O : %.cpp %.hpp
	$(shell $(WX_CONFIG_W32) --cxx --cppflags) $(FLAGS) -DPLATFORM_WINDOWS -c $< -o $@

clean:
	rm -f App gui.a gui.A $(OBJS) $(W32_OBJS) *~