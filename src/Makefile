MKDIR := mkdir -p
AVRCXX := avr-g++
AVRCC  := avr-gcc
OBJCOPY := avr-objcopy

AVRDUDE ?= $(AVRDUDE_PREFIX)avrdude
AVRDUDE_CFG ?= -C $(AVRDUDE_PREFIX)avrdude.conf
AVRSIZE ?= avr-size

BOARD_MCU := attiny85
CPU_FREQ  := 8000000

TARGET  := $(BUILDDIR)/firmware.hex
CCSRC := main.cc


CXXFLAGS := -fno-exceptions -fno-threadsafe-statics -mmcu=$(BOARD_MCU) -Os \
            -ffunction-sections -fdata-sections -O3 -Os -g -DF_CPU=$(CPU_FREQ)L \
            -fomit-frame-pointer -flto

OBJECTS := $(addprefix $(BUILDDIR)/, $(addsuffix .o,$(CCSRC)))


.PHONY: all clean upload

all: $(TARGET)

clean:
	$(RM) $(OBJECTS) $(TARGET)

upload: $(TARGET)
	$(AVRDUDE) $(AVRDUDE_CFG) -P /dev/ttyUSB0 -p $(BOARD_MCU) -c buspirate -U flash:w:$(TARGET):i


ifeq (,$(filter clean,$(MAKECMDGOALS)))
  -include $(wildcard $(BUILDDIR)/.*.d)
endif


%.eep: %.elf
	$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom="alloc,load" --no-change-warnings --change-section-lma .eeprom=0 $< $@

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(TARGET:.hex=.elf): $(OBJECTS)
	$(AVRCXX) $(CFLAGS) -o $@ $(CXXFLAGS) -Wl,--gc-sections -fuse-linker-plugin $?
	$(AVRSIZE) --mcu=$(BOARD_MCU) -A $@

$(BUILDDIR)/%.cc.o: %.cc | $(BUILDDIR)
	$(AVRCXX) -MMD -MP -MF $(dir $@)/.$(notdir $@).d -o $@ -c $(CXXFLAGS) $<

$(BUILDDIR):
	$(MKDIR) $@
