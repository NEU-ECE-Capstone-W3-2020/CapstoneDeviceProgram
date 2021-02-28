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
#include "emic.h"
#include "capstone_bluetooth.h"

static volatile int keep_running = 1;

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

int main(void) {
    Nano nano;
    struct epoll_event ee;
    char bt_buf[BUF_SIZE];
    int bt_buf_idx, emic_fd, epoll_fd, ret, rv;
    // Initialize
    ret = 0;
    bt_buf_idx = 0;
    emic_fd = init_emic("/dev/ttyUBS1");
    if(emic_fd < 0) {
        return 1;
    }

    if(nano_init(&nano, emic_fd) != 0){
        return 1;
    }

    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
      perror("Failed to created epoll instance");
      return 1;
    }
    // Add stdin to epoll interest list
    ee.events = EPOLLIN;
    ee.data.fd = STDIN_FILENO;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ee) < 0) {
      perror("Failed to control epoll instance");
      return 1;
    }
    
    DPRINTF("Initialized!\n");

    if(nano_start_reading(&nano) != 0) {
        nano_cleanup(&nano);
        return 1;
    }

    while(keep_running) {
        rv = epoll_wait(epoll_fd, &ee, 1, -1);
        if(rv > 0 && (ee.events & EPOLLIN)) {
            // bluetooth
            rv = read(ee.data.fd, bt_buf + bt_buf_idx, BUF_SIZE - bt_buf_idx);
            if(rv >= 0) {
                bt_buf_idx += rv;
                rv = bt_parse_msg(bt_buf, bt_buf_idx, emic_fd);
                if(rv < 0) {
                    ret = 1;
                    keep_running = 0;
                }
                memmove(bt_buf, bt_buf + rv, bt_buf_idx - rv);
                bt_buf_idx -= rv;
            } else {
                perror("Error reading from bluetooth fd");
                ret = 1;
                keep_running = 0;
            }
        }
    }

    // Clean-up
    DPRINTF("Clean up!\n");

    close(epoll_fd);
    nano_cleanup(&nano);
    close(emic_fd);
    return ret;
}
