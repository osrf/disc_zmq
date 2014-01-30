#include "nnet_config.h"
#include "nnet_eth.h"

uint32_t nnet_config_ip_addr = 0x0a0a0102; 

uint8_t nnet_config_mac_addr_array[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
uint8_t *nnet_config_mac_addr = nnet_config_mac_addr_array;
char *nnet_config_fqdn = 0;

void nnet_config_set_mac_addr(const uint8_t m0, const uint8_t m1,
                              const uint8_t m2, const uint8_t m3,
                              const uint8_t m4, const uint8_t m5)
{
  nnet_config_mac_addr[0] = m0;
  nnet_config_mac_addr[1] = m1;
  nnet_config_mac_addr[2] = m2;
  nnet_config_mac_addr[3] = m3;
  nnet_config_mac_addr[4] = m4;
  nnet_config_mac_addr[5] = m5;
}

