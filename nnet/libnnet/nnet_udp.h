#ifndef NNET_UDP_H
#define NNET_UDP_H

#include "nnet_ip.h"
#include "nnet_config.h"

typedef struct
{
  nnet_ip_header_t ip;
  uint16_t source_port;
  uint16_t dest_port;
  uint16_t len;
  uint16_t checksum;
} __attribute__((packed)) nnet_udp_header_t;

void nnet_udp_init_header(nnet_udp_header_t *u,
                          const uint8_t *dest_mac,
                          const uint32_t dest_ip,
                          const uint16_t dest_port,
                          const uint16_t source_port,
                          const uint16_t payload_len);

void nnet_udp_tx(nnet_udp_header_t *udp);
uint8_t *nnet_udp_payload(nnet_udp_header_t *udp);

#ifdef NNET_CONFIG_CONSOLE_AVAIL
void nnet_udp_print_packet(nnet_udp_header_t *u);
#endif

#endif
