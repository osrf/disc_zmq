#include "nnet_udp.h"
#include "nnet_config.h"
#include "nnet_eth.h"
#include "nnet_utils.h"
#include "nnet_ip.h"

#ifdef NNET_CONFIG_CONSOLE_AVAIL
  #include <stdio.h>
#endif

void nnet_udp_init_header(nnet_udp_header_t *u,
                          const uint8_t *dest_mac,
                          const uint32_t dest_ip,
                          const uint16_t dest_port,
                          const uint16_t source_port,
                          const uint16_t payload_len)
{
  uint8_t i;
  // init ethernet frame
  for (i = 0; i < NNET_ETH_MAC_LEN; i++)
  {
    u->ip.eth.dest_addr[i]   = dest_mac[i];  //NNET_MDNS_IPV4_MAC[i];
    u->ip.eth.source_addr[i] = nnet_config_mac_addr[i];
  }
  u->ip.eth.ethertype = nnet_htons(NNET_IP_ETHERTYPE);
  // init IP frame
  u->ip.header_len = NNET_IP_HEADER_LEN;
  u->ip.version = NNET_IP_VERSION;
  u->ip.ecn = 0;
  u->ip.diff_serv = 0;
  u->ip.len = nnet_htons(20 + 8 + payload_len);
  u->ip.id = 0;
  u->ip.flag_frag = nnet_htons(NNET_IP_DONT_FRAGMENT);
  u->ip.ttl = 42; // todo: not sure what this should be
  u->ip.proto = NNET_IP_PROTO_UDP;
  u->ip.checksum = 0; // will be filled in later
  u->ip.dest_addr = nnet_htonl(dest_ip); //OSRF_MDNS_IPV4_ADDR);
  u->ip.source_addr = nnet_htonl(nnet_config_ip_addr); //  SMDNS_OUR_IPV4_ADDR);
  nnet_ip_insert_checksum(&u->ip);
  // finally, init UDP frame
  u->dest_port   = nnet_htons(dest_port);
  u->source_port = nnet_htons(source_port);
  u->len = nnet_htons(8 + payload_len);
  u->checksum = 0; // ipv4 UDP checksum is optional.
}

#ifdef NNET_CONFIG_CONSOLE_AVAIL
void nnet_udp_print_packet(nnet_udp_header_t *u)
{
  uint16_t i = 0; 
  printf("src: %s:%d\n", 
         nnet_ip_ntoa(u->ip.source_addr), nnet_htons(u->source_port));
  printf("dst: %s:%d\n", 
         nnet_ip_ntoa(u->ip.dest_addr), nnet_htons(u->dest_port));
  printf("ip checksum: 0x%04x\n", nnet_htons(u->ip.checksum));
  const uint16_t udp_payload_len = nnet_htons(u->ip.len) - 20 - 8;
  printf("udp payload length: %d\n", udp_payload_len);
  printf("udp payload (hex):\n000:  ");
  uint8_t *payload = (uint8_t *)u + sizeof(*u); // don't do this at home, kids
  for (i = 0; i < udp_payload_len; i++)
  {
    printf("%02x ", payload[i]);
    if (i % 16 == 7)
      printf("   ");
    else if (i % 16 == 15)
      printf("\n%03d:  ", i+1);
  }
  printf("\n");
}
#endif

