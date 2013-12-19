/*
 * Copyright (C) 2013 Open Source Robotics Foundation, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DZMQ__CONNECTION_H
#define DZMQ__CONNECTION_H

#include "config.h"

typedef struct dzmq_connection_t {
    int fd;
    char addr[DZMQ_MAX_ADDR_LENGTH];
    struct dzmq_connection_t * next;
    struct dzmq_connection_t * prev;

} dzmq_connection_t;

typedef struct {
    struct dzmq_connection_t * first;
    struct dzmq_connection_t * last;
} dzmq_connection_list_t;

int dzmq_connection_list_append(dzmq_connection_list_t * list, int fd, const char * addr);

int dzmq_connection_list_remove(dzmq_connection_t * connection);

dzmq_connection_t * dzmq_fd_in_list(dzmq_connection_list_t * list, int fd);

dzmq_connection_t * dzmq_addr_in_list(dzmq_connection_list_t * list, const char * addr);

int dzmq_connection_list_destroy(dzmq_connection_list_t * list);

#endif /* DZMQ__CONNECTION_H */
