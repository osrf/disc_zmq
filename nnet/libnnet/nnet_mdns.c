#include "nnet_mdns.h"
#include "nnet_buf.h"

#ifdef NNET_CONSOLE_AVAIL
  #include <stdio.h>
  #include <stdlib.h>
#endif

static const uint8_t  NNET_MDNS_MAC[6] = {0x01, 0x00, 0x5e, 
                                          0x00, 0x00, 0xfb};
static const uint32_t NNET_MDNS_IP_ADDR = 0xe00000fb;
static const uint16_t NNET_MDNS_UDP_PORT = 5353;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void smdns_init() { }
void smdns_fini() { }

void smdns_send_query(const char *name)
{
  const uint16_t name_len = smdns_strlen(name, NNET_BUF_TX_SIZE - 40);
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  printf("name_len = %d\n", name_len);
#endif
  const uint16_t udp_payload_len =   sizeof(nnet_mdns_header_t) 
                                   - sizeof(nnet_udp_header_t)
                                   + name_len + 2  // name + null + extra byte for count
                                   + 4;            // final flags
  nnet_udp_header *udp = (nnet_udp_header *)nnet_buf_tx;
  nnet_udp_init_header(udp, 
                       NNET_MDNS_MAC, NNET_MDNS_IP_ADDR, 
                       NNET_MDNS_UDP_PORT, NNET_MDNS_UDP_PORT
                       udp_payload_len);
  osrf_mdns_header_t *mdns = (nnet_mdns_header_t *)nnet_buf_tx;
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
  *qtype = osrf_htons(1);  // request the A record
  *qclass = osrf_htons(1); // request the internet address class
#ifdef NNET_CONFIG_CONSOLE_AVAIL
  nnet_udp_print_packet(udp);
#endif
  nnet_eth_tx((uint8_t *)udp, udp_payload_len + sizeof(nnet_udp_header_t));
}

static int smdns_streq(const char *a, const char *b)
{
  while (*a && *b)
  {
    if (*a != *b)
      return SMDNS_FALSE;
    *a++;
    *b++;
  }
  return SMDNS_TRUE;
}

static void smdns_tx_answer()
{
  printf("smdns_tx_answer\n");
}

static void smdns_rx_query(const uint8_t *data, const uint16_t len)
{
  const osrf_mdns_header_t *mdns = (osrf_mdns_header_t *)data;
  // sanity-check
  if (!mdns->qdcount)
    return;
  const uint16_t udp_payload_len = osrf_htons(mdns->udp.len) - 8;
#ifdef SMDNS_CONSOLE_DEBUG
  printf("smdns_rx_query\n");
#endif
  // convert the FQDN to a normal c-string
  const uint8_t *mdns_data = data + sizeof(osrf_mdns_header_t);
  char *fqdn = g_smdns_rx_buf; // use our rx scratch buffer
  char *str_wr_ptr = fqdn;
  const char *str_rd_ptr = mdns_data;
  while (*str_rd_ptr && (str_wr_ptr - fqdn < SMDNS_RX_BUF_SIZE - 1))
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
  printf("fqdn: [%s]\n", fqdn);
  if (smdns_streq(fqdn, SMDNS_OUR_FQDN))
    smdns_tx_answer();
}

void smdns_rx_frame(const uint8_t *data, const uint16_t len)
{
#ifdef SMDNS_CONSOLE_DEBUG
  printf("smdns rx %d-byte frame\n", len);
#endif
  const osrf_mdns_header_t *mdns = (osrf_mdns_header_t *)data;
  // sanity-check: make sure that it's at least a UDP packet
  if (mdns->udp.ip.eth.ethertype != osrf_htons(OSRF_IP_ETHERTYPE) ||
      mdns->udp.ip.proto != OSRF_IP_PROTO_UDP)
    return;
  // todo: verify IPV4 csum... though if it got through eth csum, it's fine.
  // ensure that this packet is addressed to the MDNS address and port
  if (mdns->udp.ip.dest_addr != osrf_htonl(OSRF_MDNS_IPV4_ADDR) ||
      mdns->udp.dest_port != osrf_htons(OSRF_MDNS_UDP_PORT))
    return;
  // ensure that it is long enough to be meaningful
  const uint16_t udp_payload_len = osrf_htons(mdns->udp.len) - 8;
#ifdef SMDNS_CONSOLE_DEBUG
  printf("udp payload: %d bytes\n", udp_payload_len);
#endif
  // now, dispatch depending on mDNS packet type
  if (mdns->flags == 0) // todo: it's more complex than this, i think
    smdns_rx_query(data, len);
}

void smdns_register_tx_fn(const smdns_tx_fn_t f)
{
  g_smdns_tx_fn = f;
}

// reference: http://www.ccs.neu.edu/home/amislove/teaching/cs4700/fall09/handouts/project1-primer.pdf
// http://en.wikipedia.org/wiki/MDNS
// http://en.wikipedia.org/wiki/IPv4_header#Header
