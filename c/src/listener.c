#include <stdio.h>
#include <string.h>

#include "dzmq/dzmq.h"

void callback(const char * topic_name, const uint8_t * msg, size_t len)
{
    printf("Received message (%lu): %s\n", len, (char *) msg);
}

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

    if (0 >= dzmq_subscribe(topic_name, callback))
    {
        perror("Error subscribing");
        return 1;
    }

    if(0 >= dzmq_spin())
    {
        perror("Error during spin");
        return 1;
    }
    return 0;
}
