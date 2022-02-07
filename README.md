# Teensy 4.0 Command Line Monitor
This project implements a basic command line monitor program for the Teensy 4.0 development board. The monitor was designed to allow the user to investigate the memory layout, memory contents and integrated peripherals of the soc. Future versions will support download and execution of code from memory. I wrote this code to gain more experience working with small embedded hardware platforms.

The program binary is built entirely from the command line via the project makefile. A makefile target is included to flash the target board as well. The Arduino IDE and Teensyduino add-on files must be installed as they supply the arm compiler tools and teensy specific files required for the build.

## Prerequisits
- Arduino IDE (project uses v1.8.19)
- Teensyduino add-on for the Arduino IDE
- Teensy Loader, Command Line
- Linux udev rules

## Installation
This project was created in a KVM guest VM running Debian 10 (buster).
1. Download and install the Arduino IDE (see download page: https://www.arduino.cc/en/software/)
2. Download and install the Teensyduino add-on (see instructions at: https://www.pjrc.com/teensy/td_download.html)
3. Download the Linux udev rules (download link: https://www.pjrc.com/teensy/00-teensy.rules).
4. Install the udev rules with this command: su -c "cp 00-teensy.rules /etc/udev/rules.d/00-teensy.rules"
5. Download a source archive or clone repository https://github.com/PaulStoffregen/teensy_loader_cli
6. Build the teensy_loader_cli binary. Test the loader by flashing the blink_slow_Teensy40.hex image to the board.
7. Clone this repository to get the project files.

## Build Configuration
The build system needs to know the location of the Arduino installation. I installed the IDE in my home directory using the default settings in the Arduino installer. This placed the root of the installation at /home/bob/arduino-1.8.19. This directory is hard coded in file include/teensy40.mk and may need to be changed to match your specific installation:

    # use the Arduino arm compiler and teensy support files
    ARDUINOPATH := $(HOME)/arduino-1.8.19

In addition the path to the teensy loader is specified in file include/teensy40.mk and will need to be updated:

    LOADER := teensy_loader_cli

As stated earlier my build runs in a KVM guest VM which currently does not fully support hot plugging USB devices. For this reason I installed the teensy_loader_cli program on the KVM host and use ssh to execute the loader to push the hex file to the target hardware. The makefile includes two targets which can be used to program the teensy hardware:
1. flash - scp the hex file to the KVM host and execute the teensy_loader_cli program via ssh to push the hex file to the target
2. flash2 - execute the teensy_loader_cli program directly on the development machine to push the local hex file to the target

The relevant lines in the makefile:

    #-----------------------------------------------------------------------
    # My build runs in a VM (kvm guest) while the teensy is accessed through
    # a host USB port. So instead of calling the loader directly the .hex
    # file is scp'd to the host and the loader is run via ssh.
    flash: $(BUILD_DIR)/$(OUTFILE).hex
        scp $< vm3700:~/teensy
        ssh vm3700 "cd ~/teensy; ./$(LOADER) --mcu=TEENSY40 -s -w -v $(OUTFILE).hex"
    
    # Use this target when the teensy hardware is directly connected.
    flash2: $(BUILD_DIR)/$(OUTFILE).hex
        $(LOADER) --mcu=TEENSY40 -s -w -v $<

Use the target which works best for you.

## Using Make
Executing make (without arguments) displays this help text:

    Command Line Monitor Targets:
    
    all         - compile the .hex output file
    clean       - remove generated files
    edit        - edit files
    flash       - program the teensy board
    hosttest    - build the code for test on PC
    teensylinks - construct teensy source file links

Before building the monitor program construct the required soft links to the teensy source code by executing command:

    make teensylinks

A list of the soft links constructed will be displayed on the console. This command only needs to be run after initial cloning of the project.  

**Note:** due to a race condition (which I haven't investigated yet) in the build process do not combine the teensylinks target with the all target. The result will be a failed build.  

Build the monitor program with this command:

    make clean all

Flash the binary on the target hardware with this command:

    make flash

## Using the Monitor
At the command prompt ('>') type help<return> to get a list of commands:

    > help
           +  add ( a b -- a+b )
           -  subtract ( a b -- a-b )
           *  multiply ( a b -- a*b )
           /  divide ( a b -- a/b )
           .  print decimal top of stack ( a -- )
          .h  print hex top of stack ( a -- )
          .s  print decimal stack
         .hs  print hex stack
       blink  blink led ( count -- )
        dmem  display byte memory ( address lines -- )
      dmem16  display short memory ( address lines -- )
      dmem32  display word memory ( address lines -- )
        emit  print character top of stack ( a -- )
        help  display help text
         mod  modulo divide ( a b -- a mod b )
        peek  display byte at address ( address -- )
      peek16  display short at address ( address -- )
      peek32  display word at address ( address -- )
        poke  store byte at address ( address byte -- )
      poke16  store short at address ( address short -- )
      poke32  store word at address ( address word -- )
       words  display list of command words
        quit  quit the test program

    Decimal number: 1234, -56
        Hex number: 0x1234abcd
      Octal number: 0377
         Character: 'A
    >

The command line uses postfix notation where operators/commands follow their operands. Operands represented by signed decimal, hex, and octal numbers, and by character constants in the form: single quote + character are pushed onto a 16 level stack. The stack holds 32-bit values. Operators/commands take zero or more operands from the stack and perform their function. Some commands like .s, .hs, help and words do not alter the stack contents. Data stack overflow and underflow conditions are detected and reported.

The memory display commands: dmem, dmem16 and dmem32 display memory from the specified 16 byte alligned address (rounded down), displaying bytes, shorts and words with each line representing 16 bytes of memory contents. The byte display (dmem) will also display the ascii character represented by each byte value. The short (dmem16) and word (dmem32) commands will display 8 16-bit short and 4 32-bit values respectively.

Memory values may be read one at a time as bytes, shorts or words using the peek, peek16 and peek32 commands repspectively.

Memory values my be written one at a time as bytes, shorts or words using the poke, poke16 and poke32 commands.

Operands may be entered in the following formats:
- Decimal Number: 42, -89
- Hex Number: 0xdeaf123 (leading '0x' characters indicate case insensitive hex characters follow)
- Octal Number: 0377 (leading '0' character indicates octal digits follow)
- Character: 'A (leading single quote indicates a single character literal follows)

Makefile target 'hosttest' builds a version of the command line monitor which will execute in a Linux command line shell. In this build a quit command will be available to cleanly exit the program. Shell characters Ctrl-C and Ctrl-D may be used to force the program to terminate immediately.

## License and Acknowledgements
- The Teensy 4.0 Command Line Monitor program is Copyright Robert I. Gike under the Apache 2.0 license.
- The Arduino IDE is Copyright Arduino LLC.
- The Teensy 4.0 hardware and software is Copyright PJRC.COM LLC
