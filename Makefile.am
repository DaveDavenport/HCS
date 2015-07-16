##
# Rofi the program
##
bin_PROGRAMS=hcs

LIBS=\
	@libudev_LIBS@\
    -lreadline

AM_CXXFLAGS=\
    -I$(top_srcdir)/include/\
    -I$(top_builddir)/

hcs_SOURCES=\
    src/hcs.cc\
	src/hcs-ea.cc\
	src/hcs-pps.cc\
	include/hcs.h\
	include/hcs-ea.h\
	include/hcs-pps.h

indent: ${hcs_SOURCES}
	uncrustify -c ${top_srcdir}/data/uncrustify.cfg --replace $^
