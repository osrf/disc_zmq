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

#ifndef DZMQ__TIMING_H
#define DZMQ__TIMING_H

#include <time.h>

void dzmq_get_time_now(struct timespec * time);

long dzmq_time_till(struct timespec * last_time, long timer_period);

#endif /* DZMQ__TIMING_H */
