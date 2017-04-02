src=../common/shell.cpp

# override with environment variable
CXX ?= g++

# Path to perl header files
INCLUDEDIR ?= ${HOME}/perl5/perlbrew/perls/slic3r-perl/lib/5.24.0/x86_64-linux-thread-multi/CORE

# path to library files for perl
LIBDIR ?= ${HOME}/perl5/perlbrew/perls/slic3r-perl/lib/5.24.0/x86_64-linux-thread-multi/CORE

LIBS += -lperl -lpthread -lcrypt

CXXFLAGS += -std=c++11 -static-libgcc -static-libstdc++ -I${INCLUDEDIR}
LDFLAGS += -L${LIBDIR}

.PHONY: all clean
all: Slic3r Slic3r-console

Slic3r: slic3r.o
	${CXX} ${LDFLAGS} -o $@ $< ${LIBS}

Slic3r-console: slic3r-console.o
	${CXX} ${LDFLAGS} -o $@ $< ${LIBS}
slic3r-console.o: ${src}
	${CXX} -c ${CXXFLAGS} -o $@ $<
slic3r.o: ${src}
	${CXX} -c -DFORCE_GUI ${CXXFLAGS} -o $@ $<

clean: 
	rm *.o Slic3r*
