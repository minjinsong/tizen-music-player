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

#include "ug-mp-group-list.h"
#include "mp-media-info.h"
#include "ug-mp-common.h"
#include "ug-mp-track-list.h"

typedef struct{
	struct ug_data *ugd;

	Evas_Object *no_content;
	Evas_Object *genlist;

	Elm_Genlist_Item_Class itc;

	mp_group_type_e type;
	char *type_str;
	int playlist_id;

	mp_media_list_h group_list;
}group_list_data_t;

#define GET_LIST_DATA(obj)	evas_object_data_get(obj, "list_data")

static Evas_Object *
_ug_mp_create_genlist(Evas_Object *parent)
{
	Evas_Object *genlist = NULL;
	MP_CHECK_NULL(parent);

	genlist = elm_genlist_add(parent);
	elm_genlist_select_mode_set(genlist, ELM_OBJECT_SELECT_MODE_ALWAYS);

	return genlist;
}

#if 0
#define TIME_FORMAT_LEN	15
static void
_format_duration(char *time, int ms)
{
	int sec = (ms + 500) / 1000;
	int min = sec / 60;

	if(min >= 60)
	{
		int hour = min / 60;
		snprintf(time, TIME_FORMAT_LEN, "%02u:%02u:%02u", hour, min % 60, sec % 60);
	}
	else
		snprintf(time, TIME_FORMAT_LEN, "%02u:%02u", min, sec % 60);
}
#endif

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	group_list_data_t *ld  = data;
	char *name = NULL;
	int track_type = MP_GROUP_BY_ALBUM;
	int playlist_id = 0;

	elm_genlist_item_selected_set(event_info, EINA_FALSE);
	MP_CHECK(ld);

	mp_media_info_h media = elm_object_item_data_get(event_info);
	MP_CHECK(media);
	mp_media_info_group_get_main_info(media, &name);
	DEBUG_TRACE("group_type: ld->type: %d, path: %s", ld->type, name);

	if(ld->type == MP_GROUP_BY_PLAYLIST)
	{
		track_type = MP_TRACK_BY_PLAYLIST;
		mp_media_info_group_get_playlist_id(media, &playlist_id);
	}
	else if(ld->type == MP_GROUP_BY_ARTIST)
		track_type = MP_TRACK_BY_ARTIST;

	Evas_Object *track_list = ug_mp_track_list_create(obj, ld->ugd, NULL);
	ug_mp_track_list_set_data(track_list, track_type, name, playlist_id);
	ug_mp_track_list_update(track_list);

	elm_naviframe_item_push(ld->ugd->navi_bar, name, NULL, NULL, track_list, NULL);

}

static void
_layout_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	group_list_data_t *ld  = data;
	MP_CHECK(ld);

	IF_FREE(ld->type_str);

	free(ld);
}

static void
_ug_mp_itc_init(int type, group_list_data_t *ld)
{
	MP_CHECK(ld);

	ld->itc.func.content_get = ug_mp_group_content_get;
	switch(type)
	{
	case MP_GROUP_BY_ALBUM:
		ld->itc.item_style = "3text.1icon.2";
		ld->itc.func.text_get = ug_mp_album_text_get;
		ld->type = MP_GROUP_BY_ALBUM;
		break;
	case MP_GROUP_BY_ARTIST:
		ld->itc.item_style = "2text.1icon";
		ld->itc.func.text_get = ug_mp_artist_text_get;
		ld->type = MP_GROUP_BY_ARTIST;
		break;
	case MP_GROUP_BY_PLAYLIST:
		ld->itc.item_style = "2text.1icon";
		ld->itc.func.text_get = ug_mp_playlist_text_get;
		ld->type = MP_GROUP_BY_PLAYLIST;
		break;
	default:
		ERROR_TRACE("Invalid vd->type: %d", type);
		break;
	}
}


Evas_Object *ug_mp_group_list_create(Evas_Object *parent, struct ug_data *ugd)
{
	startfunc;
	Evas_Object *layout ;
	group_list_data_t *ld = NULL;

	MP_CHECK_NULL(parent);
	MP_CHECK_NULL(ugd);

	layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, "layout", "application", "default");
	MP_CHECK_NULL(layout);

	ld = calloc(1, sizeof(group_list_data_t));
	MP_CHECK_NULL(ld);

	ld->ugd = ugd;

	evas_object_data_set(layout, "list_data", ld);
	evas_object_event_callback_add(layout, EVAS_CALLBACK_FREE, _layout_del_cb, ld);

	return layout;
}

int ug_mp_group_list_update(Evas_Object *list)
{
	startfunc;
	Evas_Object *content;

	int count = 0;
	group_list_data_t *ld  = GET_LIST_DATA(list);
	MP_CHECK_VAL(ld, -1);

	if(ld->group_list)
	{
		mp_media_info_group_list_destroy(ld->group_list);
		ld->group_list = NULL;
	}

	content = elm_layout_content_get(list, "elm.swallow.content");
	evas_object_del(content);

	mp_media_info_group_list_count(ld->type, ld->type_str, NULL, &count);
	if(count)
	{
		content = _ug_mp_create_genlist(list);
		mp_media_info_group_list_create(&ld->group_list, ld->type, ld->type_str, NULL, 0, count);
		int i = 0;
		for(i=0; i<count; i++)
		{
			mp_media_info_h media =  mp_media_info_group_list_nth_item(ld->group_list, i);
			elm_genlist_item_append(content, &ld->itc, media, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, ld);
		}
	}
	else {
		NoContentType_e type = NO_CONTENT_SONG;
		if (ld->type == MP_GROUP_BY_PLAYLIST)
			type = NO_CONTENT_PLAYLIST;
		else if (ld->type == MP_GROUP_BY_ARTIST)
			type = NO_CONTENT_ARTIST;
		else if (ld->type == MP_GROUP_BY_ALBUM)
			type = NO_CONTENT_ALBUM;

		content = ug_mp_widget_no_content_add(list, type);
	}

	elm_layout_content_set(list, "elm.swallow.content", content);

	return 0;
}

int ug_mp_group_list_set_data(Evas_Object *list, int group_type, const char *type_str)
{
	startfunc;
	group_list_data_t *ld  = GET_LIST_DATA(list);
	MP_CHECK_VAL(ld, -1);

	ld->type = group_type;
	ld->type_str = g_strdup(type_str);

	_ug_mp_itc_init(group_type, ld);

	return 0;
}


