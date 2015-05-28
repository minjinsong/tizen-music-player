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

#include "ug-mp-common.h"
#include "mp-media-info.h"

bool ug_mp_check_image_valid(Evas *evas, const char *path)
{
	if(!path) return false;
	MP_CHECK_FALSE(evas);

	if (!ecore_file_exists(path)) {
		ERROR_TRACE("file not exitst");
		return false;
	}

	Evas_Object *image = NULL;
	int width = 0;
	int height = 0;

	image = evas_object_image_add(evas);
	MP_CHECK_FALSE(image);
	evas_object_image_file_set(image, path, NULL);
	evas_object_image_size_get(image, &width, &height);
	evas_object_del(image);

	if (width <= 0 || height <= 0) {
		DEBUG_TRACE("Cannot load file : %s", path);
		return false;
	}

	return true;
}

char *ug_mp_artist_text_get(void *data, Evas_Object *obj, const char *part)
{
	char *text = NULL;
	if (strcmp(part, "elm.text.1") == 0) {
		mp_media_info_group_get_main_info(data, &text);
		return g_strdup(text);
	}else if(strcmp(part, "elm.text.2") == 0) {
		int count;
		mp_media_info_group_get_main_info(data, &text);
		mp_media_info_list_count(MP_TRACK_BY_ARTIST, text, NULL, NULL, 0, &count);
		text = g_strdup_printf("(%d)", count);
		return text;
	}
	return NULL;
}

char *ug_mp_album_text_get(void *data, Evas_Object *obj, const char *part)
{
	char *text = NULL;

	int ret = 0;
	if (strcmp(part, "elm.text.1") == 0) {
		ret = mp_media_info_group_get_main_info(data, &text);
		MP_CHECK_NULL(ret==0);
		return g_strdup(text);
	}else if (strcmp(part, "elm.text.2") == 0) {
		ret = mp_media_info_group_get_sub_info(data, &text);
		MP_CHECK_NULL(ret==0);
		return g_strdup(text);
	}else if(strcmp(part, "elm.text.3") == 0) {
		int count;
		ret = mp_media_info_group_get_main_info(data, &text);
		MP_CHECK_NULL(ret==0);
		ret = mp_media_info_list_count(MP_TRACK_BY_ALBUM, text, NULL, NULL, 0, &count);
		MP_CHECK_NULL(ret==0);
		text = g_strdup_printf("(%d)", count);
		return text;
	}
	return NULL;
}

char * ug_mp_playlist_text_get(void *data, Evas_Object *obj, const char *part)
{
	char *text = NULL;
	if (strcmp(part, "elm.text.1") == 0) {
		mp_media_info_group_get_main_info(data, &text);
		return g_strdup(GET_STR(text));
	}else if(strcmp(part, "elm.text.2") == 0) {
		int id = 0;
		int count = 0;
		mp_media_info_group_get_playlist_id(data, &id);
		if(id == MP_SYS_PLST_MOST_PLAYED)
			mp_media_info_list_count(MP_TRACK_BY_PLAYED_COUNT, NULL, NULL, NULL, 0, &count);
		else if(id == MP_SYS_PLST_RECENTELY_ADDED)
			mp_media_info_list_count(MP_TRACK_BY_ADDED_TIME, NULL, NULL, NULL, 0, &count);
		else if(id == MP_SYS_PLST_RECENTELY_PLAYED)
			mp_media_info_list_count(MP_TRACK_BY_PLAYED_TIME, NULL, NULL, NULL, 0, &count);
		else if(id == MP_SYS_PLST_QUICK_LIST)
			mp_media_info_list_count(MP_TRACK_BY_FAVORITE, NULL, NULL, NULL, 0, &count);
		else
			mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, id, &count);
		text = g_strdup_printf("(%d)", count);
		return text;
	}
	return NULL;
}

Evas_Object * ug_mp_group_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *content = NULL;
	char *thumbpath = NULL;

	content = elm_bg_add(obj);
	elm_bg_load_size_set(content, ICON_SIZE, ICON_SIZE);

	mp_media_info_group_get_thumbnail_path(data, &thumbpath);

	if(ug_mp_check_image_valid(	evas_object_evas_get(obj), thumbpath))
		elm_bg_file_set(content, thumbpath, NULL);
	else
		elm_bg_file_set(content, DEFAULT_THUMBNAIL, NULL);
	return content;
}

void
ug_mp_quit_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct ug_data *ugd = data;
	MP_CHECK(data);
	DEBUG_TRACE("");
	ug_destroy_me(ugd->ug);
}

