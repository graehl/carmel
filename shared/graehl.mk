#.SUFFIXES:
.PHONY = distclean all clean depend default dirs
# always execute
ifndef ARCH
  ARCH := $(shell print_arch)
  export ARCH
endif

BASEOBJ=obj
BASEBIN=bin
OBJ = $(BASEOBJ)/$(ARCH)
OBJ_TEST = $(OBJ)/test
OBJ_BOOST = $(OBJ)/boost
OBJ_DEBUG = $(OBJ)/debug
BIN = $(BASEBIN)/$(ARCH)

BOOST_TEST_SRCS=test_tools.cpp unit_test_parameters.cpp execution_monitor.cpp  unit_test_log.cpp unit_test_result.cpp supplied_log_formatters.cpp unit_test_main.cpp unit_test_suite.cpp unit_test_monitor.cpp
BOOST_TEST_OBJS=$(BOOST_TEST_SRCS:%.cpp=$(OBJ_BOOST)/%.o)
BOOST_TEST_LIB=$(OBJ_BOOST)/libtest.a
BOOST_DIR=/home/graehl/isd/boost
BOOST_TEST_SRC_DIR = $(BOOST_DIR)/libs/test/src
LDFLAGS += $(addprefix -l,$(LIB))
LDFLAGS_TEST = $(LDFLAGS) -L$(OBJ_BOOST) -ltest
CPPFLAGS += $(addprefix -I,$(INC)) -I- -I$(BOOST_DIR) -DBOOST_DISABLE_THREADS -DBOOST_NO_MT

$(BOOST_TEST_LIB): $(BOOST_TEST_OBJS)
	ar cr $@ $^
	ranlib $@


define PROG_template

$$(BIN)/$(1): $$(addprefix $$(OBJ)/,$$($(1)_OBJ))
	$$(CXX) $$(LDFLAGS) $$^ -o $$@

$$(BIN)/$(1).test: $$(addprefix $$(OBJ_TEST)/,$$($(1)_OBJ)) $$(BOOST_TEST_LIB)
	$$(CXX) $$(LDFLAGS) $$^ -o $$@
	$$@ --catch_system_errors=no

$$(BIN)/$(1).debug: $$(addprefix $$(OBJ_DEBUG)/,$$($(1)_OBJ))
	$$(CXX) $$(LDFLAGS) $$^ -o $$@

ALL_OBJS   += $$(addprefix $$(OBJ)/,$$($(1)_OBJ)) $$(addprefix $$(OBJ_DEBUG)/,$$($(1)_OBJ)) $$(addprefix $$(OBJ_TEST)/,$$($(1)_OBJ))
ALL_PROGS  += $(addprefix $$(BIN)/, $(1) $(1).debug $(1).test)
ALL_TESTS += $$(BIN)/$(1).test
ALL_DEPENDS += $$($(1)_OBJ:%.o=%.d)

endef

$(foreach prog,$(PROGS),$(eval $(call PROG_template,$(prog))))

all: dirs $(ALL_PROGS)

depend: $(ALL_DEPENDS)

test: $(ALL_TESTS)

.PRECIOUS: %/.
%/.:
	mkdir -p $(@)

.PRECIOUS: $(OBJ_BOOST)/%.o
$(OBJ_BOOST)/%.o:: $(BOOST_TEST_SRC_DIR)/%.cpp
	echo COMPILE boost $< into $@
	$(CXX) -c $(CXXFLAGS_TEST) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJ_TEST)/%.o
$(OBJ_TEST)/%.o:: %.cpp %.d
	echo COMPILE test $< into $@
	$(CXX) -c $(CXXFLAGS_TEST) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJ)/%.o
$(OBJ)/%.o:: %.cpp %.d
	echo COMPILE $< into $@
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

.PRECIOUS: $(OBJ_DEBUG)/%.o
$(OBJ_DEBUG)/%.o:: %.cpp %.d
	echo COMPILE debug $< into $@
	$(CXX) -c $(CXXFLAGS_DEBUG) $(CPPFLAGS) $< -o $@

dirs: $(BIN)/. $(OBJ_DEBUG)/. $(OBJ_TEST)/. $(OBJ_BOOST)/. $(OBJ)/.

clean:
	rm -f $(ALL_OBJS)
	rm -f $(ALL_PROGS)

distclean: clean
	rm -f $(ALL_DEPENDS)


ifeq ($(MAKECMDGOALS),depend)
DEPEND=1
%.d: %.cpp
	echo CREATE DEPENDENCIES for $<
	set -e; [ x$(DEPEND) != x ] && $(CXX) -c -MM $(TESTCXXFLAGS) $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@
endif


ifneq ($(MAKECMDGOALS),depend)
include $(ALL_DEPENDS)
endif
