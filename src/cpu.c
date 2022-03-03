#include "cpu.h"
#include "bus.h"
#include "debug.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

Cpu cpu_init(Bus* bus) {
  return (Cpu){.bus = bus, .pc = 0xC000, .sp = 0xFD, .status = 0x24};
}

static bool pages_differ(uint16_t one, uint16_t two) {
  return (one & 0xFF00) != (two & 0xFF00);
}

static uint16_t get_address_immediate(Cpu* cpu) { return cpu->pc++; }

static uint16_t get_address_zeropage(Cpu* cpu) {
  return mem_read(cpu->bus, cpu->pc++);
}

static uint16_t get_address_zeropage_x(Cpu* cpu) {
  return (uint8_t)(get_address_zeropage(cpu) + cpu->x);
}

static uint16_t get_address_zeropage_y(Cpu* cpu) {
  return (uint8_t)(get_address_zeropage(cpu) + cpu->y);
}

static uint16_t get_address_relative(Cpu* cpu) {
  uint16_t addr = (uint16_t)(cpu->pc + (int8_t)mem_read(cpu->bus, cpu->pc++));
  if (pages_differ(cpu->pc, addr + 1)) {
    cpu->bounds_crossed = true;
  }

  return addr;
}

static uint16_t get_address_absolute(Cpu* cpu) {
  uint8_t lo = mem_read(cpu->bus, cpu->pc++);
  uint8_t hi = mem_read(cpu->bus, cpu->pc++);
  return (uint16_t)((hi << 8) | lo);
}

static uint16_t get_address_absolute_x(Cpu* cpu) {
  uint16_t base = get_address_absolute(cpu);
  uint16_t final = base + cpu->x;
  if (pages_differ(base, final)) {
    cpu->bounds_crossed = true;
  }

  return final;
}

static uint16_t get_address_absolute_y(Cpu* cpu) {
  uint16_t base = get_address_absolute(cpu);
  uint16_t final = base + cpu->y;
  if (pages_differ(base, final)) {
    cpu->bounds_crossed = true;
  }

  return final;
}

static uint16_t get_address_indirect(Cpu* cpu) {
  // Read next 2 bytes
  uint16_t base = get_address_absolute(cpu);

  // Simulate page boundary bug
  if ((base & 0x00FF) == 0x00FF) {
    // When getting the hi byte, 6502 does not overflow into the next position
    // e.g. $10FF will get the low from $10FF and the high from $1000 (not
    // $1100)
    uint8_t lo = mem_read(cpu->bus, base);
    uint8_t hi = mem_read(cpu->bus, base & 0xFF00);
    return (uint16_t)((hi << 8) | lo);
  }

  // No bug otherwise, just read the value at the location
  return mem_read_16(cpu->bus, base);
}

static uint16_t get_address_indirect_x(Cpu* cpu) {
  uint8_t base = mem_read(cpu->bus, cpu->pc++) + cpu->x;
  uint8_t lo = mem_read(cpu->bus, base);
  uint8_t hi = mem_read(cpu->bus, (uint8_t)(base + 1));
  return (uint16_t)((hi << 8) | lo);
}

static uint16_t get_address_indirect_y(Cpu* cpu) {
  uint8_t base = mem_read(cpu->bus, cpu->pc++);

  uint8_t lo = mem_read(cpu->bus, base);
  uint8_t hi = mem_read(cpu->bus, (uint8_t)(base + 1));
  uint16_t final = (uint16_t)(((hi << 8) | lo) + cpu->y);

  if (pages_differ((uint16_t)((hi << 8) | lo), final)) {
    cpu->bounds_crossed = true;
  }

  return final;
}

static uint16_t get_address(Cpu* cpu, AddressingMode mode) {
  switch (mode) {
    case Implied:
    case Accumulator:
      return 0;
    case Immediate:
      return get_address_immediate(cpu);
    case ZeroPage:
      return get_address_zeropage(cpu);
    case ZeroPageX:
      return get_address_zeropage_x(cpu);
    case ZeroPageY:
      return get_address_zeropage_y(cpu);
    case Relative:
      return get_address_relative(cpu);
    case Absolute:
      return get_address_absolute(cpu);
    case AbsoluteX:
      return get_address_absolute_x(cpu);
    case AbsoluteY:
      return get_address_absolute_y(cpu);
    case Indirect:
      return get_address_indirect(cpu);
    case IndirectX:
      return get_address_indirect_x(cpu);
    case IndirectY:
      return get_address_indirect_y(cpu);
  }
}

// clang-format off
static const int FLAG_STATUS_NEGATIVE          = 0b10000000;
static const int FLAG_STATUS_OVERFLOW          = 0b01000000;
static const int FLAG_STATUS_B2                = 0b00100000;
static const int FLAG_STATUS_B1                = 0b00010000;
static const int FLAG_STATUS_DECIMAL           = 0b00001000;
static const int FLAG_STATUS_INTERRUPT_DISABLE = 0b00000100;
static const int FLAG_STATUS_ZERO              = 0b00000010;
static const int FLAG_STATUS_CARRY             = 0b00000001;
// clang-format on

static void set_negative_and_zero(Cpu* cpu, uint8_t num) {
  set_flag(&cpu->status, FLAG_STATUS_NEGATIVE, num & 0x80);
  set_flag(&cpu->status, FLAG_STATUS_ZERO, num == 0);
}

static const int STACK_START = 0x0100;

static void stack_push(Cpu* cpu, uint8_t val) {
  mem_write(cpu->bus, STACK_START + cpu->sp, val);
  cpu->sp--;
}

static void stack_push_16(Cpu* cpu, uint16_t val) {
  stack_push(cpu, val >> 8);
  stack_push(cpu, val & 0xFF);
}

static uint8_t stack_pop(Cpu* cpu) {
  cpu->sp++;
  return mem_read(cpu->bus, STACK_START + cpu->sp);
}

static uint16_t stack_pop_16(Cpu* cpu) {
  uint8_t lo = stack_pop(cpu);
  uint8_t hi = stack_pop(cpu);
  return (uint16_t)(hi << 8) | lo;
}

// === CPU INSTRUCTIONS ==

static void adc(Cpu* cpu, uint8_t val) {
  // Add the accumulator, value, and carry bit if needed
  uint16_t result = cpu->a + val + (cpu->status & FLAG_STATUS_CARRY);
  // Set overflow
  set_flag(&cpu->status, FLAG_STATUS_OVERFLOW,
           (cpu->a ^ result) & (val ^ result) & 0x80);

  cpu->a = (uint8_t)result;
  set_negative_and_zero(cpu, cpu->a);

  set_flag(&cpu->status, FLAG_STATUS_CARRY, result > 255);
}

// clang-format off
// For some reason clang-format doesn't like the function name "and"
static void and(Cpu* cpu, uint8_t val) {
  cpu->a &= val;
  set_negative_and_zero(cpu, cpu->a);
}
// clang-format on

static void rol_a(Cpu* cpu) {
  uint8_t carry = cpu->status & FLAG_STATUS_CARRY;
  set_flag(&cpu->status, FLAG_STATUS_CARRY, cpu->a & 0x80);
  cpu->a <<= 1;
  cpu->a |= carry;
  set_negative_and_zero(cpu, cpu->a);
}

static void rol(Cpu* cpu, uint16_t addr) {
  uint8_t val = mem_read(cpu->bus, addr);
  uint8_t carry = cpu->status & FLAG_STATUS_CARRY;
  set_flag(&cpu->status, FLAG_STATUS_CARRY, val & 0x80);
  val <<= 1;
  val |= carry;
  mem_write(cpu->bus, addr, val);
  set_negative_and_zero(cpu, val);
}

static void ror_a(Cpu* cpu) {
  uint8_t carry = cpu->status & FLAG_STATUS_CARRY;
  set_flag(&cpu->status, FLAG_STATUS_CARRY, cpu->a & 1);
  cpu->a >>= 1;
  cpu->a |= carry << 7;
  set_negative_and_zero(cpu, cpu->a);
}

static void ror(Cpu* cpu, uint16_t addr) {
  uint8_t val = mem_read(cpu->bus, addr);
  uint8_t carry = cpu->status & FLAG_STATUS_CARRY;
  set_flag(&cpu->status, FLAG_STATUS_CARRY, val & 1);
  val >>= 1;
  val |= carry << 7;
  mem_write(cpu->bus, addr, val);
  set_negative_and_zero(cpu, val);
}

static void rti(Cpu* cpu) {
  cpu->status = (cpu->status & ~0xCF) | (stack_pop(cpu) & 0xCF);
  cpu->pc = stack_pop_16(cpu);
}

static void rts(Cpu* cpu) {
  uint16_t pc = stack_pop_16(cpu);
  cpu->pc = pc + 1;
}

static void sbc(Cpu* cpu, uint8_t val) { adc(cpu, ~val); }

static void sec(Cpu* cpu) { set_flag(&cpu->status, FLAG_STATUS_CARRY, true); }

static void asl_a(Cpu* cpu) {
  set_flag(&cpu->status, FLAG_STATUS_CARRY, cpu->a & 0x80);
  cpu->a <<= 1;
  set_negative_and_zero(cpu, cpu->a);
}

static void asl(Cpu* cpu, uint16_t addr) {
  uint8_t val = mem_read(cpu->bus, addr);
  set_flag(&cpu->status, FLAG_STATUS_CARRY, val & 0x80);
  val <<= 1;
  mem_write(cpu->bus, addr, val);
  set_negative_and_zero(cpu, val);
}

static void bit(Cpu* cpu, uint8_t val) {
  uint8_t anded = cpu->a & val;
  set_flag(&cpu->status, FLAG_STATUS_ZERO, anded == 0);

  set_flag(&cpu->status, FLAG_STATUS_NEGATIVE, (val & 0x80) >> 7);
  set_flag(&cpu->status, FLAG_STATUS_OVERFLOW, (val & 0x40) >> 6);
}

static void compare(Cpu* cpu, uint8_t a, uint8_t b) {
  int result = a - b;
  set_flag(&cpu->status, FLAG_STATUS_CARRY, a >= b);
  set_flag(&cpu->status, FLAG_STATUS_ZERO, a == b);
  set_flag(&cpu->status, FLAG_STATUS_NEGATIVE, result & 0x80);
}

static void dec(Cpu* cpu, uint16_t addr) {
  uint8_t val = mem_read(cpu->bus, addr);
  mem_write(cpu->bus, addr, --val);
  set_negative_and_zero(cpu, val);
}

static void eor(Cpu* cpu, uint8_t val) {
  cpu->a ^= val;
  set_negative_and_zero(cpu, cpu->a);
}

static void inc(Cpu* cpu, uint16_t addr) {
  uint8_t val = mem_read(cpu->bus, addr);
  mem_write(cpu->bus, addr, ++val);
  set_negative_and_zero(cpu, val);
}

static void lda(Cpu* cpu, uint8_t val) {
  cpu->a = val;
  set_negative_and_zero(cpu, cpu->a);
}

static void ldx(Cpu* cpu, uint8_t val) {
  cpu->x = val;
  set_negative_and_zero(cpu, cpu->x);
}

static void ldy(Cpu* cpu, uint8_t val) {
  cpu->y = val;
  set_negative_and_zero(cpu, cpu->y);
}

static void lsr_a(Cpu* cpu) {
  set_flag(&cpu->status, FLAG_STATUS_CARRY, cpu->a & 1);
  cpu->a >>= 1;
  set_negative_and_zero(cpu, cpu->a);
}

static void lsr(Cpu* cpu, uint16_t addr) {
  uint8_t val = mem_read(cpu->bus, addr);
  set_flag(&cpu->status, FLAG_STATUS_CARRY, val & 1);
  val >>= 1;
  mem_write(cpu->bus, addr, val);
  set_negative_and_zero(cpu, val);
}

static void inx(Cpu* cpu) { set_negative_and_zero(cpu, ++cpu->x); }

static void iny(Cpu* cpu) { set_negative_and_zero(cpu, ++cpu->y); }

static void jmp(Cpu* cpu, uint16_t addr) { cpu->pc = addr; }

static void jsr(Cpu* cpu, uint16_t addr) {
  stack_push_16(cpu, cpu->pc - 1);
  cpu->pc = addr;
}

static void ora(Cpu* cpu, uint8_t val) {
  cpu->a |= val;
  set_negative_and_zero(cpu, cpu->a);
}

static void pha(Cpu* cpu) { stack_push(cpu, cpu->a); }

static void php(Cpu* cpu) { stack_push(cpu, cpu->status | 0b00110000); }

static void pla(Cpu* cpu) {
  cpu->a = stack_pop(cpu);
  set_negative_and_zero(cpu, cpu->a);
}

static void plp(Cpu* cpu) {
  cpu->status = (cpu->status & ~0xCF) | (stack_pop(cpu) & 0xCF);
}

static void dex(Cpu* cpu) { set_negative_and_zero(cpu, --cpu->x); }

static void dey(Cpu* cpu) { set_negative_and_zero(cpu, --cpu->y); }

static void sed(Cpu* cpu) { set_flag(&cpu->status, FLAG_STATUS_DECIMAL, true); }

static void sei(Cpu* cpu) {
  set_flag(&cpu->status, FLAG_STATUS_INTERRUPT_DISABLE, true);
}

static void sta(Cpu* cpu, uint16_t addr) { mem_write(cpu->bus, addr, cpu->a); }

static void stx(Cpu* cpu, uint16_t addr) { mem_write(cpu->bus, addr, cpu->x); }

static void sty(Cpu* cpu, uint16_t addr) { mem_write(cpu->bus, addr, cpu->y); }

static void tax(Cpu* cpu) {
  cpu->x = cpu->a;
  set_negative_and_zero(cpu, cpu->x);
}

static void tay(Cpu* cpu) {
  cpu->y = cpu->a;
  set_negative_and_zero(cpu, cpu->y);
}

static void tsx(Cpu* cpu) {
  cpu->x = cpu->sp;
  set_negative_and_zero(cpu, cpu->x);
}

static void txa(Cpu* cpu) {
  cpu->a = cpu->x;
  set_negative_and_zero(cpu, cpu->a);
}

static void txs(Cpu* cpu) { cpu->sp = cpu->x; }

static void tya(Cpu* cpu) {
  cpu->a = cpu->y;
  set_negative_and_zero(cpu, cpu->a);
}

static void branch(Cpu* cpu, uint16_t addr, bool condition) {
  if (condition) {
    cpu->cycles_remaining++;
    cpu->pc = addr + 1;
  } else {
    // Unset boundary cross as we don't actually cross
    cpu->bounds_crossed = false;
  }
}

// == Undocumented Instructions ==
static void lax(Cpu* cpu, uint8_t val) {
  cpu->a = val;
  cpu->x = val;
  set_negative_and_zero(cpu, val);
}

static void sax(Cpu* cpu, uint16_t addr) {
  uint8_t result = cpu->x & cpu->a;
  mem_write(cpu->bus, addr, result);
}

static void dcp(Cpu* cpu, uint16_t addr) {
  uint8_t num = mem_read(cpu->bus, addr) - 1;
  mem_write(cpu->bus, addr, num);

  compare(cpu, cpu->a, num);
}

static void isb(Cpu* cpu, uint16_t addr) {
  uint8_t val = mem_read(cpu->bus, addr);
  mem_write(cpu->bus, addr, ++val);
  set_negative_and_zero(cpu, val);
  sbc(cpu, val);
}

static void slo(Cpu* cpu, uint16_t addr) {
  asl(cpu, addr);
  cpu->a |= mem_read(cpu->bus, addr);
  set_negative_and_zero(cpu, cpu->a);
}

static void rla(Cpu* cpu, uint16_t addr) {
  rol(cpu, addr);
  cpu->a &= mem_read(cpu->bus, addr);
  set_negative_and_zero(cpu, cpu->a);
}

static void rra(Cpu* cpu, uint16_t addr) {
  ror(cpu, addr);
  adc(cpu, mem_read(cpu->bus, addr));
}

static void sre(Cpu* cpu, uint16_t addr) {
  lsr(cpu, addr);
  eor(cpu, mem_read(cpu->bus, addr));
}

static void arr(Cpu* cpu, uint8_t val) {
  cpu->a &= val;
  set_negative_and_zero(cpu, cpu->a);
  ror_a(cpu);

  bool b5 = cpu->a & 0b00100000;
  bool b6 = cpu->a & 0b01000000;
  if (b5 && b6) {
    set_flag(&cpu->status, FLAG_STATUS_CARRY, true);
    set_flag(&cpu->status, FLAG_STATUS_OVERFLOW, false);
  } else if (!b5 && !b6) {
    set_flag(&cpu->status, FLAG_STATUS_CARRY, false);
    set_flag(&cpu->status, FLAG_STATUS_OVERFLOW, false);
  } else if (b5 && !b6) {
    set_flag(&cpu->status, FLAG_STATUS_CARRY, false);
    set_flag(&cpu->status, FLAG_STATUS_OVERFLOW, true);
  } else {
    set_flag(&cpu->status, FLAG_STATUS_CARRY, true);
    set_flag(&cpu->status, FLAG_STATUS_OVERFLOW, true);
  }
}

static void asr(Cpu* cpu, uint8_t val) {
  cpu->a &= val;
  set_negative_and_zero(cpu, cpu->a);
  lsr_a(cpu);
}

static void atx(Cpu* cpu, uint8_t val) {
  cpu->a &= val;
  set_negative_and_zero(cpu, cpu->a);
  cpu->x = cpu->a;
}

static void axa(Cpu* cpu, uint16_t addr) {
  mem_write(cpu->bus, cpu->x & cpu->a & (addr >> 8), (uint8_t)addr);
}

static void axs(Cpu* cpu, uint8_t val) {
  cpu->x &= cpu->a;
  cpu->x -= val;
  if (cpu->x >= val) {
    set_flag(&cpu->status, FLAG_STATUS_CARRY, true);
  }
  set_negative_and_zero(cpu, cpu->x);
}

static void lar(Cpu* cpu, uint16_t addr) {
  uint8_t result = mem_read(cpu->bus, addr) & cpu->sp;
  cpu->a = result;
  cpu->x = result;
  cpu->sp = result;
  set_negative_and_zero(cpu, result);
}

static void sxa(Cpu* cpu, uint16_t addr) {
  uint8_t result = cpu->x & ((uint8_t)(addr >> 8) + 1);
  set_negative_and_zero(cpu, result);
  mem_write(cpu->bus, addr, result);
}

static void sya(Cpu* cpu, uint16_t addr) {
  uint8_t result = cpu->y & ((uint8_t)(addr >> 8) + 1);
  set_negative_and_zero(cpu, result);
  mem_write(cpu->bus, addr, result);
}

static void xaa(Cpu* cpu, uint8_t val) {
  cpu->a = cpu->x;
  cpu->a &= val;
  set_negative_and_zero(cpu, cpu->a);
}

static void xas(Cpu* cpu, uint16_t addr) {
  cpu->sp = cpu->x & cpu->a;
  uint8_t result = cpu->sp & ((uint8_t)(addr >> 8) + 1);
  mem_write(cpu->bus, addr, result);
  set_negative_and_zero(cpu, result);
}

static void aac(Cpu* cpu, uint8_t val) {
  cpu->a &= val;
  set_negative_and_zero(cpu, cpu->a);
  set_flag(&cpu->status, FLAG_STATUS_CARRY, cpu->status & FLAG_STATUS_NEGATIVE);
}

void cpu_execute(Cpu* cpu) {
  // If we're waiting for cycles to pass,
  // let them pass and don't run any more codef
  if (cpu->cycles_remaining) {
    cpu->cycles_total++; // These do count as cycles
    cpu->cycles_remaining--;
    return;
  }

  print_debug(cpu);

  cpu->bounds_crossed = false;

  // Convenience
  Bus* bus = cpu->bus;

  // Read the next opcode
  uint8_t opcode = mem_read(bus, cpu->pc++);

  // Fetch opcode info
  uint8_t cycles_num = (uint8_t)OPCODES[opcode][3];
  AddressingMode mode = (AddressingMode)OPCODES[opcode][2];

  // Address of the operation's input
  uint16_t addr = get_address(cpu, mode);

  // Increment cycles
  cpu->cycles_remaining += cycles_num;

  switch (opcode) {
    // ADC
    case 0x69:
    case 0x65:
    case 0x75:
    case 0x6D:
    case 0x7D:
    case 0x79:
    case 0x61:
    case 0x71:
      adc(cpu, mem_read(bus, addr));
      break;
    // AND
    case 0x29:
    case 0x25:
    case 0x35:
    case 0x2D:
    case 0x3D:
    case 0x39:
    case 0x21:
    case 0x31:
      and(cpu, mem_read(bus, addr));
      break;
    // ROL
    case 0x2A:
      rol_a(cpu);
      break;
    case 0x26:
    case 0x36:
    case 0x2E:
    case 0x3E:
      rol(cpu, addr);
      break;
    // ROR
    case 0x6A:
      ror_a(cpu);
      break;
    case 0x66:
    case 0x76:
    case 0x6E:
    case 0x7E:
      ror(cpu, addr);
      break;
    // RTI
    case 0x40:
      rti(cpu);
      break;
    // RTS
    case 0x60:
      rts(cpu);
      break;
    // SBC
    case 0xE9:
    case 0xE5:
    case 0xF5:
    case 0xED:
    case 0xFD:
    case 0xF9:
    case 0xE1:
    case 0xF1:
      sbc(cpu, mem_read(bus, addr));
      break;
    // SEC
    case 0x38:
      sec(cpu);
      break;
    // ASL
    case 0x0A:
      asl_a(cpu);
      break;
    case 0x06:
    case 0x16:
    case 0x0E:
    case 0x1E:
      asl(cpu, addr);
      break;
    // BCC
    case 0x90:
      branch(cpu, addr, !(cpu->status & 0x1));
      break;
    // BCS
    case 0xB0:
      branch(cpu, addr, cpu->status & 0x1);
      break;
    // BEQ
    case 0xF0:
      branch(cpu, addr, cpu->status & FLAG_STATUS_ZERO);
      break;
    // BIT
    case 0x24:
    case 0x2C:
      bit(cpu, mem_read(bus, addr));
      break;
    // BMI
    case 0x30:
      branch(cpu, addr, cpu->status & FLAG_STATUS_NEGATIVE);
      break;
    // BNE
    case 0xD0:
      branch(cpu, addr, !(cpu->status & FLAG_STATUS_ZERO));
      break;
    // BPL
    case 0x10:
      branch(cpu, addr, !(cpu->status & FLAG_STATUS_NEGATIVE));
      break;
    // BVC
    case 0x50:
      branch(cpu, addr, !(cpu->status & FLAG_STATUS_OVERFLOW));
      break;
    // BVS
    case 0x70:
      branch(cpu, addr, cpu->status & FLAG_STATUS_OVERFLOW);
      break;
    // CLC
    case 0x18:
      set_flag(&cpu->status, FLAG_STATUS_CARRY, false);
      break;
    // CLD
    case 0xD8:
      set_flag(&cpu->status, FLAG_STATUS_DECIMAL, false);
      break;
    // CLV
    case 0xB8:
      set_flag(&cpu->status, FLAG_STATUS_OVERFLOW, false);
      break;
    // CMP
    case 0xC9:
    case 0xC5:
    case 0xD5:
    case 0xCD:
    case 0xDD:
    case 0xD9:
    case 0xC1:
    case 0xD1:
      compare(cpu, cpu->a, mem_read(bus, addr));
      break;
    // CPX
    case 0xE0:
    case 0xE4:
    case 0xEC:
      compare(cpu, cpu->x, mem_read(bus, addr));
      break;
    // CPY
    case 0xC0:
    case 0xC4:
    case 0xCC:
      compare(cpu, cpu->y, mem_read(bus, addr));
      break;
    // DEC
    case 0xC6:
    case 0xD6:
    case 0xCE:
    case 0xDE:
      dec(cpu, addr);
      break;
    // EOR
    case 0x49:
    case 0x45:
    case 0x55:
    case 0x4D:
    case 0x5D:
    case 0x59:
    case 0x41:
    case 0x51:
      eor(cpu, mem_read(bus, addr));
      break;
    // INC
    case 0xE6:
    case 0xF6:
    case 0xEE:
    case 0xFE:
      inc(cpu, addr);
      break;
    // INX
    case 0xE8:
      inx(cpu);
      break;
    // INY
    case 0xC8:
      iny(cpu);
      break;
    // JMP
    case 0x4C:
    case 0x6C:
      jmp(cpu, addr);
      break;
    // JSR
    case 0x20:
      jsr(cpu, addr);
      break;
    // LDA
    case 0xA9:
    case 0xA5:
    case 0xB5:
    case 0xAD:
    case 0xBD:
    case 0xB9:
    case 0xA1:
    case 0xB1:
      lda(cpu, mem_read(bus, addr));
      break;
    // LDX
    case 0xA2:
    case 0xA6:
    case 0xB6:
    case 0xAE:
    case 0xBE:
      ldx(cpu, mem_read(bus, addr));
      break;
    // LDY
    case 0xA0:
    case 0xA4:
    case 0xB4:
    case 0xAC:
    case 0xBC:
      ldy(cpu, mem_read(bus, addr));
      break;
    // LSR
    case 0x4A:
      lsr_a(cpu);
      break;
    case 0x46:
    case 0x56:
    case 0x4E:
    case 0x5E:
      lsr(cpu, addr);
      break;
    // NOP
    case 0xEA:
      break;
    // ORA
    case 0x09:
    case 0x05:
    case 0x15:
    case 0x0D:
    case 0x1D:
    case 0x19:
    case 0x01:
    case 0x11:
      ora(cpu, mem_read(bus, addr));
      break;
    // PHA
    case 0x48:
      pha(cpu);
      break;
    // PHP
    case 0x08:
      php(cpu);
      break;
    // PLA
    case 0x68:
      pla(cpu);
      break;
    // PLP
    case 0x28:
      plp(cpu);
      break;
    // DEX
    case 0xCA:
      dex(cpu);
      break;
    // DEY
    case 0x88:
      dey(cpu);
      break;
    // SED
    case 0xF8:
      sed(cpu);
      break;
    // SEI
    case 0x78:
      sei(cpu);
      break;
    // STA
    case 0x85:
    case 0x95:
    case 0x8D:
    case 0x9D:
    case 0x99:
    case 0x81:
    case 0x91:
      sta(cpu, addr);
      break;
    // STX
    case 0x86:
    case 0x96:
    case 0x8E:
      stx(cpu, addr);
      break;
    // STY
    case 0x84:
    case 0x94:
    case 0x8C:
      sty(cpu, addr);
      break;
    // TAX
    case 0xAA:
      tax(cpu);
      break;
    // TAY
    case 0xA8:
      tay(cpu);
      break;
    // TSX
    case 0xBA:
      tsx(cpu);
      break;
    // TXA
    case 0x8A:
      txa(cpu);
      break;
    // TXS
    case 0x9A:
      txs(cpu);
      break;
    // TYA
    case 0x98:
      tya(cpu);
      break;
      // Undocumented
    // https://www.nesdev.com/undocumented_opcodes.txt

    // NOP
    case 0x1A:
    case 0x3A:
    case 0x5A:
    case 0x7A:
    case 0xDA:
    case 0xFA:
      break;
    // DOP
    case 0x04:
    case 0x14:
    case 0x34:
    case 0x44:
    case 0x54:
    case 0x64:
    case 0x74:
    case 0x80:
    case 0x82:
    case 0x89:
    case 0xC2:
    case 0xD4:
    case 0xE2:
    case 0xF4:
      break;
    // TOP
    case 0x0C:
    case 0x1C:
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
      break;
    // LAX
    case 0xA7:
    case 0xB7:
    case 0xAF:
    case 0xBF:
    case 0xA3:
    case 0xB3:
      lax(cpu, mem_read(bus, addr));
      break;
    // SAX
    case 0x87:
    case 0x97:
    case 0x83:
    case 0x8F:
      sax(cpu, addr);
      break;
    // *SBC (same as 0xE9)
    case 0xEB:
      sbc(cpu, mem_read(bus, addr));
      break;
    // DCP
    case 0xC7:
    case 0xD7:
    case 0xCF:
    case 0xDF:
    case 0xDB:
    case 0xC3:
    case 0xD3:
      dcp(cpu, addr);
      break;
    // ISB
    case 0xE7:
    case 0xF7:
    case 0xEF:
    case 0xFF:
    case 0xFB:
    case 0xE3:
    case 0xF3:
      isb(cpu, addr);
      break;
    // SLO
    case 0x07:
    case 0x17:
    case 0x0F:
    case 0x1F:
    case 0x1B:
    case 0x03:
    case 0x13:
      slo(cpu, addr);
      break;
    // RLA
    case 0x27:
    case 0x37:
    case 0x2F:
    case 0x3F:
    case 0x3B:
    case 0x23:
    case 0x33:
      rla(cpu, addr);
      break;
    // RRA
    case 0x67:
    case 0x77:
    case 0x6F:
    case 0x7F:
    case 0x7B:
    case 0x63:
    case 0x73:
      rra(cpu, addr);
      break;
    // SRE
    case 0x47:
    case 0x57:
    case 0x4F:
    case 0x5F:
    case 0x5B:
    case 0x43:
    case 0x53:
      sre(cpu, addr);
      break;
    // ARR
    case 0x6B:
      arr(cpu, mem_read(bus, addr));
      break;
    // ASR
    case 0x4B:
      asr(cpu, mem_read(bus, addr));
      break;
    // ATX
    case 0xAB:
      atx(cpu, mem_read(bus, addr));
      break;
    // AXA
    case 0x9F:
      axa(cpu, addr);
      break;
    case 0x93:
      axa(cpu, addr);
      break;
    // AXS
    case 0xCB:
      axs(cpu, mem_read(bus, addr));
      break;
    // KIL
    case 0x02:
    case 0x12:
    case 0x22:
    case 0x32:
    case 0x42:
    case 0x52:
    case 0x62:
    case 0x72:
    case 0x92:
    case 0xB2:
    case 0xD2:
    case 0xF2:
      break;
    // LAR
    case 0xBB:
      lar(cpu, mem_read(bus, addr));
      break;
    // SXA
    case 0x9E:
      sxa(cpu, mem_read(bus, addr));
      break;
    // SYA
    case 0x9C:
      sya(cpu, mem_read(bus, addr));
      break;
    // XAA
    case 0x8B:
      xaa(cpu, mem_read(bus, addr));
      break;
    // XAS
    case 0x9B:
      xas(cpu, mem_read(bus, addr));
      break;
    // AAC
    case 0x0B:
    case 0x2B:
      aac(cpu, mem_read(bus, addr));
      break;
    default:
      printf("Unknown opcode %02X\n", opcode);
      exit(0);
  }

  if (cpu->bounds_crossed && OPCODES[opcode][4]) {
    cpu->cycles_remaining++;
  }

  // By getting to here we've already completed
  // one cycle, so let's get rid of it
  cpu->cycles_remaining--;
  cpu->cycles_total++;
}
