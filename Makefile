SOURCES=$(wildcard *.cc)
PROGRAM=hcs
OBJECTS=$(SOURCES:%.cc=%.o)
CXXFLAGS=-std=c++0x -g3 -Wall


all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) -o $@ $^

clean:
	rm -rf $(OBJECTS) $(PROGRAM)
