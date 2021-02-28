#ifndef CAPSTONE_BLUETOOTH_H
#define CAPSTONE_BLUETOOTH_H

#include <stdint.h>

#include "capstone.h"

int bt_parse_msg(const char *buf, const int length, const int emic);

#endif
