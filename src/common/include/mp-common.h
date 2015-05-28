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

#ifndef __mp_common_H__
#define __mp_common_H__

#include <Elementary.h>
#include "music.h"
#include "mp-view-layout.h"

struct text_part
{
	char *part;
	char *msgid;
};

char *mp_common_track_list_label_get(void *data, Evas_Object * obj, const char *part);
Evas_Object *mp_common_track_list_icon_get(void *data, Evas_Object * obj, const char *part);
Evas_Object *mp_common_create_editfield_layout(Evas_Object * parent, struct appdata *ad, char *text);
void mp_common_track_genlist_sel_cb(void *data, Evas_Object * obj, void *event_info);
int mp_check_db_initializing(void);

void mp_common_hide_search_ise_context(view_data_t * view_data);
void mp_common_search_button_cb(void *data, Evas_Object * obj, void *event_info);
void mp_common_edit_button_cb(void *data, Evas_Object * obj, void *event_info);
void mp_common_back_button_cb(void *data, Evas_Object * obj, void *event_info);
void mp_common_item_check_changed_cb(void *data, Evas_Object * obj, void *event_info);
void mp_common_set_toolbar_button_sensitivity(mp_layout_data_t * layout_data, int selected_count);
void mp_common_change_item_class(Evas_Object * genlist, Elm_Genlist_Item_Class * itc);
void mp_common_navigationbar_finish_effect(void *data, Evas_Object * obj, void *event_info);

#endif // __mp_common_H__
