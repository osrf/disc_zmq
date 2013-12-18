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

#ifndef DZMQ__HEADER_H
#define DZMQ__HEADER_H

#include <stdint.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "config.h"

typedef struct {
    uint16_t version;
    uuid_t guid;
    char topic[DZMQ_MAX_TOPIC_LENGTH];
    uint8_t type;
    uint8_t flags[DZMQ_FLAGS_LENGTH];
} dzmq_msg_header_t;

size_t serialize_msg_header(uint8_t * buffer, dzmq_msg_header_t * header);

size_t deserialize_msg_header(dzmq_msg_header_t * header, uint8_t * buffer, size_t len);

void dzmq_print_header(dzmq_msg_header_t * header);

#endif /* DZMQ__HEADER_H */
