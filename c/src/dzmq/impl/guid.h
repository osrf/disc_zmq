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

#ifndef DZMQ__GUID_H
#define DZMQ__GUID_H

#include <uuid/uuid.h>

#define GUID_LEN sizeof(uuid_t)
#define GUID_STR_LEN (sizeof(uuid_t) * 2) + 4 + 1

void dzmq_guid_to_str(uuid_t guid, char * guid_str, int guid_str_len);

int dzmq_guid_compare(uuid_t guid, uuid_t other);

#endif /* DZMQ__GUID_H */
