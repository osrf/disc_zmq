#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <zmq.h>

#include "dzmq.h"
#include "impl/header.h"
#include "impl/adv_msg.h"
#include "impl/guid.h"
#include "impl/ip.h"
#include "impl/topic.h"
#include "impl/config.h"

uuid_t GUID = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int init_called = 0;

int bcast_fd;
struct sockaddr_in dst_addr, rcv_addr;
struct ip_mreq mreq;

char ip_address[INET_ADDRSTRLEN];
char bcast_address[INET_ADDRSTRLEN];
char tcp_address[DZMQ_MAX_ADDR_LENGTH];

void * zmq_context;
void * zmq_publish_sock;
zmq_pollitem_t poll_items[DZMQ_MAX_POLL_ITEMS];
size_t poll_items_count = 0;

dzmq_topic_list_t published_topics;
dzmq_topic_list_t subscribed_topics;


int register_file_descriptor(int fd)
{
    if (poll_items_count >= DZMQ_MAX_POLL_ITEMS)
    {
        perror("Too many devices to poll");
        return 0;
    }
    poll_items[poll_items_count].socket = 0;
    poll_items[poll_items_count].fd = fd;
    poll_items[poll_items_count].events = ZMQ_POLLIN;
    poll_items_count += 1;
    return poll_items_count;
}

int register_socket(void * socket)
{
    if (poll_items_count >= DZMQ_MAX_POLL_ITEMS)
    {
        perror("Too many devices to poll");
        return 0;
    }
    poll_items[poll_items_count].socket = socket;
    poll_items[poll_items_count].events = ZMQ_POLLIN;
    poll_items_count += 1;
    return poll_items_count;
}

int dzmq_init()
{
    if (init_called)
    {
        perror("dzmq_init called more than once");
        return 0;
    }
    init_called = 1;
    /* Generate uuid */
    uuid_generate(GUID);
    /* Get the IPv4 address */
    if (0 >= dzmq_get_address_ipv4(ip_address))
    {
        perror("Error getting IPv4 address.");
        return 0;
    }
    /* Get the broadcast address from the IPv4 address */
    if (0 >= dzmq_broadcast_ip_from_address_ip(ip_address, bcast_address))
    {
        perror("Error comupting broadcast ip address");
        return 0;
    }
    /* Setup broadcast socket */
    bcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 >= bcast_fd)
    {
        perror("Error opening socket");
        return 0;
    }
    /* Make socket non-blocking */
    if (fcntl(bcast_fd, F_SETFL, O_NONBLOCK, 1) < 0)
    {
        perror("Error setting socket to non-blocking");
        return 0;
    }
    /* Allow multiple sockets to use the same PORT number */
    u_int yes = 1;
    if (setsockopt(bcast_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    {
        perror("Reusing ADDR failed");
        return 0;
    }
    if (setsockopt(bcast_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0)
    {
        perror("setsockopt (SOL_SOCKET, SO_REUSEPORT)");
        return 0;
    }
    /* Allow socket to use broadcast */
    if (setsockopt(bcast_fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0)
    {
        perror("setsockopt (SOL_SOCKET, SO_BROADCAST)");
        return 0;
    }
    /* Set up destination address */
    memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_addr.s_addr = inet_addr(bcast_address);
    dst_addr.sin_port = htons(DZMQ_DISC_PORT);
    /* Set up receiver address */
    memset(&rcv_addr, 0, sizeof(rcv_addr));
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rcv_addr.sin_port = htons(DZMQ_DISC_PORT);
    /* Bind to receive address */
    if (bind(bcast_fd, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr)) < 0)
    {
        perror("Error binding socket");
        return 0;
    }
    /* Add the bcast socket to the zmq poller */
    register_file_descriptor(bcast_fd);
    /* Setup publisher zmq socket */
    zmq_context = zmq_ctx_new();
    zmq_publish_sock = zmq_socket(zmq_context, ZMQ_PUB);
    char publish_addr[DZMQ_MAX_ADDR_LENGTH];
    sprintf(publish_addr, "tcp://%s:*", ip_address);
    if (0 > zmq_bind(zmq_publish_sock, publish_addr))
    {
        perror("Error binding zmq socket");
        return 0;
    }
    size_t size_of_tcp_address = sizeof(tcp_address);
    if (0 > zmq_getsockopt(zmq_publish_sock, ZMQ_LAST_ENDPOINT, tcp_address, &size_of_tcp_address))
    {
        perror("Error getting endpoint address of publisher socket");
        return 0;
    }
    register_socket(zmq_context);
    /* Report the state of the node */
    char guid_str[GUID_STR_LEN];
    dzmq_guid_to_str(GUID, guid_str, GUID_STR_LEN);
    printf("GUID:          %s\n", guid_str);
    printf("IPv4 Address:  %s\n", ip_address);
    printf("Bcast Address: %s\n", bcast_address);
    printf("TCP Endpoint:  %s\n", tcp_address);
    /* Return the handle */
    return 1;
}

int sendto_bcast(unsigned char * buffer, size_t buffer_len)
{
    return sendto(bcast_fd, buffer, buffer_len, 0, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
}

int send_adv(const char * topic_name)
{
    /* Build an adv_msg.header */
    dzmq_adv_msg_t adv_msg;
    adv_msg.header.version = 0x01;
    memcpy(adv_msg.header.guid, GUID, GUID_LEN);
    strcpy(adv_msg.header.topic, topic_name);
    adv_msg.header.type = DZMQ_OP_ADV;
    memset(adv_msg.header.flags, 0, DZMQ_FLAGS_LENGTH);
    /* Copy in adv_msg.addr */
    strcpy(adv_msg.addr, tcp_address);
    uint8_t buffer[DZMQ_UDP_MAX_SIZE];
    size_t adv_msg_len = serialize_adv_msg(buffer, &adv_msg);
    if (0 >= sendto_bcast(buffer, adv_msg_len))
    {
        perror("Error sending ADV message to broadcast");
        return 0;
    }
    return 1;
}

int dzmq_advertise(const char * topic_name)
{
    if (!init_called)
    {
        perror("(dzmq_advertise) dzmq_init must be called first");
        return 0;
    }
    /* Add topic to publisher list */
    if (!dzmq_topic_list_append(&published_topics, topic_name))
    {
        return 0;
    }
    return send_adv(topic_name);
}

int send_sub(const char * topic_name)
{
    /* Build a sub_msg */
    dzmq_msg_header_t header;
    header.version = 0x01;
    memcpy(header.guid, GUID, GUID_LEN);
    strcpy(header.topic, topic_name);
    header.type = DZMQ_OP_SUB;
    memset(header.flags, 0, DZMQ_FLAGS_LENGTH);
    uint8_t buffer[DZMQ_UDP_MAX_SIZE];
    size_t header_len = serialize_msg_header(buffer, &header);
    if (0 >= sendto_bcast(buffer, header_len))
    {
        perror("Error sending SUB message to broadcast");
        return 0;
    }
    return 1;
}

int dzmq_subscribe(const char * topic_name, dzmq_callback_t callback)
{
    if (!init_called)
    {
        perror("(dzmq_subscribe) dzmq_init must be called first");
        return 0;
    }
    /* Add topic to subscriber list */
    if (!dzmq_topic_list_append(&subscribed_topics, topic_name))
    {
        return 0;
    }
    return send_sub(topic_name);
}

int dzmq_publish(const char * topic_name, const uint8_t * msg, size_t len)
{
    if (!init_called)
    {
        perror("(dzmq_publish) dzmq_init must be called first");
        return 0;
    }
    return 1;
}

int handle_bcast_msg(uint8_t * buffer, int length)
{
    dzmq_msg_header_t header;
    size_t header_size = deserialize_msg_header(&header, buffer, length);
    if (header.type == DZMQ_OP_ADV)
    {
        dzmq_adv_msg_t adv_msg;
        deserialize_adv_msg(&adv_msg, buffer + header_size, length - header_size);
        memcpy(&adv_msg.header, &header, sizeof(header));
        if (dzmq_guid_compare(GUID, adv_msg.header.guid))
        {
            /* Ignore self messages */
            return 1;
        }
        if (0 == strncmp("tcp://", adv_msg.addr, 6))
        {
            printf("I should connect to tcp address: %s\n", adv_msg.addr);
            return 1;
        }
        else
        {
            printf("Unkown protocol type for address: %s\n", adv_msg.addr);
            return 0;
        }
    }
    else if (header.type == DZMQ_OP_SUB)
    {
        if (0 != dzmq_topic_in_list(&published_topics, header.topic))
        {
            /* Resend the ADV message */
            return send_adv(header.topic);
        }
    }
    else
    {
        printf("Unknown type: %i\n", header.type);
    }
    return 1;
}

int dzmq_spin_once(long timeout)
{
    int rc = zmq_poll(poll_items, poll_items_count - 1, timeout);
    if (rc < 0)
    {
        switch (errno)
        {
            case EINTR:
                return 1;
            case EFAULT:
                perror("Invalid poll items");
                return 0;
            case ETERM:
                perror("Socket terminated");
                return 0;
            default:
                perror("Unknown error in zmq_poll");
                return 0;
        }
    }

    /* timeout */
    if (rc == 0)
    {
        return 1;
    }

    if (poll_items[0].revents & ZMQ_POLLIN)
    {
        uint8_t buffer[DZMQ_UDP_MAX_SIZE];
        socklen_t len_rcv_addr = sizeof(rcv_addr);
        int ret = recvfrom(bcast_fd, buffer, DZMQ_UDP_MAX_SIZE, 0, (struct sockaddr *) &rcv_addr, &len_rcv_addr);
        if (ret < 0)
        {
            perror("Error in recvfrom on broadcast socket");
            return 0;
        }
        return handle_bcast_msg(buffer, ret);
    }

    return 1;
}

int dzmq_spin()
{
    int ret = 0;
    while (0 < (ret = dzmq_spin_once(200))) {}
    return ret;
}
