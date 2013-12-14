/*
 * Copyright (C) 2013 Open Source Robotics Foundation
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

#ifndef DISC_ZMQ__DISC_ZMQ_H
#define DISC_ZMQ__DISC_ZMQ_H

#include <stdlib.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>

/* Defaults and overrides */
#define ADV_SUB_PORT 11312
#define ADV_SUB_HOST '255.255.255.255'
#define DZMQ_PORT_KEY 'DZMQ_BCAST_PORT'
#define DZMQ_HOST_KEY 'DZMQ_BCAST_HOST'
#define DZMQ_IP_KEY 'DZMQ_IP'

/* Constants */
#define OP_ADV 0x01
#define OP_SUB 0x02
#define UDP_MAX_SIZE 512
#define ADV_REPEAT_PERIOD 1.0

/* Typedefs */
#define MAX_DZMQ_HANDLES 1024
#define DZMQ_IPV4_ADDRSTRLEN INET_ADDRSTRLEN

typedef size_t dzmq_handle_t;

dzmq_handle_t dzmq_init();
void dzmq_free(dzmq_handle_t handle);

void dzmq_set_ip_addr(dzmq_handle_t handle, char * address);

int dzmq_get_address_ipv4(char * address);

#endif /* DISC_ZMQ__DISC_ZMQ_H */
