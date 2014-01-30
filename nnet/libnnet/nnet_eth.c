#include "nnet_eth.h"
#include "nnet_config.h"
#include "nnet_ip.h"
#include "nnet_arp.h"

#define NNET_ETH_MAC_FILTER_REJECT 0
#define NNET_ETH_MAC_FILTER_PASS   1
int nnet_eth_mac_filter(const nnet_eth_header_t *eth)
{
  const uint8_t mac_bcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  const uint8_t mac_mdns[6]  = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0xfb };
  #define NNET_MAC_FILTER_LEN 3
  const uint8_t * const allowable_macs[NNET_MAC_FILTER_LEN] = 
    { nnet_config_mac_addr, mac_bcast, mac_mdns };
  for (int i = 0; i < NNET_MAC_FILTER_LEN; i++)
  {
    int match = 1;
    for (int j = 0; match && j < NNET_ETH_MAC_LEN; j++)
      if (eth->dest_addr[j] != allowable_macs[i][j])
        match = 0;
    if (match)
      return NNET_ETH_MAC_FILTER_PASS;
  }
  return NNET_ETH_MAC_FILTER_REJECT;
}

void nnet_eth_rx(const uint8_t *data, const uint16_t len)
{
  // sanity check the length
  if (len < sizeof(nnet_eth_header_t))
    return;
  const nnet_eth_header_t *eth = (nnet_eth_header_t *)data;
  if (nnet_eth_mac_filter(eth) == NNET_ETH_MAC_FILTER_REJECT)
    return; 
  switch (nnet_ntohs(eth->ethertype))
  {
    case NNET_ETH_ETHERTYPE_IP:  nnet_ip_rx (data, len); break;
    case NNET_ETH_ETHERTYPE_ARP: nnet_arp_rx(data, len); break;
    default: break;
  }
}

