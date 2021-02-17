#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pigpiod_if2.h"

#define DEBUG
#define BAUDRATE    115200
#define MAX_SIZE       256
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

int text_to_voice(const char data, const size_t length){
    // TODO
}

int parse_packet(const char *buffer, const size_t length){
    // TODO
    return length;
}

int init(void){
    // Set control c to call our custom handler
    set_handler();

    pi = pigpio_start(NULL, NULL);
    if(pi < 0){
        fprintf(stderr, "Failed to connect to pigpi daemon, is it running?\n");
        return 1;
    }
    // TODO: check path
    serial = serial_open(pi, "/dev/serial0", BAUDRATE, 0);
    if(serial < 0) {
        fprintf(stderr, "Failed to open serial\n");
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

    size_t buf_idx = 0;
    char buffer[MAX_SIZE];

    while(keep_running) {
        // main event loop
        if(serial_data_available(pi, serial) > 0) {
            int rv = serial_read(pi, serial, buffer, MAX_SIZE - buf_idx);
            if(rv > 0) {
                buf_idx += rv;
                rv = parse_packet(buffer, buf_idx);
                memmove(buffer, buffer + rv, rv);
                buf_idx -= rv;
            } else if(rv < 0) {
                fprintf(stderr, "Failed to read from serial, code: %d\n", rv);
            }
        }
        // TODO: check bluetooth, prob through another serial call
    }

    // Clean-up
    pigpio_stop(pi);
    return 0;
}
