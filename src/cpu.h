#pragma once
#include "bus.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum AddressingMode {
  Implied,
  Accumulator,
  Immediate,
  ZeroPage,
  ZeroPageX,
  ZeroPageY,
  Relative,
  Absolute,
  AbsoluteX,
  AbsoluteY,
  Indirect,
  IndirectX,
  IndirectY
} AddressingMode;

// clang-format off
static const char* OPCODES_NAMES[0x50] = {
//         0      1      2      3      4      5      6     7       8      9       A     B      C      D      E      F
/*0x0_*/ "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI",
/*0x1_*/ "CLV", "CMP", "CPX", "CPY", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY", "JMP", "JSR", "LDA", "LDX", "LDY",
/*0x2_*/ "LSR", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP", "ROL", "ROR", "RTI", "RTS", "SBC", "SEC", "SED", "SEI", "STA",
/*0x3_*/ "STX", "STY", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA", "*AAC","*SAX","*ARR","*ASR","*ATX","*AXA","*AXS","*DCP",
/*0x4_*/ "*DOP","*ISB","*KIL","*LAR","*LAX","*NOP","*RLA","*RRA","*SBC","*SLO","*SRE","*SXA","*SYA","*TOP","*XAA","*XAS"
};

// Mapping of {name index, length, addressing mode, cycle count, page breaking}
static const int OPCODES[0x100][5] = {
/*                 0                         1                  2                        3                  4                         5                     6                         7                   8                    9                       A                            B                    C                       D                   E                        F */
/*0x0_*/ {0x0A,1,Implied,7,0},  {0x22,2,IndirectX,6,0},{0x42,1,Implied,0,0},  {0x49,2,IndirectX,8,0},{0x45,2,ZeroPage,3,0}, {0x22,2,ZeroPage,3,0}, {0x02,2,ZeroPage,5,0}, {0x49,2,ZeroPage,5,0}, {0x24,1,Implied,3,0},{0x22,2,Immediate,2,0},{0x02,1,Accumulator,2,0},{0x38,2,Immediate,2,0},{0x45,3,Absolute,4,0}, {0x22,3,Absolute,4,0}, {0x02,3,Absolute,6,0}, {0x49,3,Absolute,6,0},
/*0x1_*/ {0x09,2,Relative,2,1}, {0x22,2,IndirectY,5,1},{0x42,1,Implied,0,0},  {0x49,2,IndirectY,8,0},{0x45,2,ZeroPageX,4,0},{0x22,2,ZeroPageX,4,0},{0x02,2,ZeroPageX,6,0},{0x49,2,ZeroPageX,6,0},{0x0D,1,Implied,2,0},{0x22,3,AbsoluteY,4,1},{0x45,1,Implied,2,0},    {0x49,3,AbsoluteY,7,0},{0x45,3,AbsoluteX,4,1},{0x22,3,AbsoluteX,4,1},{0x02,3,AbsoluteX,7,0},{0x49,3,AbsoluteX,7,0},
/*0x2_*/ {0x1C,3,Absolute,6,0}, {0x01,2,IndirectX,6,0},{0x42,1,Implied,0,0},  {0x46,2,IndirectX,8,0},{0x06,2,ZeroPage,3,0}, {0x01,2,ZeroPage,3,0}, {0x27,2,ZeroPage,5,0}, {0x46,2,ZeroPage,5,0}, {0x26,1,Implied,4,0},{0x01,2,Immediate,2,0},{0x27,1,Accumulator,2,0},{0x38,2,Immediate,2,0},{0x06,3,Absolute,4,0}, {0x01,3,Absolute,4,0}, {0x27,3,Absolute,6,0}, {0x46,3,Absolute,6,0},
/*0x3_*/ {0x07,2,Relative,2,1}, {0x01,2,IndirectY,5,1},{0x42,1,Implied,0,0},  {0x46,2,IndirectY,8,0},{0x45,2,ZeroPageX,4,0},{0x01,2,ZeroPageX,4,0},{0x27,2,ZeroPageX,6,0},{0x46,2,ZeroPageX,6,0},{0x2C,1,Implied,2,0},{0x01,3,AbsoluteY,4,1},{0x45,1,Implied,2,0},    {0x46,3,AbsoluteY,7,0},{0x45,3,AbsoluteX,4,1},{0x01,3,AbsoluteX,4,1},{0x27,3,AbsoluteX,7,0},{0x46,3,AbsoluteX,7,0},
/*0x4_*/ {0x29,1,Implied,6,0},  {0x17,2,IndirectX,6,0},{0x42,1,Implied,0,0},  {0x4A,2,IndirectX,8,0},{0x45,2,ZeroPage,3,0}, {0x17,2,ZeroPage,3,0}, {0x20,2,ZeroPage,5,0}, {0x4A,2,ZeroPage,5,0}, {0x23,1,Implied,3,0},{0x17,2,Immediate,2,0},{0x20,1,Accumulator,2,0},{0x3B,2,Immediate,2,0},{0x1B,3,Absolute,3,0}, {0x17,3,Absolute,4,0}, {0x20,3,Absolute,6,0}, {0x4A,3,Absolute,6,0},
/*0x5_*/ {0x0B,2,Relative,2,1}, {0x17,2,IndirectY,5,1},{0x42,1,Implied,0,0},  {0x4A,2,IndirectY,8,0},{0x45,2,ZeroPageX,4,0},{0x17,2,ZeroPageX,4,0},{0x20,2,ZeroPageX,6,0},{0x4A,2,ZeroPageX,6,0},{0x0F,1,Implied,2,0},{0x17,3,AbsoluteY,4,1},{0x45,1,Implied,2,0},    {0x4A,3,AbsoluteY,7,0},{0x45,3,AbsoluteX,4,1},{0x17,3,AbsoluteX,4,1},{0x20,3,AbsoluteX,7,0},{0x4A,3,AbsoluteX,7,0},
/*0x6_*/ {0x2A,1,Implied,6,0},  {0x00,2,IndirectX,6,0},{0x42,1,Implied,0,0},  {0x47,2,IndirectX,8,0},{0x45,2,ZeroPage,3,0}, {0x00,2,ZeroPage,3,0}, {0x28,2,ZeroPage,5,0}, {0x47,2,ZeroPage,5,0}, {0x25,1,Implied,4,0},{0x00,2,Immediate,2,0},{0x28,1,Accumulator,2,0},{0x3A,2,Immediate,2,0},{0x1B,3,Indirect,5,0}, {0x00,3,Absolute,4,0}, {0x28,3,Absolute,6,0}, {0x47,3,Absolute,6,0},
/*0x7_*/ {0x0C,2,Relative,2,1}, {0x00,2,IndirectY,5,1},{0x42,1,Implied,0,0},  {0x47,2,IndirectY,8,0},{0x45,2,ZeroPageX,4,0},{0x00,2,ZeroPageX,4,0},{0x28,2,ZeroPageX,6,0},{0x47,2,ZeroPageX,6,0},{0x2E,1,Implied,2,0},{0x00,3,AbsoluteY,4,1},{0x45,1,Implied,2,0},    {0x47,3,AbsoluteY,7,0},{0x45,3,AbsoluteX,4,1},{0x00,3,AbsoluteX,4,1},{0x28,3,AbsoluteX,7,0},{0x47,3,AbsoluteX,7,0},
/*0x8_*/ {0x45,2,Immediate,2,0},{0x2F,2,IndirectX,6,0},{0x45,2,Immediate,2,0},{0x39,2,IndirectX,6,0},{0x31,2,ZeroPage,3,0}, {0x2F,2,ZeroPage,3,0}, {0x30,2,ZeroPage,3,0}, {0x39,2,ZeroPage,3,0}, {0x16,1,Implied,2,0},{0x45,2,Immediate,2,0},{0x35,1,Implied,2,0},    {0x4E,2,Immediate,2,0},{0x31,3,Absolute,4,0}, {0x2F,3,Absolute,4,0}, {0x30,3,Absolute,4,0}, {0x39,3,Absolute,4,0},
/*0x9_*/ {0x03,2,Relative,2,1}, {0x2F,2,IndirectY,6,0},{0x42,1,Implied,0,0},  {0x3D,2,IndirectY,6,0},{0x31,2,ZeroPageX,4,0},{0x2F,2,ZeroPageX,4,0},{0x30,2,ZeroPageY,4,0},{0x39,2,ZeroPageY,4,0},{0x37,1,Implied,2,0},{0x2F,3,AbsoluteY,5,0},{0x36,1,Implied,2,0},    {0x4C,3,AbsoluteX,5,0},{0x4F,3,AbsoluteY,5,0},{0x2F,3,AbsoluteX,5,0},{0x4B,3,AbsoluteY,5,0},{0x3D,3,AbsoluteY,5,0},
/*0xA_*/ {0x1F,2,Immediate,2,0},{0x1D,2,IndirectX,6,0},{0x1E,2,Immediate,2,0},{0x44,2,IndirectX,6,0},{0x1F,2,ZeroPage,3,0}, {0x1D,2,ZeroPage,3,0}, {0x1E,2,ZeroPage,3,0}, {0x44,2,ZeroPage,3,0}, {0x33,1,Implied,2,0},{0x1D,2,Immediate,2,0},{0x32,1,Implied,2,0},    {0x3C,2,Immediate,2,0},{0x1F,3,Absolute,4,0}, {0x1D,3,Absolute,4,0}, {0x1E,3,Absolute,4,0}, {0x44,3,Absolute,4,0},
/*0xB_*/ {0x04,2,Relative,2,1}, {0x1D,2,IndirectY,5,1},{0x42,1,Implied,0,0},  {0x44,2,IndirectY,5,1},{0x1F,2,ZeroPageX,4,0},{0x1D,2,ZeroPageX,4,0},{0x1E,2,ZeroPageY,4,0},{0x44,2,ZeroPageY,4,0},{0x10,1,Implied,2,0},{0x1D,3,AbsoluteY,4,1},{0x34,1,Implied,2,0},    {0x43,3,AbsoluteY,4,1},{0x1F,3,AbsoluteX,4,1},{0x1D,3,AbsoluteX,4,1},{0x1E,3,AbsoluteY,4,1},{0x44,3,AbsoluteY,4,1},
/*0xC_*/ {0x13,2,Immediate,2,0},{0x11,2,IndirectX,6,0},{0x45,2,Immediate,2,0},{0x3F,2,IndirectX,8,0},{0x13,2,ZeroPage,3,0}, {0x11,2,ZeroPage,3,0}, {0x14,2,ZeroPage,5,0}, {0x3F,2,ZeroPage,5,0}, {0x1A,1,Implied,2,0},{0x11,2,Immediate,2,0},{0x15,1,Implied,2,0},    {0x3E,2,Immediate,2,0},{0x13,3,Absolute,4,0}, {0x11,3,Absolute,4,0}, {0x14,3,Absolute,6,0}, {0x3F,3,Absolute,6,0},
/*0xD_*/ {0x08,2,Relative,2,1}, {0x11,2,IndirectY,5,1},{0x42,1,Implied,0,0},  {0x3F,2,IndirectY,8,0},{0x45,2,ZeroPageX,4,0},{0x11,2,ZeroPageX,4,0},{0x14,2,ZeroPageX,6,0},{0x3F,2,ZeroPageX,6,0},{0x0E,1,Implied,2,0},{0x11,3,AbsoluteY,4,1},{0x45,1,Implied,2,0},    {0x3F,3,AbsoluteY,7,0},{0x45,3,AbsoluteX,4,1},{0x11,3,AbsoluteX,4,1},{0x14,3,AbsoluteX,7,0},{0x3F,3,AbsoluteX,7,0},
/*0xE_*/ {0x12,2,Immediate,2,0},{0x2B,2,IndirectX,6,0},{0x45,2,Immediate,2,0},{0x41,2,IndirectX,8,0},{0x12,2,ZeroPage,3,0}, {0x2B,2,ZeroPage,3,0}, {0x18,2,ZeroPage,5,0}, {0x41,2,ZeroPage,5,0}, {0x19,1,Implied,2,0},{0x2B,2,Immediate,2,0},{0x21,1,Implied,2,0},    {0x48,2,Immediate,2,0},{0x12,3,Absolute,4,0}, {0x2B,3,Absolute,4,0}, {0x18,3,Absolute,6,0}, {0x41,3,Absolute,6,0},
/*0xF_*/ {0x05,2,Relative,2,1}, {0x2B,2,IndirectY,5,1},{0x42,1,Implied,0,0},  {0x41,2,IndirectY,8,0},{0x45,2,ZeroPageX,4,0},{0x2B,2,ZeroPageX,4,0},{0x18,2,ZeroPageX,6,0},{0x41,2,ZeroPageX,6,0},{0x2D,1,Implied,2,0},{0x2B,3,AbsoluteY,4,1},{0x45,1,Implied,2,0},    {0x41,3,AbsoluteY,7,0},{0x45,3,AbsoluteX,4,1},{0x2B,3,AbsoluteX,4,1},{0x18,3,AbsoluteX,7,0},{0x41,3,AbsoluteX,7,0}
};
// clang-format on

typedef struct Bus Bus;
typedef struct Cpu {
  Bus* bus;

  uint8_t a;
  uint8_t x;
  uint8_t y;
  uint8_t status;
  uint8_t sp;
  uint16_t pc;

  int cycles_remaining;
  int cycles_total;
  bool bounds_crossed;
} Cpu;

// clang-format on

Cpu cpu_init(Bus* bus);
void cpu_execute(Cpu* cpu);
