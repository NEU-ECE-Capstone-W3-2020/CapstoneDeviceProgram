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

#define DEBUG
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
  buffer[TYPE_IDX] = TTS_MSG_TYPE;
  buffer[LEN_IDX] = length + HDR_SIZE;
  int buf_idx = HDR_SIZE;
  memcpy(buffer + buf_idx, data, length);
  buf_idx += length;
#ifdef DEBUG
  printf("Sending To Arduino: ");
  print_buffer_string(buffer + HDR_SIZE, buf_idx - HDR_SIZE);
  printf("\n");
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
        printf("Received TTS msg: ");
        print_buffer_string(buffer + HDR_SIZE, buffer[LEN_IDX] - HDR_SIZE);
        printf("\n");
#endif
        break;
    default:
      return 0;
    }
    return buffer[LEN_IDX];
}

int init(void){
    // Set control c to call our custom handler
    set_handler();

    serial = open("/dev/USB0", O_RDWR);
    if(serial < 0) {
        perror("Failed to open serial");
        return -1;
    }

    if(setup_arduino_serial(serial) < 0) {
        printf("Failed to configure serial");
        close(serial);
        return -1;
    } 

    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
      perror("Failed to created epoll instance");
      close(serial);
      return -1;
    }
    // Add stdin to epoll interest list
    struct epoll_event ee;
    ee.events = EPOLLIN;
    ee.data.fd = STDIN_FILENO;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ee) < 0) {
      perror("Failed to control epoll instance");
      close(serial);
      return -1;
    }

    // Add serial to epoll interest list
    ee.events = EPOLLIN;
    ee.data.fd = serial;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serial, &ee) < 0) {
      perror("Failed to control epoll instance");
      close(serial);
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
        rv = epoll_wait(epoll_fd, &ee, 1, -1);
        if(rv > 0 && (ee.events & EPOLLIN)) {
            if(ee.data.fd == STDIN_FILENO) {
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
            } else if(ee.data.fd == serial){
                printf("Serial Data Found!\n");
                rv = read(serial, arduinoRxBuffer + arx_buf_idx, MAX_SIZE - arx_buf_idx);
                if(rv >= 0) {
                    arx_buf_idx += rv;
                    rv = parse_serial(arduinoRxBuffer, arx_buf_idx);
                    if(rv < 0) {
                        ret = 1;
                        keep_running = 0;
                    }
                    memmove(arduinoRxBuffer, arduinoRxBuffer + rv, arx_buf_idx - rv);
                    arx_buf_idx -= rv;
                    } else {
                        /* printf("Failed to read from serial: %s\n", pigpio_error(rv)); */
                        perror("Failed to read from serial");
                        ret = 1;
                        keep_running = 0;
                    }
            } else {
                printf("Unkown fd\n");
            }
        } else if(rv < 0) {
            perror("Error in epoll_wait");
            ret = 1;
            keep_running = 0;
        }
    }

    // Clean-up
    printf("Clean up!\n");
    close(serial);
    close(epoll_fd);
    return ret;
}
