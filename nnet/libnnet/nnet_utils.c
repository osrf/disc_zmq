#include "nnet_utils.h"

// todo: find ways for this to be overridden on CPU's with built-ins for this
uint16_t nnet_swap16(const uint16_t x)
{
  return ((x & 0xff) << 8) | ((x >> 8) & 0xff);
}

uint32_t nnet_swap32(const uint32_t x)
{
  return ((x & 0x000000ff) << 24)  |
         ((x & 0x0000ff00) << 8)   |
         ((x & 0x00ff0000) >> 8)   |
         ((x & 0xff000000) >> 24);
}

uint16_t nnet_strlen(const char *str, const uint16_t max_len)
{
  const char *c = str;
  while (*c && (c - str) < max_len)
    c++;
  return (uint16_t)(c - str);
}

int nnet_streq(const char *a, const char *b)
{
  while (*a && *b)
  {
    if (*a != *b)
      return NNET_FALSE;
    *a++;
    *b++;
  }
  return NNET_TRUE;
}

