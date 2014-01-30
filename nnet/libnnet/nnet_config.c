#include "nnet_config.h"

uint32_t nnet_config_ip_addr = 0x0a0a0102; 

uint8_t nnet_config_mac_addr_array[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
uint8_t *nnet_config_mac_addr = nnet_config_mac_addr_array;
char *nnet_config_fqdn = NULL;
