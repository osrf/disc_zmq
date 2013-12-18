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

#ifndef DZMQ__ADV_MSG_H
#define DZMQ__ADV_MSG_H

#include <stdint.h>
#include <uuid/uuid.h>

#include "config.h"
#include "header.h"

typedef struct {
    dzmq_msg_header_t header;
    char addr[DZMQ_MAX_ADDR_LENGTH];
} dzmq_adv_msg_t;

size_t serialize_adv_msg(uint8_t * buffer, dzmq_adv_msg_t * adv_msg);

size_t deserialize_adv_msg(dzmq_adv_msg_t * adv_msg, uint8_t * buffer, size_t len);

#endif /* DZMQ__ADV_MSG_H */
