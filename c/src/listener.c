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
    if (!dzmq_init()) return 1;

    /* Determine the topic name */
    const char * topic_name = (argc > 1) ? argv[1] : "foo";

    /* Subscribe */
    if (!dzmq_subscribe(topic_name, callback)) return 1;

    /* Spin */
    if(!dzmq_spin()) return 1;

    return 0;
}
