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

#ifndef DZMQ__TOPIC_H
#define DZMQ__TOPIC_H

#include "callback.h"
#include "config.h"

typedef struct dzmq_topic_t {
    char name[DZMQ_MAX_TOPIC_LENGTH];
    dzmq_msg_callback_t * callback;
    struct dzmq_topic_t * next;
    struct dzmq_topic_t * prev;
} dzmq_topic_t;

typedef struct {
    struct dzmq_topic_t * first;
    struct dzmq_topic_t * last;
} dzmq_topic_list_t;

int dzmq_topic_list_append(dzmq_topic_list_t * topic_list, const char * topic, dzmq_msg_callback_t * callback);

int dzmq_topic_list_remove(dzmq_topic_t * topic);

dzmq_topic_t * dzmq_topic_in_list(dzmq_topic_list_t * topic_list, const char * topic_name);

int dzmq_topic_list_destroy(dzmq_topic_list_t * topic_list);

#endif /* DZMQ__TOPIC_H */
