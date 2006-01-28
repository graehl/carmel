include $(SHARED)/gmsl

__LOOP := 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32

__PROMPT = $(shell read -p "$(__HISTORY)> " CMD ARG ; echo $$CMD $$ARG)

__DEBUG =  $(eval __c = $(strip $1))                                         \
           $(eval __a = $(strip $2))                                         \
           $(if $(call seq,$(__c),c),                                        \
              $(true),                                                       \
              $(if $(call seq,$(__c),q),                                     \
                 $(error Debugger terminated build),                         \
                 $(if $(call seq,$(__c),v),                                  \
                    $(warning $(__a) has value '$($(__a))'),                 \
                    $(if $(call seq,$(__c),d),                               \
                       $(warning $(__a) is defined as '$(value $(__a))'),    \
                       $(if $(call seq,$(__c),o),                            \
                          $(warning $(__a) came from $(origin $(__a))),      \
                          $(if $(call seq,$(__c),h),                         \
                             $(warning c       continue)                     \
                             $(warning q       quit)                         \
                             $(warning v VAR   print value of $$(VAR))       \
                             $(warning o VAR   print origin of $$(VAR))      \
                             $(warning d VAR   print definition of $$(VAR)), \
                             $(warning Unknown command '$(__c)')))))))

__BREAK = $(eval __INPUT := $(__PROMPT))                                     \
          $(call __DEBUG,                                                    \
              $(word 1,$(__INPUT)),                                          \
              $(word 2,$(__INPUT)))

__BANNER = $(warning GNU Make Debugger Break)                                \
           $(if $^,                                                          \
              $(warning - Building '$@' from '$^'),                          \
              $(warning - Building '$@'))                                    \
           $(if $<,$(warning - First prerequisite is '$<'))                  \
           $(if $%,$(warning - Archive target is '$%'))                      \
           $(if $?,$(warning - Prequisites '$?' are newer than '$@'))

__BREAKPOINT = $(__BANNER)                                                   \
               $(eval __TERMINATE := $(false))                               \
               $(foreach __HISTORY,                                          \
                   $(__LOOP),                                                \
                   $(if $(__TERMINATE),,                                     \
                      $(eval __TERMINATE := $(__BREAK))))
