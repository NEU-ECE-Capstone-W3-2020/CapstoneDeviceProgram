#ifndef CAPSTONE_H
#define CAPSTONE_H

#include <stdio.h>

#define DEBUG

#define UNUSED(arg) (void) arg
#ifdef DEBUG
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...) do{}while(0)
#endif

#endif
