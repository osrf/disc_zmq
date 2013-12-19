#include <stdio.h>
#include <string.h>

#include "dzmq/dzmq.h"

const char * topic_name;

void publish_callback()
{
    char msg[4] = "bar";
    dzmq_publish(topic_name, (uint8_t *) msg, 4);
}

int main(int argc, const char * argv[])
{
    /* Initialize dzmq */
    if (!dzmq_init()) return 1;

    /* Determine the topic name */
    topic_name = (argc > 1) ? argv[1] : "foo";

    /* Advertise the topic */
    if (!dzmq_advertise(topic_name)) return 1;

    /* Setup timer for publish once a second */
    if (!dzmq_timer(publish_callback, 1000)) return 1;

    /* Spin */
    if(!dzmq_spin()) return 1;
    return 0;
}
