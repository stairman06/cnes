
#include "bus.h"
#include "cpu.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Syntax: %s <ines rom file>\n", argv[0]);
    return 1;
  }

  char* filename = argv[1];

  FILE* file = fopen(filename, "rb");
  fseek(file, 0, SEEK_END);
  unsigned long rom_size = (unsigned long)ftell(file);
  rewind(file);

  unsigned char* rom = malloc(rom_size);
  fread(rom, 1, rom_size, file);
  fclose(file);

  if (rom[0] != 'N' && rom[1] != 'E' && rom[2] != 'S' && rom[3] != 0x1A) {
    printf("Not an iNES file\n");
    return 1;
  }

  Bus bus = bus_init(rom);
  Cpu cpu = cpu_init(&bus);
  bus.cpu = &cpu;

  while (true) {
    cpu_execute(&cpu);
  }
}
