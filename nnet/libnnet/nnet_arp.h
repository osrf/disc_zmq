#ifndef NNET_ARP_H
#define NNET_ARP_H

#include "nnet_eth.h"

typedef struct
{
  nnet_eth_header_t eth;
  uint16_t hw_type         : 16;
  uint16_t proto_type      : 16;
  uint8_t  hw_addr_len     :  8;
  uint8_t  proto_addr_len  :  8;
  uint16_t operation       : 16;
  uint8_t  sender_hw_addr[6];
  uint32_t sender_proto_addr;
  uint8_t  target_hw_addr[6];
  uint32_t target_proto_addr;
} __attribute__((packed)) nnet_arp_packet_t;

#define NNET_ARP_HW_ETHERNET  1
#define NNET_ARP_PROTO_IPV4   0x0800
#define NNET_ARP_OP_REQUEST   1
#define NNET_ARP_OP_RESPONSE  2

void nnet_arp_rx(const uint8_t *data, const uint16_t data_len);

#endif

