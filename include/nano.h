#ifndef NANO_H
#define NANO_H

#include "tm_reader.h"

#define NANO_BUFFER_SIZE   512

#define ANTENNA_POWER     1000
#define BAUDRATE          9600

#define EPC_OFFSET          31

#define VERSION_TAG       0x01
#define END_OF_TAG        0xFF
#define WOMENS_BATHROOM   0x11
#define MENS_BATHROOM     0x12
#define UNISEX_BATHROOM   0x13
#define CLASSROOM         0x21
#define OFFICE            0x22
#define FLOOR             0x31
#define ELEVATOR          0x32
#define STAIRCASE         0x33
#define EXIT              0xF1


typedef TMR_Reader Nano;

int nano_init(Nano *nano);
int nano_cleanup(Nano *nano);
int nano_start_reading(Nano *nano);
int nano_stop_reading(Nano *nano);

#endif
