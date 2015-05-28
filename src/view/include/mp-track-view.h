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


#ifndef __MP_TRACK_VIEW_H_
#define __MP_TRACK_VIEW_H_

#include "music.h"
	Evas_Object * mp_track_view_create(struct appdata *ad);
void mp_track_view_destroy(Evas_Object * track_view);
void mp_track_view_refresh(Evas_Object * track_view);
void mp_track_view_update_title_button(Evas_Object * track_view);
void mp_track_view_add_to_playlist_done_cb(void *data, Evas_Object * obj, void *event_info);
void mp_track_view_add_to_playlist_cancel_cb(void *data, Evas_Object * obj, void *event_info);

#endif	/*  */

