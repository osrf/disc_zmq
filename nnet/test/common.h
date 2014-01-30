#ifndef NNET_TEST_COMMON_H
#define NNET_TEST_COMMON_H

#include <stdint.h>

#ifdef TEST_MSP430
int putchar(int c);
#endif

int open_raw_socket(const char *iface);
void send_raw(const int sock, const uint8_t *data, const uint16_t len);

#endif

