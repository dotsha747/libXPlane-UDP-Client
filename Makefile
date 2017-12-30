
# This is the name of the package
NAME=libxplane-udp-client
MAJORVER=1
MINORVER=0

# the name with "-" removed, used to name the library.
LIBNAME=$(shell echo $(NAME)|sed -e 's/-//g')$(MAJORVER)

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


$(EXE):$(TESTOBJS) $(LIBFILE)
	@echo "\t[LD] $@"
	$(CC) -L . -o $@ $< $(LDOPTS)

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
	mkdir -p $(PREFIX)/share/doc/$(NAME)$(MAJORVER)-dev
	cp -r docs/* $(PREFIX)/share/doc/$(NAME)$(MAJORVER)-dev

all: $(LIBFILE) $(EXE)
