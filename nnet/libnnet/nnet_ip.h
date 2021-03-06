#ifndef NNET_IP_H
#define NNET_IP_H

#include <stdint.h>
#include "nnet_eth.h"
#include "nnet_config.h"

typedef struct
{
  nnet_eth_header_t eth;
  uint8_t  header_len   :  4;
  uint8_t  version      :  4;
  uint8_t  ecn          :  2;
  uint8_t  diff_serv    :  6;
  uint16_t len          : 16;
  uint16_t id           : 16;
  uint16_t flag_frag    : 16;
  uint8_t  ttl          :  8;
  uint8_t  proto        :  8;
  uint16_t checksum     : 16;
  uint32_t source_addr  : 32;
  uint32_t dest_addr    : 32;
} __attribute__((packed)) nnet_ip_header_t;

#define NNET_IP_HEADER_LEN     5
#define NNET_IP_VERSION        4
#define NNET_IP_DONT_FRAGMENT  0x4000

#define NNET_IP_PROTO_ICMP  0x01
#define NNET_IP_PROTO_UDP   0x11

void nnet_ip_insert_checksum(nnet_ip_header_t *header);

#ifdef NNET_CONFIG_CONSOLE_AVAIL
const char *nnet_ip_ntoa(const uint32_t addr);
#endif

void nnet_ip_rx(const uint8_t *data, const uint16_t data_len);

#endif

