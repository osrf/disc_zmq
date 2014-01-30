#ifndef NNET_ETH_H
#define NNET_ETH_H

#include <stdint.h>

#define NNET_ETH_MAC_LEN 6
typedef struct
{
  uint8_t  dest_addr[NNET_ETH_MAC_LEN];
  uint8_t  source_addr[NNET_ETH_MAC_LEN];
  uint16_t ethertype : 16;
} __attribute__((packed)) nnet_eth_header_t;

#define NNET_ETH_ETHERTYPE_IP    0x0800
#define NNET_ETH_ETHERTYPE_ARP   0x0806

// the raw ethernet TX interface must be implemented for each target system
void nnet_eth_tx(const uint8_t *data, const uint16_t len); 
typedef void (*nnet_eth_tx_fn_t)(const uint8_t *data, const uint16_t len);

// this is the packet dispatch function. enter here. 
void nnet_eth_rx(const uint8_t *data, const uint16_t len);

#endif

