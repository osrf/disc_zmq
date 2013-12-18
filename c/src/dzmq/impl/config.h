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

#ifndef DZMQ__CONFIG_H
#define DZMQ__CONFIG_H

/* Defaults and overrides */
#define DZMQ_DISC_PORT 11312
#define DZMQ_INPROC_ADDR "inproc://topics"

/* Constants */
#define DZMQ_OP_ADV 0x01
#define DZMQ_OP_SUB 0x02
#define DZMQ_OP_PUB 0x03

#define DZMQ_UDP_MAX_SIZE 512
#define DZMQ_ADV_REPEAT_PERIOD 1.0
#define DZMQ_MAX_TOPIC_LENGTH 193 + 1
#define DZMQ_MAX_ADDR_LENGTH 267 + 1
#define DZMQ_FLAGS_LENGTH 16
#define DZMQ_MAX_POLL_ITEMS 1024

#endif /* DZMQ__CONFIG_H */
