
AM_LDFLAGS=
AM_CPPFLAGS= \
    $(NULL)

DEFS+= \
    -std=c++0x \
    `getconf LFS_CFLAGS` \
    `getconf LFS64_CFLAGS` \
    -D_GNU_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    $(NULL)

AM_CPPFLAGS+= \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/include/ \
    $(NULL)

AM_LDFLAGS += \
    -L$(top_builddir)/src/ \
    `getconf LFS_LDFLAGS` \
    `getconf LFS64_LDFLAGS` \
    $(NULL)

if DEBUG
# use "valgrind --tool=memcheck --leak-check=yes" to check memory leak, MemWatch will drag the program.
#DEFS+=-DMEMWATCH
DEFS+= -DDEBUG=1
AM_CPPFLAGS+=-g -Wall

else
AM_CPPFLAGS+=-O3 -Wall
endif

DEFS+=$(AM_CPPFLAGS)

EXT_FLAGS=
@MK@GITNUMTMP=$(shell cd "$(top_srcdir)"; A=$$(git show | head -n 1 | awk '{print $$2}'); echo $${A:0:7}; cd - > /dev/null )
#@MK@SVNNUMTMP=$(shell cd "$(top_srcdir)"; LC_ALL=C svn info | grep -i Revision | awk '{print $$2}'; cd - > /dev/null )
#@MK@ifeq ($(SVNNUMTMP),)
#EXT_FLAGS+= -DSVN_VERSION='"${GITNUMTMP}"'
#@MK@else
#EXT_FLAGS+= -DSVN_VERSION='"${SVNNUMTMP}"'
#@MK@endif
@MK@ifeq ($(GITNUMTMP),)
@MK@else
EXT_FLAGS+= -DSVN_VERSION='"${GITNUMTMP}"'
@MK@endif
DEFS+=$(EXT_FLAGS)

#dist_bin_SCRIPTS=tools/genpages.sh
bin_PROGRAMS=buttonexample
#bin_PROGRAMS+=ledblinkexample
#bin_PROGRAMS+=pwrledbuttexample

base_SOURCES= \
    src/button.cpp \
    src/ledblink.cpp \
    src/pwrledbutt.cpp \
    src/sysport.cpp \
    src/timerint.cpp \
    $(NULL)

buttonexample_SOURCES= \
    $(base_SOURCES) \
    examples/buttonexample/buttonexample.cpp \
    $(NULL)

#BUILT_SOURCES = fontutf8test/fontutf8-data.h
#CLEANFILES = fontutf8test/fontutf8-data.h

.pde.cpp:
	cp $< $@
.ino.cpp:
	cp $< $@
# force the autoconf use g++ to link the c program with c++ lib
dummy.cpp:
	touch $@

buttonexample_CFLAGS=
buttonexample_CPPFLAGS=$(buttonexample_CFLAGS)
buttonexample_LDFLAGS=
buttonexample_LDADD=



