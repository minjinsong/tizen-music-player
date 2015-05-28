/*
 * Copyright 2012	 Samsung Electronics Co., Ltd
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


#ifndef __MP_APP_H__
#define __MP_APP_H__

#include <sys/inotify.h>
#include <app.h>

typedef enum _mp_inotify_event
{
	MP_INOTI_NONE = 0,
	MP_INOTI_CREATE,	// IN_CREATE
	MP_INOTI_DELETE,	// IN_DELETE
	MP_INOTI_MODIFY,	// IN_MODIFY
	MP_INOTI_MOVE_OUT,	// IN_MOVED_FROM
	MP_INOTI_MOVE_IN,	// IN_MOVED_TO
	//MP_INOTI_DELETE_SELF,         // IN_DELETE_SELF
	//MP_INOTI_MOVE_SELF,           // IN_MOVE_SELF
	MP_INOTI_MAX,
} mp_inotify_event;

typedef void (*mp_inotify_cb) (mp_inotify_event event, char *name, void *data);

void mp_app_exit(void *data);
bool mp_app_noti_init(void *data);
bool mp_app_noti_ignore(void);

#ifdef MP_ENABLE_INOTIFY
int mp_app_inotify_init(void *data);
void mp_app_inotify_finalize(struct appdata *ad);
int mp_app_inotify_add_watch(const char *path, mp_inotify_cb callback, void *user_data);
int mp_app_inotify_rm_watch(int index);
#endif

Eina_Bool mp_app_key_down_cb(void *data, int type, void *event);
Eina_Bool mp_app_key_up_cb(void *data, int type, void *event);
Eina_Bool mp_app_mouse_event_cb(void *data, int type, void *event);
bool mp_app_grab_mm_keys(struct appdata *ad);
void mp_app_ungrab_mm_keys(struct appdata *ad);
void mp_app_live_box_init(struct appdata *ad);
void mp_app_live_box_deinit(struct appdata *ad);

#endif // __MP_APP_H__
