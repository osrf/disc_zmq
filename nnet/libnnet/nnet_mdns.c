#include "nnet_mdns.h"
#include "nnet_buf.h"
#include "nnet_utils.h"

#ifdef NNET_CONFIG_CONSOLE_AVAIL
  #include <stdio.h>
  #include <stdlib.h>
#endif

static const uint8_t  NNET_MDNS_MAC[6] = {0x01, 0x00, 0x5e, 
                                          0x00, 0x00, 0xfb};
static const uint32_t NNET_MDNS_IP_ADDR = 0xe00000fb;
static const uint16_t NNET_MDNS_UDP_PORT = 5353;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void nnet_mdns_send_query(const char *name)
{
  const uint16_t name_len = nnet_strlen(name, NNET_BUF_TX_SIZE - 40);
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  printf("name_len = %d\n", name_len);
#endif
  const uint16_t udp_payload_len =   sizeof(nnet_mdns_header_t) 
                                   - sizeof(nnet_udp_header_t)
                                   + name_len + 2  // name + null + extra byte for count
                                   + 4;            // final flags
  nnet_udp_header_t *udp = (nnet_udp_header_t *)nnet_buf_tx;
  nnet_udp_init_header(udp, 
                       NNET_MDNS_MAC, NNET_MDNS_IP_ADDR, 
                       NNET_MDNS_UDP_PORT, NNET_MDNS_UDP_PORT,
                       udp_payload_len);
  nnet_mdns_header_t *mdns = (nnet_mdns_header_t *)nnet_buf_tx;
  uint8_t *mdns_data = nnet_buf_tx + sizeof(nnet_mdns_header_t);
  mdns->id = 0; // todo: generate rand16 for this
  mdns->flags = 0; // query
  mdns->qdcount = nnet_htons(1);
  mdns->ancount = 0;
  mdns->nscount = 0;
  mdns->arcount = 0;
  // now, chop up the domain name components
  const char *rd_ptr = name;
  uint8_t *len_wr_ptr = mdns_data; // we'll plop down the string size here
  char *str_wr_ptr = mdns_data+1; // the write pointer for our string copies
  uint8_t token_len = 0;
  while (rd_ptr != name + name_len)  // we're done when we have read the domain
  {
    if (*rd_ptr == '.') // end of token
    {
      *len_wr_ptr = token_len;
      token_len = 0;
      len_wr_ptr = str_wr_ptr; // save this spot for later
    }
    else
    {
      *str_wr_ptr = *rd_ptr;
      token_len++;
    }
    rd_ptr++; 
    str_wr_ptr++; // onward !! charrrrrrge!
  }
  *len_wr_ptr = token_len; // write out last token length
  *str_wr_ptr = 0; // null-terminate
  uint16_t *flag_ptr = (uint16_t *)(str_wr_ptr + 1);
  uint16_t *qtype  = (uint16_t *)(str_wr_ptr + 1);
  uint16_t *qclass = (uint16_t *)(str_wr_ptr + 3);
  *qtype = nnet_htons(1);  // request the A record
  *qclass = nnet_htons(1); // request the internet address class
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  nnet_udp_print_packet(udp);
#endif
  nnet_eth_tx((uint8_t *)udp, udp_payload_len + sizeof(nnet_udp_header_t));
}

static void nnet_mdns_tx_answer(const char *fqdn)
{
  printf("nnet_tx_answer\n");
}

static void nnet_mdns_rx_query(const uint8_t *data, const uint16_t len)
{
  const nnet_mdns_header_t *mdns = (nnet_mdns_header_t *)data;
  // sanity-check
  if (!mdns->qdcount)
    return;
  const uint16_t udp_payload_len = nnet_htons(mdns->udp.len) - 8;
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  printf("nnet_rx_query\n");
#endif
  // convert the FQDN to a normal c-string
  const uint8_t *mdns_data = data + sizeof(nnet_mdns_header_t);
  char *fqdn = nnet_buf_rx; // use our rx scratch buffer
  char *str_wr_ptr = fqdn;
  const char *str_rd_ptr = mdns_data;
  while (*str_rd_ptr && (str_wr_ptr - fqdn < NNET_BUF_RX_SIZE - 1))
  {
    uint8_t token_len = (uint8_t) *str_rd_ptr;
    str_rd_ptr++;
    for (int i = 0; i < token_len; i++)
    {
      *str_wr_ptr = *str_rd_ptr;
      str_wr_ptr++;
      str_rd_ptr++;
    }
    if (*str_rd_ptr)
      *(str_wr_ptr++) = '.';
  }
  *str_wr_ptr = 0;
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  printf("fqdn: [%s]\n", fqdn);
#endif
  if (nnet_streq(fqdn, nnet_config_fqdn))
    nnet_mdns_tx_answer(fqdn);
}

void nnet_mdns_rx_eth_frame(const uint8_t *data, const uint16_t len)
{
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  printf("nnet mdns rx %d-byte frame\n", len);
#endif
  const nnet_mdns_header_t *mdns = (nnet_mdns_header_t *)data;
  // sanity-check: make sure that it's at least a UDP packet
  if (mdns->udp.ip.eth.ethertype != nnet_htons(NNET_IP_ETHERTYPE) ||
      mdns->udp.ip.proto != NNET_IP_PROTO_UDP)
    return;
  // todo: verify IP csum... though if it got through eth csum, it's fine.
  // ensure that this packet is addressed to the MDNS address and port
  if (mdns->udp.ip.dest_addr != nnet_htonl(NNET_MDNS_IP_ADDR) ||
      mdns->udp.dest_port != nnet_htons(NNET_MDNS_UDP_PORT))
    return;
  // ensure that it is long enough to be meaningful
  const uint16_t udp_payload_len = nnet_htons(mdns->udp.len) - 8;
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  printf("udp payload: %d bytes\n", udp_payload_len);
#endif
  // now, dispatch depending on mDNS packet type
  if (mdns->flags == 0) // todo: it's more complex than this
    nnet_mdns_rx_query(data, len);
}

// reference: http://www.ccs.neu.edu/home/amislove/teaching/cs4700/fall09/handouts/project1-primer.pdf
// http://en.wikipedia.org/wiki/MDNS
// http://en.wikipedia.org/wiki/IPv4_header#Header
