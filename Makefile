CXX	  = g++
RM        = rm -f
CXXFLAGS  = -O2 -Wall

FLAGS	  = $(CXXFLAGS)
LIBS	  = -L/usr/lib/gef -lvme


xvb_v895: xvb_v895.o
	@echo Linking $@ ...
	@$(CXX) -o $@ $^ $(LIBS)

clean:
	@echo Cleaning up ...
	@$(RM) *.o *~
	@$(RM) xvb_v895

%.o: %.cc
	@echo Compiling $< ...
	@$(CXX) $(FLAGS) -c $< -o $@
