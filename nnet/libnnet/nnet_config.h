#ifndef NNET_CONFIG_H
#define NNET_CONFIG_H

#include <stdint.h>

extern uint32_t nnet_config_ip_addr;
extern uint8_t *nnet_config_mac_addr;
extern char *nnet_config_fqdn;

#define NNET_CONFIG_CONSOLE_AVAIL

// todo: find out the endian-ness the right way
#define NNET_LITTLE_ENDIAN

#endif

