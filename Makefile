SOURCES=$(wildcard *.cc)
PROGRAM=hcs
OBJECTS=$(SOURCES:%.cc=%.o)
CXXFLAGS=-std=c++0x -g3 -Wall


PREFIX?=$(HOME)/.local/


all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) -o $@ $^

clean:
	rm -rf $(OBJECTS) $(PROGRAM)


install: $(PROGRAM) |  manpage
	install $^ $(PREFIX)/bin/
	install -d $(PREFIX)/share/man/man1/ $(PREFIX)/share/$(PROGRAM)/
	install doc/HCS.1 $(PREFIX)/share/man/man1/


manpage: doc/BPM.1

doc/BPM.1: README.adoc
	a2x --doctype manpage --format manpage README.adoc -D doc/

indent:
	@astyle --style=linux -S -C -D -N -H -L -W3 -f $(SOURCES) 
