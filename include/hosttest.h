//----------------------------------------------------------------------
// Teensy 4.0 Command Line Monitor
//
// Abstractions required to build the program to run on the
// development machine.
//
// Copyright (c) 2022, RIG Industries Limited. All Rights Reserved.
//----------------------------------------------------------------------

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

class cUSBSerial {
public:
    void begin(const int speed) { _speed = speed; }
    void print(const char c) { std::cout << c; }
    void print(const char* const s) { std::cout << s; }
    void print(const int n) { std::cout << n; }
    void println(const char* const s) { std::cout << s << std::endl; }
    void println(const int n) { std::cout << n << std::endl; }
private:
    int _speed;
};
cUSBSerial Serial;
void yield() { }

// memory read/write test area
char test_memory[4096];

