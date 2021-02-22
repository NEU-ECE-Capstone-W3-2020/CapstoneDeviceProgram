#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

#include "pigpiod_if2.h"

#define DEBUG
#define BAUDRATE      9600
#define MAX_SIZE       256
#define HDR_SIZE         2
#define TYPE_IDX         0
#define LEN_IDX          1

#define TTS_MSG_TYPE  0x01
#define BT_MSG_TYPE   0x02

#define UNUSED(arg) (void) arg

enum Mode {
    Intentional,
    Explore
};

enum State {
    Idle,
    WaitingForGesture
};

static volatile int keep_running = 1;
static enum Mode mode = Explore;
static enum State state = Idle;
static int pi = 0;
static int serial = 0;
static int epoll_fd = 0;

#ifdef DEBUG
void print_buffer_string(const char *buffer, const int size) {
  char *str_buf = malloc(size + 1);
  memcpy(str_buf, buffer, size);
  str_buf[size] = '\0';
  printf("%s\n", str_buf);
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

void toggle_mode(void){
    switch(mode){
        case Intentional:
            mode = Explore;
            break;
        case Explore:
            mode = Intentional;
            break;
    }
}

int text_to_voice(const char *data, const uint8_t length){
  if(length >= MAX_SIZE - HDR_SIZE) return -1;
  char buffer[MAX_SIZE];
  uint32_t buf_idx = 0;
  buffer[buf_idx++] = TTS_MSG_TYPE;
  buffer[buf_idx++] = length + HDR_SIZE;
  memcpy(buffer + buf_idx, data, length);
  buf_idx += length;
#ifdef DEBUG
  print_buffer_string(buffer, buf_idx);
#endif
  int rv = serial_write(pi, serial, buffer, buf_idx);
  if(rv < 0) {
    fprintf(stderr, "Failed to write over serial: %s\n", pigpio_error(rv));
    fflush(stderr);
  }
  return -1;
}

int parse_bt(const char *buffer, const uint8_t length){
  int ret = 0;
  for(int i = 0; i < length; ++i) {
    if(buffer[i] == '\n') {
#ifdef DEBUG
      print_buffer_string(buffer + ret, i - ret);
#endif
      int rv = text_to_voice(buffer + ret, i - ret);
      if(rv < 0) {
          return rv;
      }
      ret += i;
    }
  }
  return ret;
}

int parse_serial(const char *buffer, const uint8_t length){
  if(length < HDR_SIZE) return 0;
  switch(buffer[TYPE_IDX]) {
    case BT_MSG_TYPE:
      if(length < buffer[LEN_IDX]) return 0;
      text_to_voice(buffer + HDR_SIZE, buffer[LEN_IDX] - HDR_SIZE);
      return buffer[LEN_IDX];
    default:
      return 0;
  }
}

int init(void){
    // Set control c to call our custom handler
    set_handler();

    pi = pigpio_start(NULL, NULL);
    if(pi < 0){
        fprintf(stderr, "Failed to connect to pigpi daemon, is it running?\n");
        fprintf(stderr, "Error: %s\n", pigpio_error(pi));
        fflush(stderr);
        return -1;
    }
    // TODO: check path
    serial = serial_open(pi, "/dev/serial1", BAUDRATE, 0);
    if(serial < 0) {
        fprintf(stderr, "Failed to open serial: %s\n", pigpio_error(serial));
        fflush(stderr);
        return -1;
    }
    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
      perror("Failed to created epoll instance");
      serial_close(pi, serial);
      pigpio_stop(pi);
      return -1;
    }
    // Add stdin to epoll interest list
    struct epoll_event ee;
    ee.events = EPOLLIN | EPOLLERR;
    ee.data.fd = STDIN_FILENO;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ee) < 0) {
      perror("Failed to control epoll instance");
      serial_close(pi, serial);
      pigpio_stop(pi);
      return -1;
    }
    return 0;
}

int main(void) {

    // Initialize
    if(init() != 0){
        return 1;
    }
    printf("Initialized!\n");

    uint8_t arx_buf_idx = 0;
    char arduinoRxBuffer[MAX_SIZE];

    uint8_t bt_buf_idx = 0;
    char bluetoothRxBuffer[MAX_SIZE];

    int ret = 0;
    int rv = 0;
    struct epoll_event ee;

    while(keep_running) {
        // main event loop
        if(serial_data_available(pi, serial) > 0) {
            printf("Serial Data Found!\n");
            int rv = serial_read(pi, serial, arduinoRxBuffer + arx_buf_idx, MAX_SIZE - arx_buf_idx);
            if(rv > 0) {
                arx_buf_idx += rv;
                rv = parse_serial(arduinoRxBuffer, arx_buf_idx);
                if(rv < 0) {
                    ret = 1;
                    keep_running = 0;
                }
                memmove(arduinoRxBuffer, arduinoRxBuffer + rv, arx_buf_idx - rv);
                arx_buf_idx -= rv;
            } else if(rv < 0) {
                fprintf(stderr, "Failed to read from serial: %s\n", pigpio_error(rv));
                fflush(stderr);
                ret = 1;
                keep_running = 0;
            }
        }
        rv = epoll_wait(epoll_fd, &ee, 1, 0);
        if(rv > 0 && (ee.events & EPOLLIN)) {
          printf("Bluetooth data found!\n");
          // File descriptor is ready to read
          rv = read(ee.data.fd, bluetoothRxBuffer + bt_buf_idx, MAX_SIZE - bt_buf_idx);
          if(rv >= 0) {
            bt_buf_idx += rv;
            rv = parse_bt(bluetoothRxBuffer, bt_buf_idx);
            if(rv < 0) {
                ret = 1;
                keep_running = 0;
            }
            memmove(bluetoothRxBuffer, bluetoothRxBuffer + rv, bt_buf_idx - rv);
            bt_buf_idx -= rv;
          } else {
              perror("Error reading from stdin");
              ret = 1;
              keep_running = 0;
          }
        } else if(rv < 0) {
            perror("Error in epoll_wait");
            ret = 1;
            keep_running = 0;
        }
    }

    // Clean-up
    printf("Clean up!\n");
    serial_close(pi, serial);
    pigpio_stop(pi);
    close(epoll_fd);
    return ret;
}
