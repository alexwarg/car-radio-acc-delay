MKDIR := mkdir -p

BUILD ?= $(SRCROOT)/.build

SUBDIRS := src

export SRCROOT = $(PWD)

.PHONY: all clean $(SUBDIRS)


ifneq ($(MAKECMDGOALS),)
$(MAKECMDGOALS): $(SUBDIRS)
endif

all: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) BUILDDIR=$(BUILD)/$@  -C $@ $(MAKECMDGOALS)

