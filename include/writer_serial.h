#ifndef WRITER_SERIAL_H
#define WRITER_SERIAL_H

#include <stdlib.h>

#define BUF_SIZE       512
#define HDR_SIZE         2
#define TYPE_IDX         0
#define LEN_IDX          1

#define TTS_MSG_TYPE  0x01
#define BT_MSG_TYPE   0x02

typedef struct writer_serial {
    int fd;
    size_t b_idx;
    char recv_buffer[BUF_SIZE];
} w_serial;

#endif
