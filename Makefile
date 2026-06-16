UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
  STRIP_FLAGS := -x
else
  STRIP_FLAGS := --strip-unneeded
endif

MODE ?= small

VALID_MODE := fast safe small
ifeq ($(filter $(MODE),$(VALID_MODE)),)
  $(error invalid MODE='$(MODE)'; must be one of: $(VALID_MODE))
endif

TARGET ?=
ifeq ($(strip $(TARGET)),)
  TARGET_FLAG :=
else
  TARGET_FLAG := -Dtarget=$(TARGET)
endif

define strip_bin
	@if command -v strip >/dev/null 2>&1; then \
		strip $(STRIP_FLAGS) $(1); \
	else \
		echo "note: 'strip' command not found; shipping an unstripped binary" >&2; \
	fi
endef

guard-zig:
	@command -v zig >/dev/null 2>&1 || { echo "error: 'zig' not found in PATH; ensure Zig 0.16.0+ is installed and on your PATH" >&2; exit 1; }

release: guard-zig
	zig build --release=$(MODE) $(TARGET_FLAG)
	$(call strip_bin,zig-out/bin/nvi)
	@ls -AFGhl zig-out/bin/nvi

install: guard-zig
	@test -n "$(DIR)" || { echo "error: DIR is required (e.g. make install DIR=\$$HOME/.local)" >&2; exit 1; }
	zig build --release=$(MODE) $(TARGET_FLAG) --prefix $(DIR)
	$(call strip_bin,$(DIR)/bin/nvi)
	@ls -AFGhl $(DIR)/bin/nvi

.PHONY: guard-zig release install
