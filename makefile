CCC = g++ 
CXXFLAGS = -Wall -g 
CXXLINK = $(CCC)
OBJS =cache.o cacheSim.o
RM = rm -f 
#Default target (usually "all") 
all: cacheSim 
#Creating the executables
cacheSim: $(OBJS) 
	$(CXXLINK) -o cacheSim $(OBJS) 
#Creating object files using default rules 
cacheSim.o: cacheSim.cpp cache.h 
cache.o: cache.cpp cache.h
#Cleaning old files before new make 
clean:
	$(RM) cacheSim  *.o *.bak *~ "#"* core

