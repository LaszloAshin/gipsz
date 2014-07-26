.PHONY: all
all: | print-flags

.PHONY: print-flags
print-flags:
	@echo "===="
	@echo "  CPPFLAGS := $(CPPFLAGS)"
	@echo "    CFLAGS := $(CFLAGS)"
	@echo "  CXXFLAGS := $(CXXFLAGS)"
	@echo "   LDFLAGS := $(LDFLAGS)"
	@echo "    LDLIBS := $(LDLIBS)"
	@echo " LOADLIBES := $(LOADLIBES)"
	@echo "===="

#ifndef V
#Q := @
#endif

define compile-c-steps
	$(if $Q,@echo "  CC    $(subst $(abspath $Od/)/,,$(abspath $@))")
	$Qmkdir -p $(@D)
	$Q$(CC) $(CPPFLAGS) $(CFLAGS) -c $< $(OUTPUT_OPTION)
endef

define compile-cxx-steps
	$(if $Q,@echo "  CXX   $(subst $(abspath $Od/)/,,$(abspath $@))")
	$Qmkdir -p $(@D)
	$Q$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< $(OUTPUT_OPTION)
endef

define link-executable-steps
	$(if $Q,@echo "  LINK  $(subst $(abspath $O)/,,$(abspath $@))")
	$Qmkdir -p $(@D)
	$Q$(CXX) $(LDFLAGS) $^ $(LDLIBS) $(LOADLIBES) $(OUTPUT_OPTION)
endef

$Od/%.o: %.c
	$(call compile-c-steps)

$Od/%.o: %.cc
	$(call compile-cxx-steps)

