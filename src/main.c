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
