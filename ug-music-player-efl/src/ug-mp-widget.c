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

#include "ug-mp-efl.h"

Evas_Object *
ug_mp_widget_no_content_add(Evas_Object *parent, NoContentType_e type)
{
	MP_CHECK_NULL(parent);
	Evas_Object *nocontents = NULL;
	nocontents = elm_layout_add(parent);
	elm_layout_theme_set(nocontents, "layout", "nocontents", "multimedia");

	const char *ids;
	if (type == NO_CONTENT_PLAYLIST)
		ids = UG_MP_TEXT_NO_PLAYLIST;
	else if (type == NO_CONTENT_ALBUM)
		ids = UG_MP_TEXT_ALBUMS;
	else if (type == NO_CONTENT_ARTIST)
		ids = UG_MP_TEXT_ARTISTS;
	else
		ids = UG_MP_TEXT_NO_SONGS;

	elm_object_text_set(nocontents, GET_STR(ids));
	elm_object_focus_allow_set(nocontents, EINA_TRUE);

	return nocontents;
}

