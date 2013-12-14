#include <stdio.h>
#include <string.h>

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "disc_zmq/disc_zmq.h"

uuid_t GUID = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static size_t GUID_LENGTH = sizeof(GUID);
static size_t GUID_STR_LENGTH = (sizeof(GUID) * 2) + 4 + 1;

int dzmq_handles[MAX_DZMQ_HANDLES];
char ** dzmq_ip_address[MAX_DZMQ_HANDLES][DZMQ_IPV4_ADDRSTRLEN];
int next_dzmq_handle = 0;

void dzmq_guid_to_str(uuid_t guid, char * guid_str, int guid_str_len) {
    for (size_t i = 0; i < GUID_LENGTH && i != guid_str_len; i ++) {
        snprintf(guid_str, guid_str_len,
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 guid[0], guid[1], guid[2], guid[3], guid[4], guid[5], guid[6],
                 guid[7], guid[8], guid[9], guid[10], guid[11], guid[12],
                 guid[13], guid[14], guid[15]
        );
    }
}

int dzmq_guid_is_zero(uuid_t guid) {
    for (size_t i = 0; i < GUID_LENGTH; i ++) {
        if (guid[i] != 0) {
            return 0;
        }
    }
    return 1;
}

dzmq_handle_t dzmq_init() {
    printf("%i\n", INET6_ADDRSTRLEN);
    /* Generate uuid if not done yet */
    if (dzmq_guid_is_zero(GUID)) {
        uuid_generate(GUID);
    }
    /* Print the guid */
    char guid_str[GUID_STR_LENGTH];
    dzmq_guid_to_str(GUID, guid_str, GUID_STR_LENGTH);
    printf("GUID: %s\n", guid_str);
    /* Check to make sure we have handles left to give out */
    if (next_dzmq_handle == sizeof(dzmq_handles) - 1) {
        perror("out of dzmq handles");
    }
    /* Set the handle as valid and return it */
    int this_handle = next_dzmq_handle;
    next_dzmq_handle += 1;
    dzmq_handles[this_handle] = 1;
    return this_handle;
}

void dzmq_free(dzmq_handle_t handle) {
    dzmq_handles[handle] = 0;
}

void dzmq_set_ip_addr(dzmq_handle_t handle, char * address) {
    if (!dzmq_handles[handle]) {
        perror("invalid handle: dzmq_set_ip_addr");
    }
    // memcpy(dzmq_ip_address[handle][0], address, )
    strncpy(*dzmq_ip_address[handle][0], address, DZMQ_IPV4_ADDRSTRLEN);
}

/* References:
 * http://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine
 * http://man7.org/linux/man-pages/man3/getifaddrs.3.html
 */
int dzmq_get_address_ipv4(char * address) {
    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);
    int address_found = 0;

    for (ifa = ifAddrStruct; NULL != ifa; ifa = ifa->ifa_next) {
        /* If IPv4 */
        if (AF_INET == ifa->ifa_addr->sa_family) {
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, address, INET_ADDRSTRLEN);
        } else {
            continue;
        }
        /* TODO: create IPv6 version */
        /* Else-If IPV6 * /
        else if (AF_INET6 == ifa->ifa_addr->sa_family) {
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            inet_ntop(AF_INET6, tmpAddrPtr, address, INET6_ADDRSTRLEN);
        } */
        /* stop at the first non 127.0.0.1 address */
        if (address[0] && strncmp(address, "127.0.0.1", 9)) {
            address_found = 1;
            break;
        }
    }
    /* Free address structure */
    if (NULL != ifAddrStruct) {
        freeifaddrs(ifAddrStruct);
    }
    /* If address not found, zero it */
    if (!address_found) {
        address[0] = 0;
    }
    return address_found;
}
