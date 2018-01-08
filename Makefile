
# This is the name of the project. We get it from the basename of 
# the project folder. It may contain mixed-case, and "-" as word
# separators. CamelCase is discouraged.
#
# The MAJORVER and MINORVER here are place holders; during a 
# gitpkgtool build they are substituted in-place
# by patchVersion.sh.

CWD=$(shell pwd)
PROJECT=$(shell basename $(CWD))
MAJORVER=1
MINORVER=0.1.gb35f430

# The debian package name. It may contain "-" as a word seperator
# but must be all lower case.

PACKAGE=$(shell echo $(PROJECT) | tr A-Z a-z)


# the library name. it may contain mixed case but the "-" should be
# removed. 
LIBNAME=$(shell echo $(PROJECT)|sed -e 's/-//g')

# the libname without the "lib" prefix, used when linking
LNAME=$(shell echo $(LIBNAME)|sed -e 's/^lib//')


# DESTDIR is overridden by debhelper.
PREFIX=$(DESTDIR)/usr
CC=g++
AR=ar
CFLAGS=-std=c++11 -I src/libsrc -I src/test -O3 
LIBS= $(LNAME) pthread
LDOPTS=$(patsubst %, -l%, $(LIBS))

LIBSRCS=$(wildcard src/libsrc/*.cpp)
LIBOBJS=$(patsubst %.cpp,%.lo, $(LIBSRCS))


TESTSRCS=$(wildcard src/test/*.cpp)
TESTOBJS=$(patsubst %.cpp,%.o, $(TESTSRCS))

EXE=$(patsubst %.o,%,$(TESTOBJS))


LIBFILE=$(LIBNAME).so.$(MAJORVER).$(MINORVER)

# The soname is the MAJORVER. Therefore MAJORVER is
# incremented whenever the ABI changes, and MINROVER is
# incremented when (only) the implementation changes.

SONAME=$(LIBNAME).so.$(MAJORVER)

world: all

%.o:%.cpp
	@echo "\t[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

%.lo:%.cpp
	@echo "\t[CC] $<"
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<
	

$(LIBFILE):$(LIBOBJS)
	@echo "\tBuilding Shared Lib $@"
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $(LIBOBJS) -lpthread
	ln -sf $(LIBFILE) $(SONAME)
	ln -sf $(LIBFILE) $(LIBNAME).so


src/test/TestXPlaneBeaconListener: $(TESTOBJS) $(LIBFILE)
	$(CC) -L . -o $@ src/test/TestXPlaneBeaconListener.o $(LIBNAME).so
	

src/test/TestXPlaneUDPClient: $(TESTOBJS) $(LIBFILE)
	$(CC) -L . -o $@ src/test/TestXPlaneUDPClient.o $(LIBNAME).so

clean:
	@echo "\t[CLEAN]"
	@rm -rf $(LIBOBJS) $(TESTOBJS) $(EXE) \
		$(LIBFILE) $(SONAME) $(LIBNAME).so \
		docs

distclean: clean

doc:
	doxygen

install: $(LIBFILE) $(EXE) doc
	install -D $(LIBFILE) $(PREFIX)/lib/$(LIBFILE)
	ln -sf $(LIBFILE) $(PREFIX)/lib/$(SONAME)
	ln -sf $(LIBFILE) $(PREFIX)/lib/$(LIBNAME).so
	install -D $(EXE) --target-directory=$(PREFIX)/bin
	install -D src/libsrc/XPlaneBeaconListener.h $(PREFIX)/include/XPlaneBeaconListener.h
	install -D src/libsrc/XPlaneUDPClient.h $(PREFIX)/include/XPlaneUDPClient.h
	mkdir -p $(PREFIX)/share/doc/$(PACKAGE)$(MAJORVER)-dev
	cp -r docs/* $(PREFIX)/share/doc/$(PACKAGE)$(MAJORVER)-dev

all: $(LIBFILE) $(EXE)
