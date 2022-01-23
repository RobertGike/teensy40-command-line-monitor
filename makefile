# vim: set ts=4 sw=4 tw=0 noet:
#-------------------------------------------------------------------------------
# Teensy 4.0 Command Line Monitor Program
#
# Makefile for building the target image.
#
# Copyright (c) 2022 Robert I. Gike
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#-------------------------------------------------------------------------------

MKDIR_P := mkdir -p
RM_F    := rm -f
SHELL   := /bin/bash

# the target file name
OUTFILE := cmdline

# project directories
BUILD_DIR  := build
INC_DIR    := include
LINK_DIR   := link
SRC_DIR    := src
TEENSY_DIR := src/teensy

# teensy platform configuration
include $(INC_DIR)/teensy40.mk

# project configuration
PROJECT_DEFS := USB_SERIAL LAYOUT_US_ENGLISH USING_MAKEFILE

LINK_FILE  := imxrt1062.ld

SRCS := $(shell find $(SRC_DIR) -name *.c -or -name *.cpp -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/obj/%.o)
DEPS := $(OBJS:.o=.d)

INCLUDE_DIRS  := $(INC_DIR) $(TEENSY4PATH)
INCLUDE_FILES := $(shell find $(INC_DIR) -name *.h)

LIB_DIRS := lib
LIBS     := m c_nano stdc++_nano gcc

CPP_FLAGS += $(addprefix -I,$(INCLUDE_DIRS)) $(addprefix -D,$(PROJECT_DEFS))
C_FLAGS   += $(addprefix -I,$(INCLUDE_DIRS)) $(addprefix -D,$(PROJECT_DEFS))
S_FLAGS   += $(addprefix -I,$(INCLUDE_DIRS))
LD_FLAGS  += $(addprefix -L,$(LIB_DIRS)) $(addprefix -l,$(LIBS))

HOST_GPP := g++ -std=gnu++14 -DHOSTTEST -Iinclude -Wall -Werror

#-----------------------------------------------------------------------
# help
#-----------------------------------------------------------------------
.PHONY: help
help:
	@echo ""
	@echo "Command Line Monitor Targets:"
	@echo ""
	@echo "all         - compile the .hex output file"
	@echo "clean       - remove generated files"
	@echo "edit        - edit files"
	@echo "flash       - program the teensy board"
	@echo "hosttest    - build the code for test on PC"
	@echo "teensylinks - construct teensy source file links"
	@echo ""

#-----------------------------------------------------------------------
.PHONY: all
all: $(BUILD_DIR)/$(OUTFILE).hex

#-----------------------------------------------------------------------
.PHONY: clean
clean:
	@echo "Remove generated files..."
	@$(RM_F) -r $(BUILD_DIR)

#-----------------------------------------------------------------------
.PHONY: edit
edit:
	@vi $(SRCS) $(INCLUDE_FILES) makefile $(INC_DIR)/teensy40.mk

#-----------------------------------------------------------------------
# My build runs in a VM (kvm guest) while the teensy is accessed through
# a host USB port. So instead of calling the loader directly the .hex
# file is scp'd to the host and the loader is run via ssh.
.PHONY: flash
flash: $(BUILD_DIR)/$(OUTFILE).hex
	scp $< vm3700:~/teensy
	ssh vm3700 "cd ~/teensy; ./$(LOADER) --mcu=TEENSY40 -s -w -v $(OUTFILE).hex"

# Use this target when the teensy hardware is directly connected
.PHONY: flash2
flash2: $(BUILD_DIR)/$(OUTFILE).hex
	$(LOADER) --mcu=TEENSY40 -s -w -v $<

#-----------------------------------------------------------------------
# standalone host test binary
.PHONY: hosttest
hosttest:
	@$(MKDIR_P) $(BUILD_DIR)
	$(HOST_GPP) $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/hosttest

#-----------------------------------------------------------------------
# construct the teensy source file links
# teensy *.c and *.cpp files
.PHONY: teensylinks
teensylinks:
	@$(MKDIR_P) $(TEENSY_DIR)
	@$(RM_F) $(TEENSY_DIR)/*
	@for file in $(TEENSYSRC) ; do \
		echo "ln -s $(TEENSY4PATH)/$$file $(TEENSY_DIR)/." ; \
		ln -s $(TEENSY4PATH)/$$file $(TEENSY_DIR)/. ; \
	done 
	@# teensy linker file
	@$(MKDIR_P) $(LINK_DIR)
	@$(RM_F) $(LINK_DIR)/$(LINK_FILE)
	ln -s $(TEENSY4PATH)/$(LINK_FILE) $(LINK_DIR)/.

#-----------------------------------------------------------------------
# convert the output file from elf to hex
$(BUILD_DIR)/$(OUTFILE).hex: $(BUILD_DIR)/$(OUTFILE).elf
	@echo "Convert elf to hex..."
	@$(OBJCOPY) -O ihex -R .eeprom $(BUILD_DIR)/$(OUTFILE).elf $(BUILD_DIR)/$(OUTFILE).hex
	@$(OBJDUMP) -d -x $(BUILD_DIR)/$(OUTFILE).elf > $(BUILD_DIR)/$(OUTFILE).dis
	@$(OBJDUMP) -d -S -C $(BUILD_DIR)/$(OUTFILE).elf > $(BUILD_DIR)/$(OUTFILE).lst
	@$(SIZE) $(BUILD_DIR)/$(OUTFILE).elf

#-----------------------------------------------------------------------
# link the program
$(BUILD_DIR)/$(OUTFILE).elf: $(OBJS)
	@echo "Linking..."
#	@echo "$(GPP) $(CPP_FLAGS) -Xlinker -Map=$(BUILD_DIR)/$(OUTFILE).map $(LD_FLAGS) -o $@ $^"
	@$(GPP) $(CPP_FLAGS) -Xlinker -Map=$(BUILD_DIR)/$(OUTFILE).map $(LD_FLAGS) -o $@ $^

#-----------------------------------------------------------------------
# compile assembler source
$(BUILD_DIR)/obj/%.s.o: %.s
	@echo "Assembling (s)   $<"
	@$(MKDIR_P) $(dir $@)
#	@echo "$(GPP) $(S_FLAGS) -c $< -o $@"
	@$(GPP) $(S_FLAGS) -c $< -o $@

#-----------------------------------------------------------------------
# compile C source
$(BUILD_DIR)/obj/%.c.o: %.c
	@echo "Compiling  (c)   $<"
	@$(MKDIR_P) $(dir $@)
#	@echo "$(GCC) $(C_FLAGS) -c $< -o $@"
	@$(GCC) $(C_FLAGS) -c $< -o $@

#-----------------------------------------------------------------------
# compile C++ source
$(BUILD_DIR)/obj/%.cpp.o: %.cpp
	@echo "Compiling  (c++) $<"
	@$(MKDIR_P) $(dir $@)
#	@echo "$(GPP) ${CPP_FLAGS} -c $< -o $@"
	@$(GPP) ${CPP_FLAGS} -c $< -o $@

