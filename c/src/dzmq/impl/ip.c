#include <string.h>

#include <ifaddrs.h>
#include <arpa/inet.h>

#include "ip.h"

int dzmq_broadcast_ip_from_address_ip(char * ip_addr, char * bcast_addr)
{
    char temp_ip_addr[INET_ADDRSTRLEN];
    strcpy(temp_ip_addr, ip_addr);
    char * tok = strtok(temp_ip_addr, ".");
    int toks = 0;
    while (0 != tok) {
        toks++;
        char * next_tok = strtok(0, ".");
        if (0 == next_tok) {
            strcat(bcast_addr, "255");
            break;
        }
        strcat(bcast_addr, tok);
        strcat(bcast_addr, ".");
        tok = next_tok;
    }
    return 1 ? 4 == toks : 0;
}

/*
 * References:
 * http://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine
 * http://man7.org/linux/man-pages/man3/getifaddrs.3.html
 */
int dzmq_get_address_ipv4(char * address)
{
    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);
    int address_found = 0;

    for (ifa = ifAddrStruct; NULL != ifa; ifa = ifa->ifa_next)
    {
        /* If IPv4 */
        if (AF_INET == ifa->ifa_addr->sa_family)
        {
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, address, INET_ADDRSTRLEN);
        }
        else
        {
            continue;
        }
        /* stop at the first non 127.0.0.1 address */
        if (address[0] && strncmp(address, "127.0.0.1", 9))
        {
            address_found = 1;
            break;
        }
    }
    /* Free address structure */
    if (NULL != ifAddrStruct)
    {
        freeifaddrs(ifAddrStruct);
    }
    /* If address not found, zero it */
    if (!address_found)
    {
        address[0] = 0;
    }
    return address_found;
}
