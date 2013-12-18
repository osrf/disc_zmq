#include <stdio.h>
#include <string.h>

#include "dzmq/dzmq.h"

int main(int argc, const char * argv[])
{
    /* Initialize dzmq */
    if (!dzmq_init()) return 1;

    /* Determine the topic name */
    const char * topic_name;
    if (argc > 2)
        topic_name = argv[2];
    else
        topic_name = "foo";

    /* Advertise the topic */
    if (!dzmq_advertise(topic_name)) return 1;

    char msg[4] = "bar";
    while (1)
    {
        /* Publish once */
        if (!dzmq_publish(topic_name, (uint8_t *) msg, strlen(msg))) return 1;
        /* Spin once */
        if (!dzmq_spin_once(1000)) return 1;
    }
    return 0;
}
