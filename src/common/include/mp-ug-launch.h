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


#ifndef __MP_ug_launch_H_
#define __MP_ug_launch_H_

int mp_ug_email_attatch_file(const char *filepath, void *user_data);
int mp_ug_bt_attatch_file(const char *filepath, int count, void *user_data);
int mp_ug_message_attatch_file(const char *filepath, void *user_data);
int mp_ug_contact_user_sel(const char *filepath, void *user_data);
int mp_ug_show_info(struct appdata *ad);
void mp_ug_send_message(struct appdata *ad, mp_ug_message_t message);
#ifdef MP_FEATURE_WIFI_SHARE
int mp_ug_wifi_attatch_file(const char *filepath, int count, void *user_data);
#endif
void mp_ug_destory_current(struct appdata *ad);
void mp_ug_destory_all(struct appdata *ad);
bool mp_ug_active(struct appdata *ad);

bool mp_send_via_appcontrol(struct appdata *ad, mp_send_type_e send_type, const char *files);

#endif // __MP_ug_launch_H_
