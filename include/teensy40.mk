# vim: set ts=4 sw=4 tw=0 noet:
#-------------------------------------------------------------------------------
# Teensy 4.0 .mk file - Teensy build environment
#
# Include file used to define the teensy build specific configuration.
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

# use the Arduino arm compiler and teensy support files
ARDUINOPATH := $(HOME)/arduino-1.8.19

# teensy include files
TEENSY4PATH := $(ARDUINOPATH)/hardware/teensy/avr/cores/teensy4

# path to teensy loader, teensy_post_compile and teensy_reboot (on Linux)
TOOLSPATH := $(ARDUINOPATH)/hardware/tools

# path to Arduino libraries (currently not used)
LIBRARYPATH := $(ARDUINOPATH)/libraries

# path to the arm-none-eabi compiler
COMPILERPATH := $(ARDUINOPATH)/hardware/tools/arm/bin

GCC     := $(COMPILERPATH)/arm-none-eabi-gcc
GPP     := $(COMPILERPATH)/arm-none-eabi-g++
OBJCOPY := $(COMPILERPATH)/arm-none-eabi-objcopy
OBJDUMP := $(COMPILERPATH)/arm-none-eabi-objdump
SIZE    := $(COMPILERPATH)/arm-none-eabi-size

# Teensy 4.0
MCU     := IMXRT1062
MCU_LD  := imxrt1062.ld
MCU_DEF := __$(MCU)__
MCU_DEF += F_CPU=600000000
MCU_DEF += ARDUINO_TEENSY40
MCU_DEF += ARDUINO=10813    # for Arduino Libraries
MCU_DEF += TEENSYDUINO=154  # for Arduino Libraries

F_MCU   := $(addprefix -D,$(MCU_DEF))

F_CPU   := -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb
F_OPT   := -O2
F_COM   := -g -Wall -ffunction-sections -fdata-sections -MMD

F_CPP   := -std=gnu++14 -fno-unwind-tables -fno-exceptions -fpermissive -fno-rtti -fno-threadsafe-statics -felide-constructors -Wno-error=narrowing
F_C     := 
F_S     := -x assembler-with-cpp

# use this for a smaller, no-float printf
# --specs=nano.specs
F_LD    := -Wl,--gc-sections,--print-memory-usage,--relax $(F_CPU) -Tlink/$(MCU_LD) --specs=nosys.specs

CPP_FLAGS := $(F_CPU) $(F_OPT) $(F_COM) $(F_MCU) $(F_CPP)
C_FLAGS   := $(F_CPU) $(F_OPT) $(F_COM) $(F_MCU) $(F_C)
S_FLAGS   := $(F_CPU) $(F_OPT) $(F_COM) $(F_MCU) $(F_S)
LD_FLAGS  := $(F_LD)

LOADER    := teensy_loader_cli

TEENSYSRC := analog.c bootdata.c clockspeed.c delay.c digital.c EventResponder.cpp
TEENSYSRC += HardwareSerial.cpp nonstd.c Print.cpp pwm.c rtc.c serialEvent.cpp startup.c
TEENSYSRC += tempmon.c usb.c usb_desc.c usb_inst.cpp usb_serial.c yield.cpp

