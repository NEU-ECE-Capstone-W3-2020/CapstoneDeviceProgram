#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "capstone.h"
#include "emic.h"

int init_emic(const char *sysfspath) {
    int rv;
    struct termios tm;
    int fd = open(sysfspath, O_RDWR);
    if(fd < 0) {
        perror("Error opening emic");
        return fd;
    }

    // get serial settings
    if((rv = tcgetattr(fd, &tm)) < 0){
        perror("Error getting serial settings");
        return rv;
    }

    // Set baud rate
    if((rv = cfsetispeed(&tm, B9600)) < 0) {
        perror("Error setting input baud rate");
        return rv;
    }
    if((rv = cfsetospeed(&tm, B9600)) < 0) {
        perror("Error setting output baud rate");
        return rv;
    }

    // Set more settings
    tm.c_iflag &= ~ICANON;   // canonical mode off

    tm.c_cflag &= ~PARENB;  // No parity bit
    tm.c_cflag &= ~CSTOPB;  // 1 stop bit
    tm.c_cflag &= ~CSIZE;   // Clear data size mask
    tm.c_cflag |= CS8;      // 8 data bits
    tm.c_cflag |= CLOCAL;   // ignore modem control lines
    tm.c_cflag |= CREAD;    // Enable receiver

    tm.c_oflag &= ~OPOST;   // raw output

    // Set the new serial settings
    if((rv = tcsetattr(fd, TCSANOW, &tm)) < 0) {
        perror("Error setting serial settings");
        return rv;
    }

    return fd;
}

int emic_text_to_speech(int emic, const char *buf, const size_t length) {
    char lbuf[1024];
    char lb_idx = 0;
    int rv = 0;
    int ret = 0;

    if(length > 1023) {
        printf("Message too long\n");
        return -1;
    }

    lbuf[0] = 'S';
    memcpy(lbuf + 1, buf, length);
    lb_idx = length + 1;

    if((rv = write(emic, lbuf, lb_idx)) < lb_idx) {
        perror("Error writing to emic");
        return rv;
    }
    ret = rv - 1;
    //  wait for response
    memset(lbuf, 0, 1024);
    lb_idx = 0;
    if((rv = read(emic, lbuf, 1024)) <= 0) {
        perror("Error reading from emic");
        return rv;
    }
    lb_idx = rv;
    // ensure we got ':' as the response
    if(buf[0] != ':') {
        printf("Unexpected response from emic: %c", lbuf[0]);
        return -1;
    }

    return ret;
}
