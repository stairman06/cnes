#include "bus.h"
Bus bus_init(unsigned char* rom) { return (Bus){.rom = rom}; }
