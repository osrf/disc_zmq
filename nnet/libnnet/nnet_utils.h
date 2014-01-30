#ifndef NNET_UTILS_H
#define NNET_UTILS_H

#include <stdint.h>

uint16_t nnet_swap16(const uint16_t x);
uint32_t nnet_swap32(const uint32_t x);

#ifdef NNET_LITTLE_ENDIAN
  #define nnet_ntohs(x) nnet_swap16(x)
  #define nnet_ntohl(x) nnet_swap32(x)
#else
  #define nnet_ntohs(x) (x)
  #define nnet_ntohl(x) (x) 
#endif

#define NNET_FALSE 0
#define NNET_TRUE  1
#define NNET_NULL  0

uint16_t nnet_strlen(const char *str, const uint16_t max_len);

#endif

