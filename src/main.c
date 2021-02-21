#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

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
  return serial_write(pi, serial, buffer, buf_idx);
}

int parse_packet(const char *buffer, const uint8_t length){
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
        return 1;
    }
    // TODO: check path
    serial = serial_open(pi, "/dev/serial0", BAUDRATE, 0);
    if(serial < 0) {
        fprintf(stderr, "Failed to open serial: %s\n", pigpio_error(serial));
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);


    // Initialize
    if(init() != 0){
        return 1;
    }

    uint8_t arx_buf_idx = 0;
    char arduinoRxBuffer[MAX_SIZE];

    uint8_t bt_buf_idx = 0;
    char bluetoothRxBuffer[MAX_SIZE];

    int rv = 0;

    while(keep_running) {
        // main event loop
        if(serial_data_available(pi, serial) > 0) {
            int rv = serial_read(pi, serial, arduinoRxBuffer + arx_buf_idx, MAX_SIZE - arx_buf_idx);
            if(rv > 0) {
                arx_buf_idx += rv;
                rv = parse_packet(arduinoRxBuffer, arx_buf_idx);
                memmove(arduinoRxBuffer, arduinoRxBuffer + rv, arx_buf_idx - rv);
                arx_buf_idx -= rv;
            } else if(rv < 0) {
                fprintf(stderr, "Failed to read from serial: %s\n", pigpio_error(rv));
            }
        }
        if((rv = read(stdin, bluetoothRxBuffer + bt_buf_idx, MAX_SIZE - bt_buf_idx) > 0) {
            bt_buf_idx += rv;
            rv = parse_packet(bluetoothRxBuffer, bt_buf_idx);
            memmove(bluetoothRxBuffer, bluetoothRxBuffer + rv, bt_buf_idx - rv);
            bt_buf_idx -= rv;
        }
    }

    // Clean-up
    pigpio_stop(pi);
    return 0;
}
