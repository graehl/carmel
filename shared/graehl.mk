BOOST_DIR=../boost

.SUFFIXES:
.PHONY = distclean all clean depend default dirs
# always execute
ifndef ARCH
  ARCH := $(shell print_arch)
  export ARCH
endif

###WARNING: don't set BASEOBJ or BASEBIN to directories including other important stuff or they will be nuked by allclean
BASEOBJ=obj
BASEBIN=bin
OBJ = $(BASEOBJ)/$(ARCH)
OBJ_TEST = $(OBJ)/test
OBJ_BOOST = $(OBJ)/boost
OBJ_DEBUG = $(OBJ)/debug
BIN = $(BASEBIN)/$(ARCH)
ALL_DIRS = $(OBJ) $(BIN) $(OBJ_DEBUG) $(OBJ_TEST) $(OBJ_BOOST)

ifeq ($(CPP_EXT),)
CPP_EXT=cpp
endif

BOOST_TEST_SRCS=test_tools.cpp unit_test_parameters.cpp execution_monitor.cpp \
unit_test_log.cpp unit_test_result.cpp supplied_log_formatters.cpp	      \
unit_test_main.cpp unit_test_suite.cpp unit_test_monitor.cpp
BOOST_TEST_OBJS=$(BOOST_TEST_SRCS:%.cpp=$(OBJ_BOOST)/%.o)
BOOST_OPT_SRCS=cmdline.cpp convert.cpp parsers.cpp utf8_codecvt_facet.cpp variables_map.cpp config_file.cpp options_description.cpp positional_options.cpp value_semantic.cpp
# winmain.cpp
BOOST_OPT_OBJS=$(BOOST_OPT_SRCS:%.cpp=$(OBJ_BOOST)/%.o)
BOOST_TEST_LIB=$(OBJ_BOOST)/libtest.a
BOOST_OPT_LIB=$(OBJ_BOOST)/libprogram_options.a
BOOST_TEST_SRC_DIR = $(BOOST_DIR)/libs/test/src
BOOST_OPT_SRC_DIR = $(BOOST_DIR)/libs/program_options/src
LDFLAGS += $(addprefix -l,$(LIB))
LDFLAGS_TEST = $(LDFLAGS) -L$(OBJ_BOOST) -ltest
CPPFLAGS += $(addprefix -I,$(INC)) -I$(BOOST_DIR) -DBOOST_DISABLE_THREADS -DBOOST_NO_MT
# somehow that is getting automatically set by boost now for gcc 3.4.1 (detecting that -lthread is not used? dunno)

ifeq ($(ARCH),cygwin)
CPPFLAGS += -DBOOST_NO_STD_WSTRING
# somehow that is getting automatically set by boost now (for Boost CVS)
endif


define PROG_template

.PHONY += $(1)

ifndef $(1)_NOOPT
$$(BIN)/$(1): $$(addprefix $$(OBJ)/,$$($(1)_OBJ)) $$($(1)_STATICLIB)
	$$(CXX) $$(LDFLAGS) $$^ -o $$@
ALL_OBJS   += $$(addprefix $$(OBJ)/,$$($(1)_OBJ))
OPT_PROGS += $$(BIN)/$(1)
$(1): $$(BIN)/$(1)
endif

ifndef $(1)_NOSTATIC
$$(BIN)/$(1).static: $$(addprefix $$(OBJ)/,$$($(1)_OBJ)) $$($(1)_STATICLIB)
	$$(CXX) $$(LDFLAGS) --static $$^ -o $$@
ALL_OBJS   += $$(addprefix $$(OBJ)/,$$($(1)_OBJ))
STATIC_PROGS += $$(BIN)/$(1).static
$(1): $$(BIN)/$(1).static
endif

ifndef $(1)_NODEBUG
$$(BIN)/$(1).debug: $$(addprefix $$(OBJ_DEBUG)/,$$($(1)_OBJ)) $$($(1)_STATICLIB)
	$$(CXX) $$(LDFLAGS) $$^ -o $$@
ALL_OBJS +=  $$(addprefix $$(OBJ_DEBUG)/,$$($(1)_OBJ)) 
DEBUG_PROGS += $$(BIN)/$(1).debug
$(1): $$(BIN)/$(1).debug
endif

ifndef $(1)_NOTEST
$$(BIN)/$(1).test: $$(addprefix $$(OBJ_TEST)/,$$($(1)_OBJ)) $$(BOOST_TEST_LIB)
	$$(CXX) $$(LDFLAGS) $$^ -o $$@
#	$$@ --catch_system_errors=no
ALL_OBJS += $$(addprefix $$(OBJ_TEST)/,$$($(1)_OBJ))
ALL_TESTS += $$(BIN)/$(1).test
TEST_PROGS += $$(BIN)/$(1).test
$(1): $$(BIN)/$(1).test
endif

#$(1): $(addprefix $$(BIN)/, $(1) $(1).debug $(1).test)

ALL_DEPENDS += $$($(1)_OBJ:%.o=%.d)

endef

.PRECIOUS: %/.
%/.:
	mkdir -p $(@)

$(foreach prog,$(PROGS),$(eval $(call PROG_template,$(prog))))

ALL_PROGS=$(OPT_PROGS) $(DEBUG_PROGS) $(TEST_PROGS) $(STATIC_PROGS)

all: dirs $(ALL_PROGS)

debug: dirs $(OPT_PROGS)

opt: dirs $(DEBUG_PROGS)

depend: dirs $(ALL_DEPENDS)

test: $(ALL_TESTS)
	for test in $(ALL_TESTS) ; do echo Running test: $$test; $$test --catch_system_errors=no ; done
#	$(foreach test,$(ALL_TESTS),$(shell $(test) --catch_system_errors=no))


$(BOOST_TEST_LIB): $(BOOST_TEST_OBJS)
	@echo
	@echo creating Boost Test lib
	ar cr $@ $^
	ranlib $@

$(BOOST_OPT_LIB): $(BOOST_OPT_OBJS)
	@echo
	@echo creating Boost Program Options lib
	ar cr $@ $^
	ranlib $@

vpath %.cpp $(BOOST_TEST_SRC_DIR):$(BOOST_OPT_SRC_DIR)
#:$(SHARED):.
.PRECIOUS: $(OBJ_BOOST)/%.o
$(OBJ_BOOST)/%.o:: %.cpp
	@echo
	@echo COMPILE\(boost\) $< into $@
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJ_TEST)/%.o
$(OBJ_TEST)/%.o:: %.$(CPP_EXT) %.d
	@echo
	@echo COMPILE\(test\) $< into $@
	$(CXX) -c $(CXXFLAGS_TEST) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJ)/%.o
$(OBJ)/%.o:: %.$(CPP_EXT) %.d
	@echo
	@echo COMPILE\(optimized\) $< into $@
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJ_DEBUG)/%.o
$(OBJ_DEBUG)/%.o:: %.$(CPP_EXT) %.d
	@echo
	@echo COMPILE\(debug\) $< into $@
	$(CXX) -c $(CXXFLAGS_DEBUG) $(CPPFLAGS) $< -o $@

dirs: $(addsufix /.,$(ALL_DIRS))

clean:
	rm -f $(ALL_OBJS) $(ALL_CLEAN)

distclean: clean
	rm -f $(ALL_DEPENDS) $(BOOST_TEST_OBJS) $(BOOST_OPT_OBJS)

allclean:
	rm -rf $(BASEOBJ) $(BASEBIN) $(ALL_DEPENDS)

virgin: clean distclean
	rm -f $(ALL_PROGS)

ifeq ($(MAKECMDGOALS),depend)
DEPEND=1
endif

%.d: %.$(CPP_EXT)
#	@echo
#	@echo CREATE DEPENDENCIES for $<
	@set -e; [ x$(DEPEND) != x -o ! -f $@ ] && echo CREATE DEPENDENCIES for $< && $(CXX) -c -MM -MG -MP $(TESTCXXFLAGS) $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

ifneq ($(MAKECMDGOALS),depend)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
include $(ALL_DEPENDS)
endif
endif
endif
