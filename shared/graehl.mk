#set BOOST_SUFFIX=gcc-mt if your boost libraries are libboost-thread$(BOOST_SUFFIX).so
#You provide:
# (the variables below)
# ARCH (if macosx, static builds are blocked)
#PROGS=a b
#build boost yourself and ensure that you have -Lwhatever -Iwhatever for it in CPPFLAGS
#a_OBJ=a.o ... a_SLIB=lib.o lib.a (static libraries e.g. a_LIB=$(BOOST_OPTIONS_LIB))
#a_NOSTATIC=1 a_NOTEST=1 ...
#NOSTATIC=1 (global setting)
# CXXFLAGS CXXFLAGS_DEBUG CXXFLAGS_TEST
# LIB = math thread ...
# INC = .
###WARNING: don't set BASEOBJ BASESHAREDOBJ or BASEBIN to directories including other important stuff or they will be nuked by make allclean

#CPPNOWIDECHAR = $(addprefix -D,BOOST_NO_CWCHAR BOOST_NO_CWCTYPE BOOST_NO_STD_WSTRING BOOST_NO_STD_WSTREAMBUF)

#include $(SHARED)/debugger.mk
#$(__BREAKPOINT)

# you can say things like "make echo-CC" to see what the variable CC is set to

echo-%:
	@echo $* == "\"$($*)\""

echoraw-%:
	@echo "$($*)"

ifndef BUILD_BASE
BUILD_BASE:=.
endif
LIB += z
ifndef ARCH
UNAME=$(shell uname)
ARCH=cygwin
ifeq ($(UNAME),Linux)
 ARCH=linux
endif
ifeq ($(UNAME),SunOS)
 ARCH=solaris
endif
ifeq ($(UNAME),Darwin)
 ARCH=macosx
endif
endif

ifeq ($(ARCH),cygwin)
CYGEXE:=.exe
else
CYGEXE:=
endif

ifeq ($(ARCH),cygwin)
NOSTATIC=1
CPPFLAGS += -DBOOST_POSIX -DCYGWIN
#CPPFLAGS += -DBOOST_NO_STD_WSTRING
#CPPNOWIDECHAR = $(addprefix -D,BOOST_NO_CWCHAR BOOST_NO_CWCTYPE BOOST_NO_STD_WSTRING BOOST_NO_STD_WSTREAMBUF)
# somehow that is getting automatically set by boost now (for Boost CVS)
endif

ifndef ARCH_FLAGS
ifeq ($(ARCH),linux64)
ARCH_FLAGS = -march=athlon64
else
 ifeq ($(ARCH),linux)
#  ARCH_FLAGS = -m32 -march=pentium4
 endif
endif
endif

ifndef INSTALL_PREFIX
ifdef HOSTBASE
INSTALL_PREFIX=$(HOSTBASE)
else
ifdef ARCHBASE
INSTALL_PREFIX=$(ARCHBASE)
else
INSTALL_PREFIX=$(HOME)
endif
endif
endif
ifndef BIN_PREFIX
BIN_PREFIX=$(INSTALL_PREFIX)/bin
endif

ifndef TRUNK
TRUNK=../..
endif

ifndef SHARED
SHARED=../shared
endif
ifndef BOOST_DIR
# note: BOOST_DIR not used for anything now, we assume boost somewhere in std include
#BOOST_DIR:=../boost
#BOOST_DIR=~/isd/$(HOST)/include
endif
ifndef BASEOBJ
BASEOBJ=$(BUILD_BASE)/obj
endif
ifndef BASEBIN
BASEBIN=bin
endif
ifndef BASESHAREDOBJ
BASESHAREDOBJ=$(SHARED)/obj
endif

.SUFFIXES:
.PHONY = distclean all clean depend default

ifndef ARCH
  ARCH := $(shell print_arch)
  export ARCH
endif

ifndef BUILDSUB
  ifdef HOST
   BUILDSUB=$(HOST)
  else
   BUILDSUB=$(ARCH)
  endif
endif
DEPSPRE:= $(BUILD_BASE)/deps/$(BUILDSUB)/

# workaround for eval line length limit: immediate substitution shorter?
OBJ:= $(BASEOBJ)/$(BUILDSUB)
OBJT:= $(OBJ)/test
ifndef OBJB
OBJB:= $(BASESHAREDOBJ)/$(BUILDSUB)
endif
OBJD:= $(OBJ)/debug
BIN:= $(BASEBIN)/$(BUILDSUB)
ALL_DIRS:= $(BASEOBJ) $(OBJ) $(BASEBIN) $(BIN) $(OBJD) $(OBJT) $(BASESHAREDOBJ) $(OBJB) $(DEPSPRE)
Dummy1783uio42:=$(shell for f in $(ALL_DIRS); do [ -d $$f ] || mkdir -p $$f ; done)

BOOST_SERIALIZATION_SRC_DIR = $(BOOST_DIR)/libs/serialization/src
BOOST_TEST_SRC_DIR = $(BOOST_DIR)/libs/test/src
BOOST_OPTIONS_SRC_DIR = $(BOOST_DIR)/libs/program_options/src
BOOST_FILESYSTEM_SRC_DIR = $(BOOST_DIR)/libs/filesystem/src
BOOST_SYSTEM_SRC_DIR = $(BOOST_DIR)/libs/system/src

#wide char archive streams not supported on cygwin so remove *_w*.cpp
BOOST_SERIALIZATION_SRCS:=$(notdir $(filter-out $(wildcard $(BOOST_SERIALIZATION_SRC_DIR)/*_w*),$(wildcard $(BOOST_SERIALIZATION_SRC_DIR)/*.cpp)))

BOOST_TEST_SRCS=$(filter-out cpp_main.cpp,$(notdir $(wildcard $(BOOST_TEST_SRC_DIR)/*.cpp)))
BOOST_OPTIONS_SRCS=$(filter-out  winmain.cpp,$(notdir $(wildcard $(BOOST_OPTIONS_SRC_DIR)/*.cpp)))
ifdef CPPNOWIDECHAR
BOOST_OPTIONS_SRCS := $(filter-out utf8_codecvt_facet.cpp,$(BOOST_OPTIONS_SRCS))
BOOST_SERIALIZATION_SRCS := $(filter-out utf8_codecvt_facet.cpp,$(BOOST_SERIALIZATION_SRCS))
# problem: multiple codecvt facet in vpath
endif
BOOST_FILESYSTEM_SRCS=$(notdir $(wildcard $(BOOST_FILESYSTEM_SRC_DIR)/*.cpp))
BOOST_SYSTEM_SRCS=$(notdir $(wildcard $(BOOST_SYSTEM_SRC_DIR)/*.cpp))

BOOST_SERIALIZATION_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_SERIALIZATION_SRCS)))
BOOST_TEST_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_TEST_SRCS)))
#BOOST_OPTIONS_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_OPTIONS_SRCS))) $(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_SYSTEM_SRCS)))
BOOST_OPTIONS_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_OPTIONS_SRCS) $(BOOST_SYSTEM_SRCS)))
BOOST_FILESYSTEM_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_FILESYSTEM_SRCS)))

ifndef BOOST_SUFFIX
ifndef BOOST_SUFFIX_BASE
#BOOST_SUFFIX_BASE="gcc"
endif
ifdef BOOST_DEBUG
ifndef BOOST_DEBUG_SUFFIX
BOOST_DEBUG_SUFFIX="-gd"
endif
endif
ifndef NO_THREADS
BOOST_SUFFIX=$(BOOST_SUFFIX_BASE)-mt$(BOOST_DEBUG_SUFFIX)
else
BOOST_SUFFIX=$(BOOST_SUFFIX_BASE)$(BOOST_DEBUG_SUFFIX)
CPPFLAGS +=  -DBOOST_DISABLE_THREADS -DBOOST_NO_MT
endif
endif

ifdef BOOST_SUFFIX
BSUF=-$(BOOST_SUFFIX)
else
BSUF=
endif

ifdef BUILD_OWN_BOOST_LIBS
vpath %.cpp $(BOOST_OPTIONS_SRC_DIR) $(BOOST_SERIALIZATION_SRC_DIR) $(BOOST_TEST_SRC_DIR)  $(BOOST_FILESYSTEM_SRC_DIR) $(BOOST_SYSTEM_SRC_DIR)
BOOST_SERIALIZATION_LIB=$(OBJB)/libserialization.a
BOOST_TEST_LIB=$(OBJB)/libtest.a
BOOST_OPTIONS_LIB=$(OBJB)/libprogram_options.a
BOOST_FILESYSTEM_LIB=$(OBJB)/libfilesystem.a
libs: $(BOOST_TIMER_LIB) $(BOOST_SERIALIZATION_LIB) $(BOOST_TEST_LIB) $(BOOST_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB)
INC += $(BOOST_DIR)
else
BOOST_SERIALIZATION_LIB=-lboost_serialization$(BSUF)
BOOST_TEST_LIB=-lboost_unit_test_framework$(BSUF)
BOOST_SERIALIZATION_LIB=-lboost_serialization$(BSUF)
BOOST_SYSTEM_LIB=-lboost_system$(BSUF)
BOOST_FILESYSTEM_LIB=-lboost_filesystem$(BSUF)
BOOST_RANDOM_LIB=-lboost_random$(BSUF)
BOOST_OPTIONS_LIB=-lboost_program_options$(BSUF) $(BOOST_SYSTEM_LIB)
BOOST_TIMER_LIB=-lboost_timer$(BSUF) $(BOOST_SYSTEM_LIB)
libs:
endif


list_src: $(BOOST_SERIALIZATION_SRCS)
	echo $(BOOST_SERIALIZATION_SRCS)


CXXFLAGS_COMMON += $(ARCH_FLAGS) $(CMDCXXFLAGS)

LARGEFILEFLAGS = -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CPPFLAGS += $(CPPNOWIDECHAR) $(LARGEFILEFLAGS)
ifndef LIBDIR
ifdef FIRST_PREFIX
LIBDIR = $(FIRST_PREFIX)/lib
endif
endif
# major-mode
LDFLAGS += $(addprefix -l,$(LIB)) -L$(OBJB) $(ARCH_FLAGS) $(addprefix -L,$(LIBDIR)) -lpthread -pthread -lz
LDFLAGS_TEST = $(LDFLAGS)  -ltest
INC += $(TRUNK)
INC += ..
#INC += ../..
CPPFLAGS := $(addprefix -I,$(INC)) $(addprefix -D,$(DEFS)) $(CPPFLAGS)
ifdef PEDANTIC
CPPFLAGS +=  -pedantic
endif
CPPFLAGS_TEST += $(CPPFLAGS) -ggdb
CPPFLAGS_DEBUG += $(CPPFLAGS) -fno-inline-functions -ggdb
CPPFLAGS_OPT += $(CPPFLAGS) -ggdb
#-DNDEBUG

#-fno-var-tracking
# somehow that is getting automatically set by boost now for gcc 3.4.1 (detecting that -lthread is not used? dunno)


ifeq ($(ARCH),solaris)
  CPPFLAGS += -DSOLARIS -DUSE_STD_RAND
#  DBOOST_PLATFORM_CONFIG=<boost/config/solaris.hpp>
endif

ifeq ($(ARCH),linux)
 CPPFLAGS +=  -DLINUX
 #-DLINUX_BACKTRACE -rdynamic
#-rdynamic: forces global symbol table (could remove for optimized build)
endif


define PROG_template

.PHONY += $(1)

ifndef $(1)_SRC_TEST
$(1)_SRC_TEST=$$($(1)_SRC)
else
ALL_SRCS_REL+=$$($(1)_SRC_TEST)
endif
ALL_SRCS_REL += $$($(1)_SRC)

$(1)_OBJ=$$(addsuffix .o,$$($(1)_SRC))
$(1)_OBJ_TEST=$$(addsuffix .o,$$($(1)_SRC_TEST))

ifndef $(1)_NOOPT
$$(BIN)/$(1)$(PROGSUFFIX)$(REVSUFFIX)$(CYGEXE):\
 $$(addprefix $$(OBJ)/,$$($(1)_OBJ))\
 $$($(1)_SLIB)
	@echo
	@echo LINK\(optimized\) $$@ - from $$^
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) $$($(1)_LIB) $$($(1)_BOOSTLIB)
ALL_OBJS   += $$(addprefix $$(OBJ)/,$$($(1)_OBJ))
OPT_PROGS += $$(BIN)/$(1)$(PROGSUFFIX)$(REVSUFFIX)$(CYGEXE)
$(1): $$(BIN)/$(1)
endif

ifneq (${ARCH},macosx) # avoid error:ld: library not found for -lcrt0.o
ifndef NOSTATIC
ifndef $(1)_NOSTATIC
$$(BIN)/$(1)$(PROGSUFFIX).static$(REVSUFFIX): $$(addprefix $$(OBJ)/,$$($(1)_OBJ)) $$($(1)_SLIB)
	@echo
	@echo LINK\(static\) $$@ - from $$^
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) $$($(1)_LIB) $$($(1)_BOOSTLIB) --static
ALL_OBJS   += $$(addprefix $$(OBJ)/,$$($(1)_OBJ))
STATIC_PROGS += $$(BIN)/$(1)$(PROGSUFFIX).static$(REVSUFFIX)
$(1): $$(BIN)/$(1).static
endif
endif
endif

ifndef $(1)_NODEBUG
$$(BIN)/$(1)$(PROGSUFFIX).debug$(REVSUFFIX):\
 $$(addprefix $$(OBJD)/,$$($(1)_OBJ)) $$($(1)_SLIB)
	@echo
	@echo LINK\(debug\) $$@ - from $$^
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) $$($(1)_LIB) $$(addsuffix -d,$$($(1)_BOOSTLIB))
ALL_OBJS +=  $$(addprefix $$(OBJD)/,$$($(1)_OBJ))
DEBUG_PROGS += $$(BIN)/$(1)$(PROGSUFFIX).debug$(REVSUFFIX)
$(1): $$(BIN)/$(1).debug
endif

ifndef $(1)_NOTEST
#$$(BOOST_TEST_LIB)
$$(BIN)/$(1)$(PROGSUFFIX).test$(REVSUFFIX): $$(addprefix $$(OBJT)/,$$($(1)_OBJ_TEST))  $$($(1)_SLIB)
	@echo
	@echo LINK\(test\) $$@ - from $$^
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) $$($(1)_LIB) $$(BOOST_TEST_LIB)
#	$$@ --catch_system_errors=no
ALL_OBJS += $$(addprefix $$(OBJT)/,$$($(1)_OBJ_TEST))
ALL_TESTS += $$(BIN)/$(1)$(PROGSUFFIX).test$(REVSUFFIX)
TEST_PROGS += $$(BIN)/$(1)$(PROGSUFFIX).test$(REVSUFFIX)
$(1): $$(BIN)/$(1).test
endif

#$(1): $(addprefix $$(BIN)/, $(1) $(1).debug $(1).test)

endef

.PRECIOUS: %/.
%/.:
	mkdir -p $@

$(foreach prog,$(PROGS),$(eval $(call PROG_template,$(prog))))


ALL_PROGS=$(OPT_PROGS) $(DEBUG_PROGS) $(TEST_PROGS) $(STATIC_PROGS)

all: $(ALL_DEPENDS) $(ALL_PROGS)

opt: $(OPT_PROGS)

debug: $(DEBUG_PROGS)

ALL_DEPENDS=$(addprefix $(DEPSPRE),$(addsuffix .d,$(ALL_SRCS_REL)))

redepend:
	rm -f $(ALL_DEPENDS)
	make depend

depend: $(ALL_DEPENDS)


install: $(OPT_PROGS) $(STATIC_PROGS) $(DEBUG_PROGS) $(CP_PROGS)
	mkdir -p $(BIN_PREFIX) ; cp -f $(STATIC_PROGS) $(DEBUG_PROGS) $(OPT_PROGS) $(CP_PROGS) $(BIN_PREFIX)

check:	test


test: $(ALL_TESTS)
	for test in $(ALL_TESTS) ; do echo Running test: $$test; $$test --catch_system_errors=no ; done
#	$(foreach test,$(ALL_TESTS),$(shell $(test) --catch_system_errors=no))


ifdef BUILD_OWN_BOOST_LIBS
$(BOOST_FILESYSTEM_LIB): $(BOOST_FILESYSTEM_OBJS)
	@echo
	@echo creating Boost Filesystem lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@

$(BOOST_TEST_LIB): $(BOOST_TEST_OBJS)
	@echo
	@echo creating Boost Test lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@

$(BOOST_OPTIONS_LIB): $(BOOST_OPTIONS_OBJS)
	@echo
	@echo creating Boost Program Options lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@

$(BOOST_SERIALIZATION_LIB): $(BOOST_SERIALIZATION_OBJS)
	@echo
	@echo creating Boost Program Serialization lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@
endif

#vpath %.cpp $(BOOST_SERIALIZATION_SRC_DIR) $(BOOST_TEST_SRC_DIR) $(BOOST_OPTIONS_SRC_DIR) $(BOOST_FILESYSTEM_SRC_DIR)
vpath %.d $(DEPSPRE)
#vpath %.hpp $(BOOST_DIR)

#:$(SHARED):.
.PRECIOUS: $(OBJB)/%.o
$(OBJB)/%.o: %
	@echo
	@echo COMPILE\(boost\) $< into $@
	$(CXX) -c $(CPPFLAGS)  $(CXXFLAGS_COMMON) $(CXXFLAGS) $< -o $@

.PRECIOUS: $(OBJT)/%.o
$(OBJT)/%.o: % %.d
	@echo
	@echo COMPILE\(test\) $< into $@
	$(CXX) -c $(CPPFLAGS_TEST) $(CXXFLAGS_COMMON) $(CXXFLAGS_TEST)  $< -o $@

.PRECIOUS: $(OBJ)/%.o
$(OBJ)/%.o: % %.d
	@echo
	@echo COMPILE\(optimized\) $< into $@
	$(CXX) -c $(CPPFLAGS_OPT) $(CXXFLAGS_COMMON) $(CXXFLAGS)  $< -o $@

.PRECIOUS: $(OBJD)/%.o
$(OBJD)/%.o: % %.d
	@echo
	@echo COMPILE\(debug\) $< into $@
	$(CXX) -c $(CPPFLAGS_DEBUG) $(CXXFLAGS_COMMON) $(CXXFLAGS_DEBUG)  $< -o $@

#dirs:
# $(addsuffix /.,$(ALL_DIRS))
#	echo dirs: $^

clean:
	-rm -rf -- $(ALL_OBJS) $(ALL_CLEAN) *.core *.stackdump

distclean: clean
	-rm -rf -- $(ALL_DEPENDS) $(BOOST_TEST_OBJS) $(BOOST_OPTIONS_OBJS) msvc++/Debug msvc++/Release

allclean: distclean
	-rm -rf -- $(BASEOBJ)* $(BASEBIN) $(BASESHAREDOBJ)

$(BIN)/text-to-cc: text-to-cc.cpp
	g++ $(LDFLAGS) $(CXXFLAGS) $< -o $@

ifeq ($(MAKECMDGOALS),depend)
DEPEND=1
endif


#                 sed 's/\($*\)\.o[ :]*/$@ : /g' $@.raw > $@ && sed 's/\($*\)\.o[ :]*/\n\%\/\1.o : /g' $@.raw >> $@ \
#sed 's/\($*\)\.o[ :]*/DEPS_$@ := /g' $@.raw > $@ && echo $(basename $<).o : \\\$DEPS_$(basename $<) >> $@ \

#phony: -MP
$(DEPSPRE)%.d: %
	@set -e; \
	if [ x$(DEPEND) != x -o ! -f $@ ] ; then \
 ( \
echo CREATE DEPENDENCIES for $< \(object=$(*F).o\) && \
		$(CXX) -c -MP -MM -MG $(CPPFLAGS_DEBUG) $< -MF $@.raw && \
		[ -s $@.raw ] && \
                 perl -pe 's|([^:]*)\.o[ :]*|$@ : |g' $@.raw > $@ && \
echo >> $@ && \
perl -pe 's|([^:]*)\.o[ :]*|$(OBJ)/$(*F).o : |g' $@.raw >>$@  && \
echo >> $@ && \
perl -pe 's|([^:]*)\.o[ :]*|$(OBJD)/$(*F).o : |g' $@.raw >> $@ && \
echo >> $@ && \
perl -pe 's|([^:]*)\.o[ :]*|$(OBJT)/$(*F).o : |g' $@.raw >> $@  \
 || rm -f $@ ); rm -f $@.raw ; fi
#; else touch $@

ifneq ($(MAKECMDGOALS),depend)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
include $(ALL_DEPENDS)
endif
endif
endif
