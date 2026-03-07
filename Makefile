MKDIR := mkdir -p

SRCROOT = $(PWD)
BUILD ?= $(SRCROOT)/.build

SUBDIRS := src

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

uploadeep:
	@$(MAKE) BUILDDIR=$(BUILD)/src SRCROOT=$(SRCROOT) -C src uploadeep

upload:
	@$(MAKE) BUILDDIR=$(BUILD)/src SRCROOT=$(SRCROOT) -C src upload

clean:
	@$(MAKE) BUILDDIR=$(BUILD)/src SRCROOT=$(SRCROOT) -C src clean

$(SUBDIRS):
	@$(MAKE) BUILDDIR=$(BUILD)/$@  SRCROOT=$(SRCROOT) -C $@

