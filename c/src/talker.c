#include <stdio.h>
#include <string.h>

#include "dzmq/dzmq.h"

int main(int argc, const char * argv[])
{
    /* Initialize dzmq */
    if (0 >= dzmq_init())
    {
        perror("Failed to initialize dzmq");
        return 1;
    }

    /* Determine the topic name */
    const char * topic_name;
    if (argc > 2)
    {
        topic_name = argv[2];
    }
    else
    {
        topic_name = "foo";
    }

    if (0 >= dzmq_advertise(topic_name))
    {
        perror("Error advertising topic");
        return 1;
    }

    while (1)
    {
        char msg[4] = "bar";
        if (0 >= dzmq_publish(topic_name, (uint8_t *) msg, strlen(msg)))
        {
            perror("Failed to publish");
            return 1;
        }
        if (0 >= dzmq_spin_once(1000))
        {
            perror("Spin failed");
            return 1;
        }
    }
    return 0;
}
