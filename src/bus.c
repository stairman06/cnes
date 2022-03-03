#include "bus.h"
#include <stdint.h>
#include <stdlib.h>

Bus bus_init(unsigned char* rom) {
  return (Bus){
      .rom = rom,
      .cpu_ram = calloc(0x0800, 1),
      .mapping_num = (rom[6] >> 4 | (rom[7] & 0b11110000)),
  };
}

uint8_t cartridge_read(Bus* bus, uint16_t raw_addr) {
  uint16_t addr = raw_addr;
  switch (bus->mapping_num) {
    case 000:
      addr -= 0x8000;
      int prg_rom_size = bus->rom[4];
      if (prg_rom_size == 1) {
        // Mirror down 0x4000 if the PRG ROM is smaller
        addr %= 0x4000;
      }

      // Add the header (0x10)
      return bus->rom[addr + 0x10];
    default:
      break;
  }

  return 0;
}

uint8_t mem_read(Bus* bus, uint16_t addr) {
  // Mirror internal RAM addresses
  if (addr <= 0x1FFF) {
    return bus->cpu_ram[addr % 0x0800];
  }

  // Cartridge space
  if (addr >= 0x4020) {
    return cartridge_read(bus, addr);
  }

  return 0;
}

uint16_t mem_read_16(Bus* bus, uint16_t addr) {
  uint8_t lo = mem_read(bus, addr);
  uint8_t hi = mem_read(bus, addr + 1);

  return (uint16_t)((hi << 8) | lo);
}

uint8_t mem_peek(Bus* bus, uint16_t addr) { return mem_read(bus, addr); }
uint16_t mem_peek_16(Bus* bus, uint16_t addr) { return mem_read_16(bus, addr); }

void mem_write(Bus* bus, uint16_t addr, uint8_t val) {
  // Internal RAM
  if (addr <= 0x1FFF) {
    bus->cpu_ram[addr % 0x0800] = val;
  }
}
