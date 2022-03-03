#pragma once
#include "cpu.h"
#include <stdint.h>

typedef struct Cpu Cpu;
typedef struct Bus {
    unsigned char* rom;
    Cpu* cpu;

    int mapping_num;

    unsigned char* cpu_ram;
} Bus;

Bus bus_init(unsigned char* rom);

uint8_t cartridge_read(Bus* bus, uint16_t addr);
uint8_t mem_read(Bus* bus, uint16_t addr);
uint16_t mem_read_16(Bus* bus, uint16_t addr);

uint8_t mem_peek(Bus* bus, uint16_t addr);
uint16_t mem_peek_16(Bus* bus, uint16_t addr);

void mem_write(Bus* bus, uint16_t addr, uint8_t val);
