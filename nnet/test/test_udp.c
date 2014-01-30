#include <stdio.h>
#include <stdlib.h>
#include "nnet_mdns.h"
#include "common.h"
#include "nnet_config.h"
#include "nnet_utils.h"

void nnet_eth_tx(const uint8_t *data, const uint16_t len)
{
  printf("nnet_eth_tx: %d bytes\n", len);
  for (int i = 0; i < len; i++)
  {
    printf("%02x ", data[i]);
    if (i % 16 == 7)
      printf("   ");
    else if (i % 16 == 15)
      printf("\n");
  }
  printf("\n");
  // minimal test: loopback to ourself
  //nnet_mdns_rx_eth_frame(data, len);
  // todo: it would be cool to connect two nnet instances through a pipe.
}

int main(int argc, char ** argv)
{
  nnet_config_ip_addr = nnet_htonl(0x0a0a0102);
  nnet_config_set_mac_addr(0,1,2,3,4,5);
  nnet_config_fqdn = "some_machine.local";
  printf("hello world\n");
  //nnet_mdns_send_query("some_machine.local");
  uint8_t tx_pkt[1024];
  nnet_udp_header_t *u = (nnet_udp_header_t *)tx_pkt;
  const uint8_t dest_mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  nnet_udp_init_header(u, dest_mac, 0x0a0a0101, 0x1234, 0x5678, 20);
  uint8_t *payload = nnet_udp_payload(u);
  for (int i = 0; i < 20; i++)
    payload[i] = i;
  nnet_udp_tx(u);
}

