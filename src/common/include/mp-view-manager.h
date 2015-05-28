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

#ifndef __MP_VIEW_MANAGER_H_
#define __MP_VIEW_MANAGER_H_

#include "music.h"
#include "mp-view-layout.h"

typedef enum
{
	MP_VIEW_CONTENT_LIST,
	MP_VIEW_CONTENT_EDIT,
	MP_VIEW_CONTENT_PLAY,
	MP_VIEW_CONTENT_NEW_PLAYLIST,
	MP_VIEW_CONTENT_NEW_PLAYLIST_BY_SWEEP,
	MP_VIEW_CONTENT_NEW_PLAYLIST_BY_EDIT,
	MP_VIEW_CONTENT_INFO,
	MP_VIEW_CONTENT_SEARCH,
	MP_VIEW_CONTENT_MAX,
} mp_view_content_t;

typedef enum
{
	MP_NAVI_CONTROL_BUTTON_EDIT,
	MP_NAVI_CONTROL_BUTTON_SHARE,
	MP_NAVI_CONTROL_BUTTON_SEARCH,
	MP_NAVI_CONTROL_BUTTON_CREATE_PLAYLIST,
	MP_NAVI_CONTROL_BUTTON_ADD_TO_PLAYLIST,
	MP_NAVI_CONTROL_BUTTON_DELETE,
	MP_NAVI_CONTROL_BUTTON_REFRESH,
	MP_NAVI_CONTROL_BUTTON_DOWNLOAD,
	MP_NAVI_CONTROL_BUTTON_UPDATE_LIBRARY,
	MP_NAVI_CONTROL_BUTTON_OPEN_PLAYLIST,
	MP_NAVI_CONTROL_BUTTON_MAX,
} mp_navi_control_button_type;

Elm_Object_Item * mp_view_manager_push_view_content(view_data_t * view_data, Evas_Object * content, mp_view_content_t type);
void mp_view_manager_pop_view_content(view_data_t * view_data, bool pop_to_first, bool pop_content);
Evas_Object *mp_view_manager_get_last_view_layout(struct appdata *ad);
Evas_Object *mp_view_manager_get_first_view_layout(struct appdata *ad);
Evas_Object *mp_view_manager_get_edit_view_layout(struct appdata *ad);
Evas_Object *mp_view_manager_get_view_layout(struct appdata *ad, mp_view_content_t type);

int mp_view_manager_count_view_content(view_data_t * view_data);
bool mp_view_manager_is_play_view(struct appdata *ad);
void mp_view_manager_update_list_contents(view_data_t * view_data, bool update_edit_list);
void mp_view_manager_set_title_and_buttons(view_data_t * view_data, char *text_ID, void *data);

void mp_view_manager_set_now_playing(view_data_t * view_data, bool show);
void mp_view_manager_freeze_progress_timer(struct appdata *ad);
void mp_view_manager_thaw_progress_timer(struct appdata *ad);

void mp_view_manager_update_first_controlbar_item(void *data);
Elm_Object_Item *mp_view_manager_get_navi_item(struct appdata * ad);
Evas_Object *mp_view_manager_get_controlbar_item(struct appdata *ad, mp_navi_control_button_type type);
void mp_view_manager_play_view_title_label_set(struct appdata *ad, const char *title);

void mp_view_manager_pop_info_view(struct appdata *ad);
void mp_view_manager_pop_play_view(struct appdata *ad);

void mp_view_manager_unswallow_info_ug_layout(struct appdata *ad);

Elm_Object_Item * mp_view_manager_get_play_view_navi_item(struct appdata *ad);
void mp_view_manager_clear(struct appdata *ad);
void mp_view_manager_set_controlbar_visible(Elm_Object_Item *navi_item, bool visible);

void mp_view_manager_set_back_button(Evas_Object * parent, Elm_Object_Item* navi_item, Evas_Smart_Cb cb, void *data);

#endif //__MP_VIEW_MANAGER_H_
