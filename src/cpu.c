#include "cpu.h"

Cpu cpu_init(Bus* bus) { return (Cpu){.bus = bus}; }

void cpu_execute(Cpu* cpu) {}
