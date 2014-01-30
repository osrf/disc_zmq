#include "nnet_ip.h"
#include "nnet_utils.h"

void nnet_ip_insert_checksum(nnet_ip_header_t *header)
{
  header->checksum = 0;
  uint32_t sum = 0;
  for (int word_idx = 0; word_idx < 10; word_idx++)
  {
    const uint16_t word = *(  (uint16_t *)header 
                            + sizeof(nnet_eth_header_t) / 2 
                            + word_idx );
    sum += nnet_ntohs(word);
    //printf("word %d: 0x%02x\r\n", word_idx, word);
  }
  sum += (sum >> 16);
  sum &= 0xffff;
  header->checksum = (uint16_t)nnet_htons(~sum);
}

#ifdef NNET_CONFIG_CONSOLE_AVAIL
const char *nnet_ip_ntoa(const uint32_t addr)
{
  static char buf[20]; // max string len: 4*3 + 3 + null = 16
  #ifdef NNET_LITTLE_ENDIAN
    sprintf(buf, "%d.%d.%d.%d", 
            (uint8_t)(addr & 0xff),
            (uint8_t)((addr & 0xff00) >> 8),
            (uint8_t)((addr & 0xff0000) >> 16),
            (uint8_t)((addr & 0xff000000) >> 24));
  #else
    #error nnet_ip_ntoa() is not yet coded up for big-endian.
  #endif
  return buf;
}

#endif

