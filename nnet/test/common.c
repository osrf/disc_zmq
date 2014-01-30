#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <sys/select.h>
#include <unistd.h>

#ifdef TEST_MSP430
int putchar(int c)
{
  printf("%c", c);
}
#endif

struct ifreq g_if_idx;
struct sockaddr_ll g_sockaddr;

int open_raw_socket(const char *iface)
{
  int sock;
  if (-1 == (sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))))
  {
    perror("socket");
    exit(1);
  }
  memset(&g_if_idx, 0, sizeof(struct ifreq));
  strncpy(g_if_idx.ifr_name, iface, IFNAMSIZ-1);
  if (ioctl(sock, SIOCGIFINDEX, &g_if_idx) < 0)
  {
    perror("SIOCGIFINDEX");
    return 1;
  }
  g_sockaddr.sll_ifindex = g_if_idx.ifr_ifindex; // index of network device 
  g_sockaddr.sll_halen   = ETH_ALEN; // address length

  printf("interface address: %d\n", g_if_idx.ifr_ifindex);

  return sock;
}

void send_raw(const int sock, const uint8_t *data, const uint16_t len)
{
  sendto(sock, data, len, 0, 
         (struct sockaddr *)&g_sockaddr, 
         sizeof(struct sockaddr_ll));
}

