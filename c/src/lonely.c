#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dzmq/dzmq.h"

const char * topic_name;
size_t seq = 0;

void callback(const char * topic_name, const uint8_t * msg, size_t len)
{
    printf("Received message (%lu): %s\n", len, (char *) msg);
}

void publish_callback()
{
    char msg[255];
    sprintf(msg, "Hello World: %lu", seq++);
    dzmq_publish(topic_name, (uint8_t *) msg, strlen(msg) + 1);
}

int main(int argc, const char * argv[])
{
    /* Initialize dzmq */
    if (!dzmq_init()) return 1;

    /* Determine the topic name */
    topic_name = (argc > 1) ? argv[1] : "foo";

    /* Subscribe */
    if (!dzmq_subscribe(topic_name, callback)) return 1;

    /* Advertise the topic */
    if (!dzmq_advertise(topic_name)) return 1;

    /* Setup timer for publish once a second */
    if (!dzmq_timer(publish_callback, 1000)) return 1;

    /* Spin */
    if(!dzmq_spin()) return 1;

    return 0;
}
