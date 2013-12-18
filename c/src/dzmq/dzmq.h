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

#ifndef DZMQ__DZMQ_H
#define DZMQ__DZMQ_H

#include <stddef.h>
#include <stdint.h>
#include <uuid/uuid.h>

#include "impl/callback.h"

int dzmq_init();

int dzmq_advertise(const char * topic_name);

int dzmq_subscribe(const char * topic_name, dzmq_msg_callback_t * callback);

int dzmq_publish(const char * topic_name, const uint8_t * msg, size_t len);

int dzmq_timer(dzmq_timer_callback_t * callback, long period_ms);

int dzmq_clear_timer();

int dzmq_spin_once(long timeout_ms);

int dzmq_spin();

#endif /* DZMQ__DZMQ_H */
