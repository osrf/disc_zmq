#ifndef NNET_CONFIG_H
#define NNET_CONFIG_H

#include <stdint.h>

extern uint32_t nnet_config_ip_addr;
extern uint8_t *nnet_config_mac_addr;
extern char *nnet_config_fqdn;

void nnet_config_set_mac_addr(const uint8_t m0, const uint8_t m1,
                              const uint8_t m2, const uint8_t m3,
                              const uint8_t m4, const uint8_t m5);

#define NNET_CONFIG_CONSOLE_AVAIL

// todo: find out the endian-ness the right way
#define NNET_LITTLE_ENDIAN

#endif

