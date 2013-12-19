#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"

int dzmq_connection_list_append(dzmq_connection_list_t * list, int fd, const char * addr)
{
    dzmq_connection_t * new_connection = (struct dzmq_connection_t *) malloc(sizeof(struct dzmq_connection_t));
    if (0 == new_connection)
    {
        perror("Error appending connection to list");
        return 0;
    }
    new_connection->next = 0;
    new_connection->fd = fd;
    strncpy(new_connection->addr, addr, DZMQ_MAX_ADDR_LENGTH);
    if (0 == list->last && 0 == list->first)
    {
        new_connection->prev = 0;
        list->last = new_connection;
        list->first = new_connection;
    }
    else
    {
        new_connection->prev = list->last;
        list->last->next = new_connection;
        list->last = new_connection;
    }
    return 1;
}

int dzmq_connection_list_remove(dzmq_connection_t * connection)
{
    if (connection->next)
    {
        connection->next->prev = connection->prev;
    }
    if (connection->prev)
    {
        connection->prev->next = connection->next;
    }
    free(connection);
    return 1;
}

dzmq_connection_t * dzmq_fd_in_list(dzmq_connection_list_t * list, int fd)
{
    dzmq_connection_t * connection = list->first;
    while (connection != 0)
    {
        if (connection->fd == fd)
        {
            break;
        }
        connection = connection->next;
    }
    return connection;
}

dzmq_connection_t * dzmq_addr_in_list(dzmq_connection_list_t * list, const char * addr)
{
    dzmq_connection_t * connection = list->first;
    while (connection != 0)
    {
        if (0 == strcmp(connection->addr, addr))
        {
            break;
        }
        connection = connection->next;
    }
    return connection;
}

int dzmq_connection_list_destroy(dzmq_connection_list_t * list)
{
    dzmq_connection_t * connection = list->first;
    while (1)
    {
        if (0 == connection->next)
        {
            free(connection);
            return 1;
        }
        connection = connection->next;
        free(connection->prev);
    }
}
