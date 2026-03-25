MKDIR := mkdir -p


SUBDIRS := src

export SRCROOT = $(PWD)
BUILD ?= $(SRCROOT)/.build

.PHONY: all clean $(SUBDIRS)


ifneq ($(MAKECMDGOALS),)
$(MAKECMDGOALS): $(SUBDIRS)
endif

all: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) BUILDDIR=$(BUILD)/$@  -C $@ $(MAKECMDGOALS)

