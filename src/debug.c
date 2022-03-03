#include "debug.h"
#include "bus.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

void print_debug(Cpu* cpu) {
  printf("%04X  ", cpu->pc);

  // Convenience
  Bus* bus = cpu->bus;

  uint8_t opcode = mem_peek(bus, cpu->pc);

  int name_index = OPCODES[opcode][0];
  int length = OPCODES[opcode][1];
  AddressingMode mode = (AddressingMode)OPCODES[opcode][2];

  const char* name = OPCODES_NAMES[name_index];

  char bytecodes[10] = "";
  for (int i = 0; i < length; i++) {
    sprintf(bytecodes + strlen(bytecodes), "%02X ",
            mem_peek(bus, (uint16_t)(cpu->pc + i)));
  }

  char display_name[5] = "";

  // Add instruction name
  if (name[0] == '*') {
    sprintf(display_name, "%s", name);
  } else {
    sprintf(display_name, " %s", name);
  }

  char args[28] = "";

  uint8_t one_v = mem_peek(bus, cpu->pc + 1);
  uint16_t one_16_v = mem_peek_16(bus, cpu->pc + 1);

  switch (mode) {
    case Accumulator:
      sprintf(args, "A");
      break;
    case Immediate:
      sprintf(args, "#$%02X", one_v);
      break;
    case ZeroPage:
      sprintf(args, "$%02X = %02X", one_v, mem_peek(bus, one_v));
      break;
    case ZeroPageX:
      sprintf(args, "$%02X,X @ %02X = %02X", one_v, (uint8_t)(one_v + cpu->x),
              mem_peek(bus, (uint8_t)(one_v + cpu->x)));
      break;
    case ZeroPageY:
      sprintf(args, "$%02X,Y @ %02X = %02X", one_v, (uint8_t)(one_v + cpu->y),
              mem_peek(bus, (uint8_t)(one_v + cpu->y)));
      break;
    case Relative:
      sprintf(args, "$%04X", cpu->pc + (int8_t)one_v + length);
      break;
    case Absolute:
      // On most instructions show the current value residing at the absolute
      // address
      if (name_index != 0x1C && name_index != 0x1B) {
        sprintf(args, "$%04X = %02X", one_16_v, mem_peek(bus, one_16_v));
      } else {
        sprintf(args, "$%04X", one_16_v);
      }
      break;
    case AbsoluteX:
      sprintf(args, "$%04X,X @ %04X = %02X", one_16_v,
              (uint16_t)(one_16_v + cpu->x), mem_read(bus, one_16_v + cpu->x));
      break;
    case AbsoluteY:
      sprintf(args, "$%04X,Y @ %04X = %02X", one_16_v,
              (uint16_t)(one_16_v + cpu->y), mem_read(bus, one_16_v + cpu->y));
      break;
    case Indirect: {
      uint16_t real;
      // 6502 page boundary bug
      if ((one_16_v & 0x00FF) == 0x00FF) {
        uint8_t lo = mem_peek(bus, one_16_v);
        uint8_t hi = mem_peek(bus, one_16_v & 0xFF00);
        real = (uint16_t)((hi << 8) | lo);
      } else {
        real = mem_peek_16(bus, one_16_v);
      }

      sprintf(args, "($%04X) = %04X", one_16_v, real);
      break;
    }
    case IndirectX: {
      uint8_t addr = one_v + cpu->x;
      uint8_t lo = mem_peek(bus, addr);
      uint8_t hi = mem_peek(bus, (uint8_t)(addr + 1));
      uint16_t final = (uint16_t)(hi << 8) | lo;
      sprintf(args, "($%02X,X) @ %02X = %04X = %02X", one_v, addr, final,
              mem_peek(bus, final));
      break;
    }
    case IndirectY: {
      uint8_t addr = mem_read(cpu->bus, cpu->pc + 1);
      uint8_t lo = mem_read(cpu->bus, addr);
      uint8_t hi = mem_read(cpu->bus, (uint8_t)(addr + 1));
      uint16_t final = (uint16_t)((hi << 8) | lo) + cpu->y;

      sprintf(args, "($%02X),Y = %04X @ %04X = %02X", one_v, (hi << 8) | lo,
              final, mem_peek(bus, final));
      break;
    }
    case Implied:
      break;
  }

  printf("%-9s%-3s %-27s ", bytecodes, display_name, args);
  printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", cpu->a, cpu->x, cpu->y,
         cpu->status, cpu->sp);

  // printf(" CYC:%i\n", cpu->cycles_total);
}
