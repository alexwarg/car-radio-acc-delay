MKDIR := mkdir -p
AVRCXX := avr-g++
AVRCC  := avr-gcc
OBJCOPY := avr-objcopy

AVRDUDE ?= $(AVRDUDE_PREFIX)avrdude
AVRDUDE_CFG ?= -C $(AVRDUDE_PREFIX)avrdude.conf
AVRSIZE ?= avr-size

TOOLDIR := $(PWD)/tools

GEN_IMG := $(TOOLDIR)/gen-oled-bitmap.py


BOARD_MCU := attiny85
CPU_FREQ  := 8000000

TARGET  := $(BUILDDIR)/firmware.hex
CCSRC := main.cc display.cc

OLED_IMGS := off settings delay s

CXXFLAGS := -Wall -Wextra -fno-exceptions -fno-threadsafe-statics -mmcu=$(BOARD_MCU) -Os \
            -ffunction-sections -fdata-sections \
            -g -DF_CPU=$(CPU_FREQ)L \
            -fomit-frame-pointer --std=c++20 \
            -I$(SRCROOT)/include


OBJECTS := $(addprefix $(BUILDDIR)/, $(addsuffix .o,$(CCSRC)))
OLED_HEADERS := $(patsubst %, img_%.h, $(subst -,_,$(OLED_IMGS)))


.PHONY: all clean upload

all: $(TARGET) $(OLED_HEADERS)

clean:
	$(RM) $(OBJECTS) $(TARGET) $(TARGET:.hex=.eep)

upload: $(TARGET) $(TARGET:.hex=.eep)
	$(AVRDUDE) $(AVRDUDE_CFG) -P /dev/ttyUSB0 -p $(BOARD_MCU) -c buspirate -U flash:w:$(TARGET):i  -U eeprom:w:$(TARGET:.hex=.eep):i


uploadeep: $(TARGET:.hex=.eep)
	$(AVRDUDE) $(AVRDUDE_CFG) -P /dev/ttyUSB0 -p $(BOARD_MCU) -c buspirate -U eeprom:w:$<:i

ifeq (,$(filter clean,$(MAKECMDGOALS)))
  -include $(wildcard $(BUILDDIR)/.*.d)
endif


%.eep: %.elf
	$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom="alloc,load" --no-change-warnings --change-section-lma .eeprom=0 $< $@

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(OLED_HEADERS): img_%.h: assets/$(subst _,-,%).tiff
	$(GEN_IMG) $< $@ img_$(*F)

#font generation
# python ~/Downloads/ttf2bmh.py -s 24 -fh 24 -f . --progmem -p -O 0 --vertical --variable_width

$(TARGET:.hex=.elf): $(OBJECTS)
	$(AVRCXX) $(CFLAGS) -o $@ $(CXXFLAGS) -Wl,--gc-sections -fuse-linker-plugin $^
	$(AVRSIZE) --mcu=$(BOARD_MCU) -A $@

$(BUILDDIR)/%.cc.o: %.cc | $(BUILDDIR)
	$(AVRCXX) -MMD -MP -MF $(dir $@)/.$(notdir $@).d -o $@ -c $(CXXFLAGS) $<

$(BUILDDIR)/%.cc.S: %.cc | $(BUILDDIR)
	$(AVRCXX) -o $@ -S -fverbose-asm $(CXXFLAGS) $<

$(BUILDDIR):
	$(MKDIR) $@
