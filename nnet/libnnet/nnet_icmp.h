#ifndef NNET_ICMP_H
#define NNET_ICMP_H

#include "nnet_ip.h"

typedef struct
{
  ip_header_t ip;
  uint8_t  type;
  uint8_t  code;
  uint16_t checksum;
  uint16_t id;
  uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

#define NNET_ICMP_ECHO_REPLY     0x00
#define NNET_ICMP_ECHO_REQUEST   0x08

#endif

