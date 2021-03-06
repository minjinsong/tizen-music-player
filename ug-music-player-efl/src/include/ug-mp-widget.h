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

#ifndef __UG_MP_WIDGET_H__
#define __UG_MP_WIDGET_H__

#include <Elementary.h>

typedef enum {
	NO_CONTENT_SONG,
	NO_CONTENT_PLAYLIST,
	NO_CONTENT_ALBUM,
	NO_CONTENT_ARTIST,
} NoContentType_e;

Evas_Object *ug_mp_widget_no_content_add(Evas_Object *parent, NoContentType_e type);

#endif /* __UG_MP_WIDGET_H__ */

