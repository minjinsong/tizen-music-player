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

#ifndef __MP_PLAYLIST_VIEW_H_
#define __MP_PLAYLIST_VIEW_H_

#include "music.h"
#include "mp-view-layout.h"
Evas_Object * mp_playlist_view_create(struct appdata *ad, mp_view_type_t type);
void mp_playlist_view_destroy(Evas_Object * playlist);
void mp_playlist_view_refresh(Evas_Object * playlist);
void mp_playlist_view_create_auto_playlist(struct appdata *ad, char *type);
bool mp_playlist_view_create_by_id(Evas_Object * obj, int p_id);
void mp_playlist_view_update_navibutton(mp_layout_data_t * layout_data);
bool mp_playlist_view_reset_rename_mode(struct appdata *ad);
void mp_playlist_view_create_playlist_button_cb(void *data, Evas_Object * obj, void *event_info);
void mp_playlist_view_create_new_cancel_cb(void *data, Evas_Object * obj, void *event_info);
void mp_playlist_view_create_new_done_cb(void *data, Evas_Object * obj, void *event_info);
void mp_playlist_view_rename_done_cb(void *data, Evas_Object * obj, void *event_info);
void mp_playlist_view_rename_cancel_cb(void *data, Evas_Object * obj, void *event_info);
void mp_playlist_view_add_button_cb(void *data, Evas_Object * obj, void *event_info);

#endif //__MP_PLAYLIST_VIEW_H_
