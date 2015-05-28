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


#include "music.h"
#include "mp-view-layout.h"
	Evas_Object * mp_edit_view_create(view_data_t * view_data);
void mp_edit_view_back_button_cb(void *data, Evas_Object * obj, void *event_info);
void mp_edit_view_create_new_cancel_cb(void *data, Evas_Object * obj, void *event_info);
void mp_edit_view_create_new_done_cb(void *data, Evas_Object * obj, void *event_info);
void mp_edit_view_delete_cb(void *data, Evas_Object * obj, void *event_info);
void mp_edit_view_add_to_plst_cb(void *data, Evas_Object * obj, void *event_info);
void mp_edit_view_share_cb(void *data, Evas_Object * obj, void *event_info);
void mp_edit_view_excute_edit(mp_layout_data_t * layout_data, mp_edit_operation_t edit_operation);
void mp_edit_view_cencel_cb(void *data, Evas_Object * obj, void *event_info);
void mp_edit_view_genlist_sel_cb(void *data, Evas_Object * obj, void *event_info);
bool mp_edit_view_get_selected_track_list(void *data, GList **p_list);

