#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/termios.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "capstone.h"
#include "writer_serial.h"
#include "settings.h"
#include "nano.h"

static volatile int keep_running = 1;

#ifdef DEBUG
void print_buffer_string(const char *buffer, const int size) {
  char *str_buf = malloc(size + 1);
  memcpy(str_buf, buffer, size);
  str_buf[size] = '\0';
  DPRINTF("%s\n", str_buf);
  free(str_buf);
}
#endif


void sigint_handler(int dummy) {
    UNUSED(dummy);
    keep_running = 0;
}

void set_handler(void) {
    struct sigaction act;
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, NULL);
}

int setup_arduino_serial(int fd) {
    struct termios tm;
    if(tcgetattr(fd, &tm) < 0) {
        perror("Error getting serial line settings");
        return -1;
    }

    // 9600 baud rate
    if(cfsetospeed(&tm, B9600) < 0) {
        perror("Error setting output baud rate");
        return -1;
    }
    if(cfsetispeed(&tm, B9600) < 0) {
        perror("Error setting input baud rate");
        return -1;
    }

    tm.c_iflag |= ICANON;   // canonical mode

    tm.c_cflag &= ~PARENB;  // No parity bit
    tm.c_cflag &= ~CSTOPB;  // 1 stop bit
    tm.c_cflag &= ~CSIZE;   // Clear data size mask
    tm.c_cflag |= CS8;      // 8 data bits
    tm.c_cflag |= CLOCAL;   // ignore modem control lines
    tm.c_cflag |= CREAD;    // Enable receiver

    tm.c_oflag &= ~OPOST;   // raw output

    if(tcsetattr(fd, TCSANOW, &tm) < 0) {
        perror("Error setting serial line settings");
        return -1;
    }
    return 0;
}

int text_to_voice(const char *data, const uint8_t length){
  if(length >= MAX_SIZE - HDR_SIZE) return -1;
  char buffer[MAX_SIZE];
  buffer[TYPE_IDX] = TTS_MSG_TYPE;
  buffer[LEN_IDX] = length + HDR_SIZE;
  int buf_idx = HDR_SIZE;
  memcpy(buffer + buf_idx, data, length);
  buf_idx += length;
#ifdef DEBUG
  DPRINTF("Sending To Arduino: ");
  print_buffer_string(buffer + HDR_SIZE, buf_idx - HDR_SIZE);
  DPRINTF("\n");
#endif
  int rv = write(serial, buffer, buf_idx);
  if(rv != buf_idx) {
      perror("Failed to write over serial");
      return -1;
  }
  return 0;
}

int parse_bt(const char *buffer, const uint8_t length){
  int ret = 0;
  for(int i = 0; i < length; ++i) {
    if(buffer[i] == '\n') {
      int rv = text_to_voice(buffer + ret, i - ret);
      if(rv < 0) {
          return rv;
      }
      ret += i;
    }
  }
  return ret + 1;
}

int parse_serial(const char *buffer, const uint8_t length){
  if(length < HDR_SIZE) return 0;
  if(length < buffer[LEN_IDX]) return 0;

    switch(buffer[TYPE_IDX]) {
    case BT_MSG_TYPE:
        // TODO
        break;
    case TTS_MSG_TYPE:
#ifdef DEBUG
        DPRINTF("Received TTS msg: ");
        print_buffer_string(buffer + HDR_SIZE, buffer[LEN_IDX] - HDR_SIZE);
        DPRINTF("\n");
#endif
        break;
    default:
      return 0;
    }
    return buffer[LEN_IDX];
}

int main(void) {
    // Initialize
    /*
    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
      perror("Failed to created epoll instance");
      return 1;
    }
    // Add stdin to epoll interest list
    struct epoll_event ee;
    ee.events = EPOLLIN;
    ee.data.fd = STDIN_FILENO;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ee) < 0) {
      perror("Failed to control epoll instance");
      return 1;
    }

    // Add serial to epoll interest list
    ee.events = EPOLLIN;
    ee.data.fd = serial;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serial, &ee) < 0) {
      perror("Failed to control epoll instance");
      return 1;
    }
    */
    int ret = 0;
    Nano nano;
    if(nano_init(&nano) != 0){
        return 1;
    }
    
    DPRINTF("Initialized!\n");

    if(nano_start_reading(&nano) != 0) {
        nano_cleanup(&nano);
        return 1;
    }

    while(keep_running) {
        // TODO
    }

    // Clean-up
    DPRINTF("Clean up!\n");

    /* close(epoll_fd); */
    return ret;
}
