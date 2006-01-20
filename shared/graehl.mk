#you provide:  
# (the variables below)
# ARCH (if macosx, static builds are blocked)
#PROGS=a b   
#a_OBJ=a.o ... a_SLIB=lib.o lib.a (static libraries e.g. a_SLIB=$(BOOST_OPT_LIB))
#a_NOSTATIC=1 a_NOTEST=1 ...
#NOSTATIC=1 (global setting)
# CXXFLAGS CXXFLAGS_DEBUG CXXFLAGS_TEST
# LIB = math thread ...
# INC = . 
###WARNING: don't set BASEOBJ BASESHAREDOBJ or BASEBIN to directories including other important stuff or they will be nuked by make allclean

#include $(SHARED)/debugger.mk
#$(__BREAKPOINT)

LIB += z
CXXFLAGS += $(CMDCXXFLAGS)
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

DEPSPRE=deps/$(ARCH)/

ifeq ($(ARCH),linux64)
ARCH_FLAGS = -march=athlon64
else
 ifeq ($(ARCH),linux)
  ARCH_FLAGS = -m32 -march=pentium4
 endif
endif

ifndef INSTALL_PREFIX
INSTALL_PREFIX=$(HOME)/isd/$(ARCH)
endif
ifndef BIN_PREFIX
BIN_PREFIX=$(INSTALL_PREFIX)/bin
endif

ifndef SHARED
SHARED=../shared
endif
ifndef BOOST_DIR
BOOST_DIR=../boost
endif
ifndef BASEOBJ
BASEOBJ=obj
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

# workaround for eval line length limit: immediate substitution shorter?
OBJ:= $(BASEOBJ)/$(ARCH)
OBJT:= $(OBJ)/test
ifndef OBJB
OBJB:= $(BASESHAREDOBJ)/$(ARCH)
endif
OBJD:= $(OBJ)/debug
BIN:= $(BASEBIN)/$(ARCH)
ALL_DIRS:= $(BASEOBJ) $(OBJ) $(BASEBIN) $(BIN) $(OBJD) $(OBJT) $(BASESHAREDOBJ) $(OBJB) $(DEPSPRE)
Dummy1783uio42:=$(shell for f in $(ALL_DIRS); do [ -d $$f ] || mkdir -p $$f ; done)

BOOST_SERIALIZATION_SRC_DIR = $(BOOST_DIR)/libs/serialization/src
BOOST_TEST_SRC_DIR = $(BOOST_DIR)/libs/test/src
BOOST_OPT_SRC_DIR = $(BOOST_DIR)/libs/program_options/src
BOOST_FS_SRC_DIR = $(BOOST_DIR)/libs/filesystem/src

#wide char archive streams not supported on cygwin so remove *_w*.cpp
BOOST_SERIALIZATION_SRCS=$(filter-out utf8_codecvt_facet.cpp,$(notdir $(filter-out $(wildcard $(BOOST_SERIALIZATION_SRC_DIR)/*_w*),$(wildcard $(BOOST_SERIALIZATION_SRC_DIR)/*.cpp))))
BOOST_TEST_SRCS=$(filter-out cpp_main.cpp,$(notdir $(wildcard $(BOOST_TEST_SRC_DIR)/*.cpp)))
BOOST_OPT_SRCS=$(filter-out winmain.cpp,$(notdir $(wildcard $(BOOST_OPT_SRC_DIR)/*.cpp)))
BOOST_FS_SRCS=$(notdir $(wildcard $(BOOST_FS_SRC_DIR)/*.cpp))

BOOST_SERIALIZATION_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_SERIALIZATION_SRCS)))
BOOST_TEST_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_TEST_SRCS)))
BOOST_OPT_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_OPT_SRCS)))
BOOST_FS_OBJS=$(addprefix $(OBJB)/,$(addsuffix .o,$(BOOST_FS_SRCS)))

BOOST_SERIALIZATION_LIB=$(OBJB)/libserialization.a
BOOST_TEST_LIB=$(OBJB)/libtest.a
BOOST_OPT_LIB=$(OBJB)/libprogram_options.a
BOOST_FS_LIB=$(OBJB)/libfilesystem.a

libs: $(BOOST_SERIALIZATION_LIB) $(BOOST_TEST_LIB) $(BOOST_OPT_LIB) $(BOOST_FS_LIB)

CXXFLAGS_COMMON += $(ARCH_FLAGS)
CPPNOWIDECHAR = $(addprefix -D,BOOST_NO_CWCHAR BOOST_NO_CWCTYPE BOOST_NO_STD_WSTRING BOOST_NO_STD_WSTREAMBUF)

#CPPFLAGS += $(CPPNOWIDECHAR)
LDFLAGS += $(addprefix -l,$(LIB)) -L$(OBJB) $(ARCH_FLAGS) $(addprefix -L,$(LIBDIR))
#-lpthread
LDFLAGS_TEST = $(LDFLAGS)  -ltest
CPPFLAGS += $(addprefix -I,$(INC)) -I$(BOOST_DIR) -DBOOST_NO_MT
ifdef PEDANTIC
CPPFLAGS +=  -pedantic
endif
CPPFLAGS_TEST += $(CPPFLAGS)
CPPFLAGS_DEBUG += $(CPPFLAGS) -fno-inline-functions -ggdb
#-fno-var-tracking
# somehow that is getting automatically set by boost now for gcc 3.4.1 (detecting that -lthread is not used? dunno)

ifndef USE_THREADS
CPPFLAGS +=  -DBOOST_DISABLE_THREADS 
endif

ifeq ($(ARCH),solaris)
  CPPFLAGS += -DSOLARIS -DUSE_STD_RAND
#  DBOOST_PLATFORM_CONFIG=<boost/config/solaris.hpp>
endif

ifeq ($(ARCH),linux)
 CPPFLAGS += -DLINUX_BACKTRACE -DLINUX  -rdynamic
#-rdynamic: forces global symbol table (could remove for optimized build)
endif

ifeq ($(ARCH),cygwin)
NOSTATIC=1
CPPFLAGS += -DBOOST_POSIX -DCYGWIN
#CPPFLAGS += -DBOOST_NO_STD_WSTRING
# somehow that is getting automatically set by boost now (for Boost CVS)
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
$$(BIN)/$(1):\
 $$(addprefix $$(OBJ)/,$$($(1)_OBJ))\
 $$($(1)_SLIB)
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) 
ALL_OBJS   += $$(addprefix $$(OBJ)/,$$($(1)_OBJ))
OPT_PROGS += $$(BIN)/$(1)
$(1): $$(BIN)/$(1)
endif

ifneq (${ARCH},macosx)
ifndef NOSTATIC
ifndef $(1)_NOSTATIC
$$(BIN)/$(1).static: $$(addprefix $$(OBJ)/,$$($(1)_OBJ)) $$($(1)_SLIB)
	@echo
	@echo LINK\(static\) $$< into $$@
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) --static 
ALL_OBJS   += $$(addprefix $$(OBJ)/,$$($(1)_OBJ))
STATIC_PROGS += $$(BIN)/$(1).static
$(1): $$(BIN)/$(1).static
endif
endif
endif

ifndef $(1)_NODEBUG
$$(BIN)/$(1).debug:\
 $$(addprefix $$(OBJD)/,$$($(1)_OBJ)) $$($(1)_SLIB)
	@echo
	@echo LINK\(debug\) $$< into $$@
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) 
ALL_OBJS +=  $$(addprefix $$(OBJD)/,$$($(1)_OBJ)) 
DEBUG_PROGS += $$(BIN)/$(1).debug
$(1): $$(BIN)/$(1).debug
endif

ifndef $(1)_NOTEST
$$(BIN)/$(1).test: $$(addprefix $$(OBJT)/,$$($(1)_OBJ_TEST)) $$(BOOST_TEST_LIB)  $$($(1)_SLIB)
	@echo
	@echo LINK\(test\) $$< into $$@
	$$(CXX) $$^ -o $$@ $$(LDFLAGS) 
#	$$@ --catch_system_errors=no
ALL_OBJS += $$(addprefix $$(OBJT)/,$$($(1)_OBJ_TEST))
ALL_TESTS += $$(BIN)/$(1).test
TEST_PROGS += $$(BIN)/$(1).test
$(1): $$(BIN)/$(1).test
endif

#$(1): $(addprefix $$(BIN)/, $(1) $(1).debug $(1).test)

endef

.PRECIOUS: %/.
%/.:
	mkdir -p $@

$(foreach prog,$(PROGS),$(eval $(call PROG_template,$(prog))))


ALL_PROGS=$(OPT_PROGS) $(DEBUG_PROGS) $(TEST_PROGS) $(STATIC_PROGS)

all: $(ALL_PROGS)

opt: $(OPT_PROGS)

debug: $(DEBUG_PROGS)

ALL_DEPENDS=$(addprefix $(DEPSPRE),$(addsuffix .d,$(ALL_SRCS_REL)))

redepend:
	rm -f $(ALL_DEPENDS)
	make depend

depend: $(ALL_DEPENDS)

ifeq ($(ARCH),cygwin)
CYGEXE=.exe
else
CYGEXE=
endif
install: $(OPT_PROGS) $(STATIC_PROGS) $(DEBUG_PROGS)
	mkdir -p $(BIN_PREFIX) ; cp $(STATIC_PROGS) $(DEBUG_PROGS) $(addsuffix $(CYGEXE), $(OPT_PROGS)) $(BIN_PREFIX)

test: $(ALL_TESTS)
	for test in $(ALL_TESTS) ; do echo Running test: $$test; $$test --catch_system_errors=no ; done
#	$(foreach test,$(ALL_TESTS),$(shell $(test) --catch_system_errors=no))


$(BOOST_FS_LIB): $(BOOST_FS_OBJS)
	@echo
	@echo creating Boost Filesystem lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@

$(BOOST_TEST_LIB): $(BOOST_TEST_OBJS)
	@echo
	@echo creating Boost Test lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@

$(BOOST_OPT_LIB): $(BOOST_OPT_OBJS)
	@echo
	@echo creating Boost Program Options lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@

$(BOOST_SERIALIZATION_LIB): $(BOOST_SERIALIZATION_OBJS)
	@echo
	@echo creating Boost Program Serialization lib
	$(AR) -rc $@ $^
#	$(RANLIB) $@

vpath %.cpp $(BOOST_SERIALIZATION_SRC_DIR) $(BOOST_TEST_SRC_DIR) $(BOOST_OPT_SRC_DIR) $(BOOST_FS_SRC_DIR)
vpath %.d $(DEPSPRE)
vpath %.hpp $(BOOST_DIR)

#:$(SHARED):.
.PRECIOUS: $(OBJB)/%.o
$(OBJB)/%.o: %
	@echo
	@echo COMPILE\(boost\) $< into $@
	$(CXX) -c $(CXXFLAGS_COMMON) $(CXXFLAGS) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJT)/%.o
$(OBJT)/%.o: % %.d
	@echo
	@echo COMPILE\(test\) $< into $@
	$(CXX) -c $(CXXFLAGS_COMMON) $(CXXFLAGS_TEST) $(CPPFLAGS_TEST) $< -o $@

.PRECIOUS: $(OBJ)/%.o
$(OBJ)/%.o: % %.d
	@echo
	@echo COMPILE\(optimized\) $< into $@
	$(CXX) -c $(CXXFLAGS_COMMON) $(CXXFLAGS) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJD)/%.o
$(OBJD)/%.o: % %.d
	@echo
	@echo COMPILE\(debug\) $< into $@
	$(CXX) -c $(CXXFLAGS_COMMON) $(CXXFLAGS_DEBUG) $(CPPFLAGS_DEBUG) $< -o $@

#dirs:
# $(addsuffix /.,$(ALL_DIRS))
#	echo dirs: $^

clean:
	-rm -rf -- $(ALL_OBJS) $(ALL_CLEAN) *.core *.stackdump

distclean: clean
	-rm -rf -- $(ALL_DEPENDS) $(BOOST_TEST_OBJS) $(BOOST_OPT_OBJS) msvc++/Debug msvc++/Release

allclean: distclean
	-rm -rf -- $(BASEOBJ)* $(BASEBIN) $(BASESHAREDOBJ)

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
