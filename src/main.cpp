//------------------------------------------------------------------------------
// Teensy 4.0 Command Line Monitor
//
// Command line monitor program the Teensy 4.0 development platform.
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

#ifdef HOSTTEST
  #include "hosttest.h"
#else
  #include "core_pins.h"
  #include "usb_serial.h"
#endif

// character defines
#define CHAR_BKSP  0x7f  // Backspace key
#define CHAR_BS    0x08
#define CHAR_CR    0x0d
#define CHAR_NL    0x0a
#define CHAR_NULL  0x00
#define CHAR_QUOTE 0x27
#define CHAR_SP    0x20
#define CHAR_TILDE 0x7e

// data stack
#define DS_SIZE 16
int32_t ds[DS_SIZE];
int     ds_index;

// dictionary
#define DICT_SIZE 64
typedef void (*wcode_t)();
struct dictentry_t {
    const char* name;
    const char* help;
    wcode_t code;
} dict[DICT_SIZE];
int dict_count; // the last entry + 1

// input buffer
char tib[128];
bool tib_end;
int  tib_offset;

// protoypes
int32_t dpop();
void dpush(int32_t n);
void xerror(const char* msg);

//----------------------------------------------------------------------
void build_dict(const char* name, const wcode_t code, const char* help) {
    dict[dict_count].name = name;
    dict[dict_count].help = help;
    dict[dict_count].code = code;
    dict_count++;
}

//----------------------------------------------------------------------
int32_t dpop() {
    if(ds_index<0) {
        xerror("Data Stack Underflow!");
        return 0;
    }
    return ds[ds_index--];
}

//----------------------------------------------------------------------
void dpush(int32_t n) {
    if(ds_index==DS_SIZE-1) {
        xerror("Data Stack Overflow!");
        return;
    }
    ds[++ds_index] = n;
}

//----------------------------------------------------------------------
void execute() {
}

//----------------------------------------------------------------------
struct dictentry_t* find(const char* name) {
    for(int i=dict_count-1; i>0; i--) {
        if(strcmp(name, dict[i].name)==0) return &dict[i];
    }
    return nullptr;
}

//----------------------------------------------------------------------
void print_address(const uint32_t address) {
#ifdef HOSTTEST
    char buf[20];
    unsigned int address64 = address;
    // add an offset to generate a valid 64-bit address for test
    address64 += reinterpret_cast<unsigned long int>(test_memory);
    sprintf(buf, "%016x ", address64);
#else
    char buf[12];
    sprintf(buf, "%08x ", (unsigned int)address);
#endif
    Serial.print(buf);
}

//----------------------------------------------------------------------
char readmem_int8(const uint32_t address, const int32_t offset=0) {
#ifdef HOSTTEST
    uint64_t address64 = address;
    // add an offset to generate a valid 64-bit address for test
    address64 += reinterpret_cast<uint64_t>(test_memory);
    return reinterpret_cast<char*>(address64)[offset];
#else
    return reinterpret_cast<char*>(address)[offset];
#endif
}

//----------------------------------------------------------------------
int16_t readmem_int16(const uint32_t address, const int32_t offset=0) {
#ifdef HOSTTEST
    uint64_t address64 = address;
    // add an offset to generate a valid 64-bit address for test
    address64 += reinterpret_cast<uint64_t>(test_memory);
    return reinterpret_cast<int16_t*>(address64)[offset];
#else
    return reinterpret_cast<int16_t*>(address)[offset];
#endif
}

//----------------------------------------------------------------------
int32_t readmem_int32(const uint32_t address, const int32_t offset=0) {
#ifdef HOSTTEST
    uint64_t address64 = address;
    // add an offset to generate a valid 64-bit address for test
    address64 += reinterpret_cast<uint64_t>(test_memory);
    return reinterpret_cast<int32_t*>(address64)[offset];
#else
    return reinterpret_cast<int32_t*>(address)[offset];
#endif
}

//----------------------------------------------------------------------
// may have to use single character fetch and loop in place of fgets
// include yield in the loop
const char* tibfill() {
    tib_end = false;
    tib_offset = 0;
    memset(tib, 0, sizeof(tib));
    Serial.print("> ");
#ifdef HOSTTEST
    char* p = fgets(tib, sizeof(tib), stdin);
    if(p) return p;
    printf("^D\n");
    exit(0);
    return nullptr;
#else
    while(true) {
        int b = Serial.read();
        if(b>0) {
            if(char(b)==CHAR_BKSP || char(b)==CHAR_BS) {
                if(tib_offset>0) {
                    Serial.print(char(CHAR_BS));
                    Serial.print(char(CHAR_SP));
                    Serial.print(char(CHAR_BS));
                    tib_offset--;
                }
                continue;
            }
            tib[tib_offset] = char(b);
            if(tib_offset<int(sizeof(tib)-1)) tib_offset++;
            if(char(b)==CHAR_CR) {
                Serial.println(char(CHAR_SP)); // echo
                tib_offset = 0;
                return tib;
            }
            Serial.print(char(b)); // echo
            continue;
        }
        yield();
    }
#endif
}

//----------------------------------------------------------------------
const char tibnext() {
    return tib[tib_offset++];
}

//----------------------------------------------------------------------
bool to_number(const char* w) {
    if(w[0]==CHAR_QUOTE && w[2]==CHAR_NULL) {
        dpush(w[1]); // character literal
        return true;
    }
    char* endptr;
    int32_t v = (int32_t)strtol(w, &endptr, 0);
    dpush(v);
    return int(strlen(w))==(endptr-w);
}

//----------------------------------------------------------------------
const char* word() {
    // drop leading spaces
    while(tib_end==false) {
        char c = tibnext();
        if(c==CHAR_SP) continue;
        tib_offset--; // backup to the character
        break;
    }
    // extract word characters
    const char* p = &tib[tib_offset];
    int count = 0;
    while(tib_end==false) {
        char c = tibnext();
        switch(c) {
        case CHAR_CR:
        case CHAR_NL:
            tib_end = true;
        case CHAR_SP:
            tib[tib_offset-1] = 0;
            return (const char*)(count>0 ? p : nullptr);
        default:
            count++;
            break;
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------
void writemem_int8(const uint32_t address, const int32_t value) {
#ifdef HOSTTEST
    uint64_t address64 = address;
    // add an offset to generate a valid 64-bit address for test
    address64 += reinterpret_cast<uint64_t>(test_memory);
    reinterpret_cast<char*>(address64)[0] = (char)value;
#else
    reinterpret_cast<char*>(address)[0] = (char)value;
#endif
}

//----------------------------------------------------------------------
void writemem_int16(const uint32_t address, const int32_t value) {
#ifdef HOSTTEST
    uint64_t address64 = address;
    // add an offset to generate a valid 64-bit address for test
    address64 += reinterpret_cast<uint64_t>(test_memory);
    reinterpret_cast<int16_t*>(address64)[0] = (int16_t)value;
#else
    reinterpret_cast<int16_t*>(address)[0] = (int16_t)value;
#endif
}

//----------------------------------------------------------------------
void writemem_int32(const uint32_t address, const int32_t value) {
#ifdef HOSTTEST
    uint64_t address64 = address;
    // add an offset to generate a valid 64-bit address for test
    address64 += reinterpret_cast<uint64_t>(test_memory);
    reinterpret_cast<int32_t*>(address64)[0] = (int32_t)value;
#else
    reinterpret_cast<int32_t*>(address)[0] = (int32_t)value;
#endif
}

//----------------------------------------------------------------------
void xerror(const char* msg) {
    Serial.print("Error! ");
    Serial.println(msg);
}

//----------------------------------------------------------------------
// command words
//----------------------------------------------------------------------

//----------------------------------------------------------------------
void add() {
    int32_t v = dpop();
    v += dpop();
    dpush(v);
}

//----------------------------------------------------------------------
void blink() {
#ifdef HOSTTEST
    dpop();
    xerror("No blink in hosttest mode");
#else
    int32_t count = dpop();
    if(count==0) return;
    pinMode(13, OUTPUT);
    while(count>0) {
        digitalWriteFast(13, HIGH);
        delay(500);
        digitalWriteFast(13, LOW);
        delay(500);
        count--;
    }
#endif
}

//----------------------------------------------------------------------
void div2() {
    int32_t v2 = dpop();
    int32_t v1 = dpop();
    if(v2==0)
        xerror("Divide by 0");
    else
        dpush(v1/v2);
}

//----------------------------------------------------------------------
void dmem() {
    int32_t lines = dpop();
    int32_t address = dpop();
    address &= 0xfffffff0; // align to 16 bytes
    char buf[12];
    const int32_t addr_inc = 16;
    for(int32_t i=0; i<lines; ++i) {
        print_address(address);
        for(int32_t x=0; x<addr_inc; ++x) {
            if(x==(addr_inc/2)) Serial.print("| ");
            sprintf(buf, "%02x ", (unsigned char)readmem_int8(address, x));
            Serial.print(buf);
        }
        Serial.print(' ');
        for(int32_t x=0; x<addr_inc; ++x) {
            if(x==(addr_inc/2)) Serial.print(' ');
            char c = readmem_int8(address, x);
            if(c<CHAR_SP || c>CHAR_TILDE) c = '.';
            sprintf(buf, "%c", c);
            Serial.print(buf);
        }
        Serial.println("");
        address += addr_inc;
    }
}

//----------------------------------------------------------------------
void dmem16() {
    int32_t lines = dpop();
    int32_t address = dpop();
    address &= 0xfffffff0; // align to 16 bytes
    char buf[12];
    const int32_t addr_inc = 8;
    for(int32_t i=0; i<lines; ++i) {
        print_address(address);
        for(int32_t x=0; x<addr_inc; ++x) {
            sprintf(buf, "%04x ", (unsigned short)readmem_int16(address, x));
            Serial.print(buf);
        }
        Serial.println("");
        address += (addr_inc * sizeof(int16_t));
    }
}

//----------------------------------------------------------------------
void dmem32() {
    int32_t lines = dpop();
    int32_t address = dpop();
    address &= 0xfffffff0; // align to 16 bytes
    char buf[12];
    const int32_t addr_inc = 4;
    for(int32_t i=0; i<lines; ++i) {
        print_address(address);
        for(int32_t x=0; x<addr_inc; ++x) {
            sprintf(buf, "%08x ", (unsigned int)readmem_int32(address, x));
            Serial.print(buf);
        }
        Serial.println("");
        address += (addr_inc * sizeof(int32_t));
    }
}

//----------------------------------------------------------------------
void dot() {
    Serial.println(dpop());
}

//----------------------------------------------------------------------
void dot_h() {
    char buf[12];
    sprintf(buf, "%08x", (unsigned int)dpop());
    Serial.println(buf);
}

//----------------------------------------------------------------------
void dot_hs() {
    if(ds_index < 0) return;
    char buf[12];
    for(int i=0; i<=ds_index; i++) {
        sprintf(buf, "%08x ", (unsigned int)ds[i]);
        Serial.print(buf);
    }
    Serial.println("");
}

//----------------------------------------------------------------------
void dot_s() {
    if(ds_index < 0) return;
    char buf[12];
    for(int i=0; i<=ds_index; i++) {
        sprintf(buf, "%d ", (int)ds[i]);
        Serial.print(buf);
    }
    Serial.println("");
}

//----------------------------------------------------------------------
void emit() {
    Serial.print(char(dpop()));
}

//----------------------------------------------------------------------
void help() {
    char buf[32];
    for(int i=1; i<dict_count; ++i) {
        sprintf(buf, "%8s  ", dict[i].name);
        Serial.print(buf);
        Serial.println(dict[i].help);
    }
    Serial.println("");
    Serial.println("Decimal number: 1234, -56");
    Serial.println("    Hex number: 0x1234abcd");
    Serial.println("  Octal number: 0377");
    Serial.println("     Character: 'A");
}

//----------------------------------------------------------------------
void mod() {
    int32_t v2 = dpop();
    int32_t v1 = dpop();
    if(v2==0)
        xerror("Modulo Divide by 0");
    else
        dpush(v1%v2);
}

//----------------------------------------------------------------------
void mul() {
    int32_t v2 = dpop();
    int32_t v1 = dpop();
    dpush(v1*v2);
}

//----------------------------------------------------------------------
void peek() {
    int32_t address = dpop();
    print_address(address);
    char buf[16];
    sprintf(buf, "%02x", (unsigned short)readmem_int8(address));
    Serial.println(buf);
}

//----------------------------------------------------------------------
void peek16() {
    int32_t address = dpop();
    print_address(address);
    char buf[16];
    sprintf(buf, "%04x", (unsigned short)readmem_int16(address));
    Serial.println(buf);
}

//----------------------------------------------------------------------
void peek32() {
    int32_t address = dpop();
    print_address(address);
    char buf[16];
    sprintf(buf, "%08x", (unsigned int)readmem_int32(address));
    Serial.println(buf);
}

//----------------------------------------------------------------------
void poke() {
    int32_t value = dpop();
    int32_t address = dpop();
    print_address(address);
    char buf[16];
    writemem_int8(address, value);
    sprintf(buf, "-> %02x", (unsigned char)readmem_int8(address));
    Serial.println(buf);
}

//----------------------------------------------------------------------
void poke16() {
    int32_t value = dpop();
    int32_t address = dpop();
    print_address(address);
    char buf[16];
    writemem_int16(address, value);
    sprintf(buf, "-> %04x", (unsigned short)readmem_int16(address));
    Serial.println(buf);
}

//----------------------------------------------------------------------
void poke32() {
    int32_t value = dpop();
    int32_t address = dpop();
    print_address(address);
    char buf[16];
    writemem_int32(address, value);
    sprintf(buf, "-> %08x", (unsigned int)readmem_int32(address));
    Serial.println(buf);
}

//----------------------------------------------------------------------
void quit() {
    Serial.println("Quit program...");
    exit(0);
}

//----------------------------------------------------------------------
void sub() {
    int32_t v2 = dpop();
    int32_t v1 = dpop();
    dpush(v1-v2);
}

//----------------------------------------------------------------------
void words() {
    for(int i=1; i<dict_count; ++i) {
        Serial.print(dict[i].name);
        Serial.print(' ');
    }
    Serial.println("");
}

//----------------------------------------------------------------------
// main
//----------------------------------------------------------------------
int main() {
    // I/O init
    Serial.begin(38400);

    // init variables
    dict_count = 0;
    ds_index = -1;
    tib_end = true;
    tib_offset = 0;

    // init dictionary
    build_dict(nullptr, nullptr, nullptr);
    build_dict("+",      add,    "add ( a b -- a+b )");
    build_dict("-",      sub,    "subtract ( a b -- a-b )");
    build_dict("*",      mul,    "multiply ( a b -- a*b )");
    build_dict("/",      div2,   "divide ( a b -- a/b )");
    build_dict(".",      dot,    "print decimal top of stack ( a -- )");
    build_dict(".h",     dot_h,  "print hex top of stack ( a -- )");
    build_dict(".s",     dot_s,  "print decimal stack");
    build_dict(".hs",    dot_hs, "print hex stack");
    build_dict("blink",  blink,  "blink led ( count -- )");
    build_dict("dmem",   dmem,   "display byte memory ( address lines -- )");
    build_dict("dmem16", dmem16, "display short memory ( address lines -- )");
    build_dict("dmem32", dmem32, "display word memory ( address lines -- )");
    build_dict("emit",   emit,   "print as character top of stack ( a -- )");
    build_dict("help",   help,   "display help text");
    build_dict("mod",    mod,    "modulo divide ( a b -- a mod b )");
    build_dict("peek",   peek,   "display byte at address ( address -- )");
    build_dict("peek16", peek16, "display short at address ( address -- )");
    build_dict("peek32", peek32, "display word at address ( address -- )");
    build_dict("poke",   poke,   "store byte at address ( address byte -- )");
    build_dict("poke16", poke16, "store short at address ( address short -- )");
    build_dict("poke32", poke32, "store word at address ( address word -- )");
    build_dict("words",  words,  "display list of command words");
#ifdef HOSTTEST
    build_dict("quit",   quit,   "quit the test program");
#endif

    Serial.println("Teensy 4.0 Command Line Monitor");
    Serial.println("Copyright (c) 2022 Robert Gike");

    // interpreter loop
    while(true) {
        tibfill();
        while(true) {
            const char* w = word();
            if(w==nullptr) break;

            // execute when found
            struct dictentry_t* entry = find(w);
            if(entry) { (*entry->code)(); continue; }

            // try convert to number
            if(to_number(w)) continue;
            dpop();

            // report error
            Serial.print(w);
            Serial.println(" ???");
            break;
        }
        yield();
    }
    return 0;
}
