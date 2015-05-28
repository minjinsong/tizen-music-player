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

#ifndef __UG_MP_COMMON_H__
#define __UG_MP_COMMON_H__

#include "ug-mp-efl.h"

bool ug_mp_check_image_valid(Evas *evas, const char *path);
char *ug_mp_artist_text_get(void *data, Evas_Object *obj, const char *part);
char *ug_mp_album_text_get(void *data, Evas_Object *obj, const char *part);
char * ug_mp_playlist_text_get(void *data, Evas_Object *obj, const char *part);
Evas_Object * ug_mp_group_content_get(void *data, Evas_Object *obj, const char *part);
void ug_mp_quit_cb(void *data, Evas_Object * obj, void *event_info);

#endif
