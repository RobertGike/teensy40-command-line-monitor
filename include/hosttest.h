//------------------------------------------------------------------------------
// Teensy 4.0 Command Line Monitor
//
// Abstractions required to build the program to run on the
// development machine.
//
// Copyright (c) 2022 Robert I. Gike
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------

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

