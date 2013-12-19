#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>

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
#include "impl/timing.h"
#include "impl/connection.h"
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
void * zmq_subscribe_sock;
zmq_pollitem_t poll_items[DZMQ_MAX_POLL_ITEMS];
size_t poll_items_count = 0;

dzmq_topic_list_t published_topics;
dzmq_topic_list_t subscribed_topics;
dzmq_connection_list_t connections;

dzmq_timer_callback_t * timer_callback = 0;
long timer_period = 0;
struct timespec last_timer = {0, 0};

int register_file_descriptor(int fd)
{
    if (poll_items_count >= DZMQ_MAX_POLL_ITEMS)
    {
        fprintf(stderr, "Too many devices to poll\n");
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
        fprintf(stderr, "Too many devices to poll\n");
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
        perror("Error binding broadcast socket");
        return 0;
    }
    /* Add the bcast socket to the zmq poller */
    register_file_descriptor(bcast_fd);
    /* Setup zmq context */
    zmq_context = zmq_ctx_new();
    /* Setup publisher zmq socket */
    zmq_publish_sock = zmq_socket(zmq_context, ZMQ_PUB);
    /* Bind publisher to tcp transport */
    char publish_addr[DZMQ_MAX_ADDR_LENGTH];
    sprintf(publish_addr, "tcp://%s:*", ip_address);
    if (0 > zmq_bind(zmq_publish_sock, publish_addr))
    {
        perror("Error binding zmq socket to tcp");
        return 0;
    }
    size_t size_of_tcp_address = sizeof(tcp_address);
    memset(tcp_address, 0, size_of_tcp_address);
    if (0 > zmq_getsockopt(zmq_publish_sock, ZMQ_LAST_ENDPOINT, tcp_address, &size_of_tcp_address))
    {
        perror("Error getting endpoint address of publisher socket");
        return 0;
    }
    /* Bind publisher to inproc transport */
    if (0 > zmq_bind(zmq_publish_sock, DZMQ_INPROC_ADDR))
    {
        perror("Error binding zmq socket to inproc");
        return 0;
    }
    /* Setup subscriber socket */
    zmq_subscribe_sock = zmq_socket(zmq_context, ZMQ_SUB);
    zmq_connect(zmq_subscribe_sock, DZMQ_INPROC_ADDR);
    register_socket(zmq_subscribe_sock);
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
        fprintf(stderr, "(dzmq_advertise) dzmq_init must be called first\n");
        return 0;
    }
    if (dzmq_topic_in_list(&published_topics, topic_name))
    {
        fprintf(stderr, "Cannot advertise the topic '%s', which has already been advertised\n", topic_name);
        return 0;
    }
    printf("Advertising topic '%s'\n", topic_name);
    /* Add topic to publisher list */
    if (!dzmq_topic_list_append(&published_topics, topic_name, 0))
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

int dzmq_subscribe(const char * topic_name, dzmq_msg_callback_t * callback)
{
    if (!init_called)
    {
        fprintf(stderr, "(dzmq_subscribe) dzmq_init must be called first\n");
        return 0;
    }
    if (dzmq_topic_in_list(&subscribed_topics, topic_name))
    {
        fprintf(stderr, "Cannot subscribe to the topic '%s', which has already been subscribed\n", topic_name);
        return 0;
    }
    printf("Subscribing to topic '%s'\n", topic_name);
    /* Add topic to subscriber list */
    if (!dzmq_topic_list_append(&subscribed_topics, topic_name, callback))
    {
        return 0;
    }
    /* Add subscription filter to inproc */
    if (0 != zmq_setsockopt(zmq_subscribe_sock, ZMQ_SUBSCRIBE, topic_name, strlen(topic_name)))
    {
        fprintf(stderr, "Error subscribing to topic '%s'\n", topic_name);
    }
    return send_sub(topic_name);
}

int dzmq_publish(const char * topic_name, const uint8_t * msg, size_t len)
{
    if (!init_called)
    {
        fprintf(stderr, "(dzmq_publish) dzmq_init must be called first\n");
        return 0;
    }
    if (!dzmq_topic_in_list(&published_topics, topic_name))
    {
        fprintf(stderr, "Cannot publish to topic '%s' which is unadvertised\n", topic_name);
        return 0;
    }
    /* Construct a header for the message */
    dzmq_msg_header_t header;
    header.version = 0x01;
    memcpy(header.guid, GUID, GUID_LEN);
    strcpy(header.topic, topic_name);
    header.type = DZMQ_OP_PUB;
    memset(header.flags, 0, DZMQ_FLAGS_LENGTH);
    uint8_t buffer[DZMQ_UDP_MAX_SIZE];
    size_t header_len = serialize_msg_header(buffer, &header);
    /* Send the topic as the first part of a three part message */
    zmq_msg_t topic_msg;
    assert(0 == zmq_msg_init_size(&topic_msg, strlen(topic_name)));
    memcpy(zmq_msg_data(&topic_msg), topic_name, strlen(topic_name));
    assert(strlen(topic_name) == zmq_msg_send(&topic_msg, zmq_publish_sock, ZMQ_SNDMORE));
    zmq_msg_close(&topic_msg);
    /* Send the header the next part */
    zmq_msg_t header_msg;
    assert(0 == zmq_msg_init_size(&header_msg, header_len));
    memcpy(zmq_msg_data(&header_msg), buffer, header_len);
    assert(header_len == zmq_msg_send(&header_msg, zmq_publish_sock, ZMQ_SNDMORE));
    zmq_msg_close(&header_msg);
    /* Finally send the data */
    zmq_msg_t data_msg;
    assert(0 == zmq_msg_init_size(&data_msg, len));
    memcpy(zmq_msg_data(&data_msg), msg, len);
    assert(len == zmq_msg_send(&data_msg, zmq_publish_sock, 0));
    zmq_msg_close(&data_msg);
    return 1;
}

int dzmq_timer(dzmq_timer_callback_t * callback, long timer_period_ms)
{
    if (timer_period_ms < 0)
    {
        fprintf(stderr, "Cannot set a timer with period less than 0");
        return 0;
    }
    if (callback != 0 && timer_period_ms == 0)
    {
        fprintf(stderr, "Cannot set a timer with period 0");
        return 0;
    }
    timer_callback = callback;
    timer_period = timer_period_ms;
    dzmq_get_time_now(&last_timer);
    return 1;
}

int dzmq_clear_timer()
{
    return dzmq_timer(0, 0);
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
            if (0 != dzmq_addr_in_list(&connections, adv_msg.addr))
            {
                printf("Skipping connection to address '%s', because it has already been made\n", adv_msg.addr);
                return 1;
            }
            if (0 != zmq_connect(zmq_subscribe_sock, adv_msg.addr))
            {
                fprintf(stderr, "Error connecting to addr '%s'\n", adv_msg.addr);
                return 0;
            }
            if (!dzmq_connection_list_append(&connections, -1, adv_msg.addr))
            {
                fprintf(stderr, "Failed to add connection to list\n");
                return 0;
            }
            return 1;
        }
        else
        {
            fprintf(stderr, "Unkown protocol type for address: %s\n", adv_msg.addr);
            return 0;
        }
    }
    else if (header.type == DZMQ_OP_SUB)
    {
        if (0 != dzmq_topic_in_list(&published_topics, header.topic))
        {
            /* Resend the ADV message */
            // printf("Resending ADV for topic '%s'\n", header.topic);
            return send_adv(header.topic);
        }
    }
    else
    {
        fprintf(stderr, "Unknown type: %i\n", header.type);
    }
    return 1;
}

int dzmq_spin_once(long timeout)
{
    if (!init_called)
    {
        fprintf(stderr, "(dzmq_spin_once) dzmq_init must be called first\n");
        return 0;
    }
    /* If there is a timer set */
    long time_till_timer = -1;
    if (0 != timer_callback)
    {
        /* Determine the time until the next firing */
        time_till_timer = dzmq_time_till(&last_timer, timer_period);
        /* If we have no time left, run the callback now */
        if (time_till_timer <= 0)
        {
            dzmq_get_time_now(&last_timer);
            timer_callback();
            return 1;
        }
    }
    /* Poll for either the given timeout, or the time until the next timer, which ever is shorter */
    long zmq_timeout = (-1 != time_till_timer && time_till_timer < timeout) ? time_till_timer : timeout;
    int rc = zmq_poll(poll_items, poll_items_count, zmq_timeout);
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

    /* Timeout */
    if (rc == 0)
    {
        /* TODO: check timer again */
        return 1;
    }

    /* Check for incoming broadcast messages */
    if (0 < poll_items_count && poll_items[0].revents & ZMQ_POLLIN)
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
    /* Check for incoming ZMQ messages */
    if (poll_items[1].revents & ZMQ_POLLIN)
    {
        char topic[DZMQ_MAX_TOPIC_LENGTH];
        dzmq_msg_header_t header;
        /* Get the topic msg */
        zmq_msg_t topic_msg;
        assert(0 == zmq_msg_init(&topic_msg));
        assert(-1 != zmq_msg_recv(&topic_msg, zmq_subscribe_sock, 0));
        memcpy(topic, zmq_msg_data(&topic_msg), zmq_msg_size(&topic_msg));
        topic[zmq_msg_size(&topic_msg)] = 0;
        assert(zmq_msg_more(&topic_msg));
        zmq_msg_close(&topic_msg);
        /* Get the header msg */
        zmq_msg_t header_msg;
        assert(0 == zmq_msg_init(&header_msg));
        assert(-1 != zmq_msg_recv(&header_msg, zmq_subscribe_sock, 0));
        deserialize_msg_header(&header, (uint8_t *) zmq_msg_data(&header_msg), zmq_msg_size(&header_msg));
        int more = zmq_msg_more(&header_msg);
        zmq_msg_close(&header_msg);
        if (header.type == DZMQ_OP_PUB)
        {
            /* Find subscriber */
            dzmq_topic_t * subscriber = dzmq_topic_in_list(&subscribed_topics, topic);
            if (!subscriber)
            {
                fprintf(stderr, "Could not find subscriber for topic '%s'\n", topic);
                return 0;
            }
            /* Receive final data msg */
            assert(more);
            zmq_msg_t data_msg;
            assert(0 == zmq_msg_init(&data_msg));
            assert(-1 != zmq_msg_recv(&data_msg, zmq_subscribe_sock, 0));
            size_t data_len = zmq_msg_size(&data_msg);
            uint8_t * data = (uint8_t *) zmq_msg_data(&data_msg);
            subscriber->callback(topic, data, data_len);
        }
        return 1;
    }

    return 1;
}

int dzmq_spin()
{
    int ret = 0;
    while (0 < (ret = dzmq_spin_once(200))) {}
    return ret;
}
