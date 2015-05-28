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
#ifndef __UG_MP_TRACK_LIST_H__
#define __UG_MP_TRACK_LIST_H__

#include "ug-mp-efl.h"
#include "mp-media-info.h"

Evas_Object *ug_mp_track_list_create(Evas_Object *parent, struct ug_data *ugd,  Elm_Object_Item *navi_it);
int ug_mp_track_list_set_data(Evas_Object *list, int track_type, const char *type_str, int playlist_id);
int ug_mp_track_list_update(Evas_Object *list);

#endif

