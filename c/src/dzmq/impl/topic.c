#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "topic.h"

int dzmq_topic_list_append(dzmq_topic_list_t * topic_list, const char * topic, dzmq_msg_callback_t * callback)
{
    dzmq_topic_t * new_topic = (struct dzmq_topic_t *) malloc(sizeof(struct dzmq_topic_t));
    if (0 == new_topic)
    {
        perror("Error appending topic to list");
        return 0;
    }
    new_topic->next = 0;
    strncpy(new_topic->name, topic, DZMQ_MAX_TOPIC_LENGTH);
    new_topic->callback = callback;
    if (0 == topic_list->last && 0 == topic_list->first)
    {
        new_topic->prev = 0;
        topic_list->last = new_topic;
        topic_list->first = new_topic;
    }
    else
    {
        new_topic->prev = topic_list->last;
        topic_list->last->next = new_topic;
        topic_list->last = new_topic;
    }
    return 1;
}

int dzmq_topic_list_remove(dzmq_topic_t * topic)
{
    if (topic->next)
    {
        topic->next->prev = topic->prev;
    }
    if (topic->prev)
    {
        topic->prev->next = topic->next;
    }
    free(topic);
    return 1;
}

dzmq_topic_t * dzmq_topic_in_list(dzmq_topic_list_t * topic_list, const char * topic_name)
{
    dzmq_topic_t * topic = topic_list->first;
    while (topic != 0)
    {
        if (0 == strcmp(topic->name, topic_name))
        {
            break;
        }
        topic = topic->next;
    }
    return topic;
}

int dzmq_topic_list_destroy(dzmq_topic_list_t * topic_list)
{
    dzmq_topic_t * topic = topic_list->first;
    while (1)
    {
        if (0 == topic->next)
        {
            free(topic);
            return 1;
        }
        topic = topic->next;
        free(topic->prev);
    }
}
