#ifndef NNET_MDNS_H
#define NNET_MDNS_H

#include <stdint.h>
#include "nnet_udp.h"

void nnet_mdns_init();
void nnet_mdns_fini();
void nnet_mdns_send_query(const char *name);
void nnet_mdns_rx_frame(const uint8_t *data, const uint16_t len);

typedef struct
{
  nnet_udp_header_t udp;
  uint16_t id;
  uint16_t flags;
  uint16_t qdcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
} __attribute__((packed)) nnet_mdns_header_t;

#endif

