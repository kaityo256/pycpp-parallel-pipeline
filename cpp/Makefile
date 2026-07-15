CXX = g++
CXXFLAGS = -std=c++14 -O2 -I./param

all: percolation

percolation: main.cpp percolation.hpp
	$(CXX) $(CXXFLAGS) main.cpp -o $@

.PHONY: clean

clean:
	rm -f percolation
