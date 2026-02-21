MKDIR := mkdir -p
BUILD := $(PWD)/.build

SUBDIRS := src

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

upload:
	@$(MAKE) BUILDDIR=$(BUILD)/src -C src upload

clean:
	@$(MAKE) BUILDDIR=$(BUILD)/src -C src clean

$(SUBDIRS):
	@$(MAKE) BUILDDIR=$(BUILD)/$@ -C $@

