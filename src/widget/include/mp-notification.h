/*
 * Copyright 2012	  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MP_NOTIFICATION_H__
#define __MP_NOTIFICATION_H__

#include <stdbool.h>

typedef void *mp_noti_h;

#ifdef __cplusplus
extern "C" {
#endif

mp_noti_h mp_noti_create(const char *title);
void mp_noti_destroy(mp_noti_h noti);
bool mp_noti_update_size(mp_noti_h noti, unsigned long long total, unsigned long long byte);

#ifdef __cplusplus
}
#endif

#endif /* __MP_NOTIFICATION_H__ */

