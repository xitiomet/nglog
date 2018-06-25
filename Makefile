CPC=g++

all: nglog

nglog: nglog.o
	$(CPC) $< -o $@
    
%.o: %.cc
	$(CPC) -I. -c $< -o $@

clean:
	rm -f nglog *~ *.o
