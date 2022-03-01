#pragma once
#include "cpu.h"

typedef struct Cpu Cpu;
typedef struct Bus {
    unsigned char* rom;
    Cpu* cpu;
} Bus;

Bus bus_init(unsigned char* rom);
