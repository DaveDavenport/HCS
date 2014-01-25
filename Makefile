BUILD_DIR=build
SOURCES=$(wildcard src/*.cc)
PROGRAM=hcs
OBJECTS=$(SOURCES:src/%.cc=$(BUILD_DIR)/%.o)
CXXFLAGS=-std=c++0x -g3 -Wall -Werror 
QUIET?=@

MANPAGE=hcs.1
PREFIX?=$(HOME)/.local/

all: $(BUILD_DIR)/$(PROGRAM) manpage

$(BUILD_DIR):
	$(info Create:       Build directory)
	$(QUIET)mkdir -p $@

$(BUILD_DIR)/%.o: src/%.cc | Makefile $(BUILD_DIR)
	$(info Build object: $^ -> $@)
	$(QUIET)$(CXX) $(CXXFLAGS) -c -o $@ $^
   

$(BUILD_DIR)/$(PROGRAM): $(OBJECTS)
	$(info Link program: $@)
	$(QUIET)$(CXX) -o $@ $^ -lreadline

clean:
	rm -rf $(BUILD_DIR)


install: $(BUILD_DIR)/$(PROGRAM) |  manpage
	install -d $(PREFIX)/share/man/man1/
	install $^ $(PREFIX)/bin/
	install $(BUILD_DIR)/$(MANPAGE) $(PREFIX)/share/man/man1/$(MANPAGE)


manpage: $(BUILD_DIR)/$(MANPAGE) | $(BUILD_DIR)

$(BUILD_DIR)/$(MANPAGE): doc/README.adoc
	$(info Create manpage: $@ -> $^ )	
	$(QUIET)a2x --doctype manpage --format manpage doc/README.adoc -D $(BUILD_DIR)/

indent: $(SOURCES)
	$(info Indent source files: $^)
	@astyle --style=linux -S -C -D -N -H -L -W3 -f $^
