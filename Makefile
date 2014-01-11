SOURCES=$(wildcard *.cc)
PROGRAM=hcs
OBJECTS=$(SOURCES:%.cc=%.o)
CXXFLAGS=-std=c++0x -g3 -Wall -Werror

MANPAGE=hcs.1
PREFIX?=$(HOME)/.local/


all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) -o $@ $^

clean:
	rm -rf $(OBJECTS) $(PROGRAM) $(MANPAGE)


install: $(PROGRAM) |  manpage
	install $^ $(PREFIX)/bin/
	install -d $(PREFIX)/share/man/man1/ $(PREFIX)/share/$(PROGRAM)/
	install hcs.1 $(PREFIX)/share/man/man1/


manpage: $(MANPAGE)

$(MANPAGE): README.adoc
	a2x --doctype manpage --format manpage README.adoc 

indent:
	@astyle --style=linux -S -C -D -N -H -L -W3 -f $(SOURCES) 
