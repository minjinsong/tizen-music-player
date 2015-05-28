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
Evas_Object * mp_group_view_create(struct appdata *ad, mp_view_type_t type);
void mp_group_view_destroy(Evas_Object * group_view);
void mp_group_view_refresh(Evas_Object * group_view);
bool mp_group_view_create_by_group_name(Evas_Object * obj, char *group_name, mp_group_type_e type);
Evas_Object * mp_group_view_icon_get(void *data, Evas_Object * obj, const char *part);
char *mp_group_view_album_list_label_get(void *data, Evas_Object * obj, const char *part);
void mp_group_view_group_list_select_cb(void *data, Evas_Object * obj, void *event_info);
char *mp_group_view_list_label_get(void *data, Evas_Object * obj, const char *part);
Evas_Object * mp_group_view_icon_get(void *data, Evas_Object * obj, const char *part);

