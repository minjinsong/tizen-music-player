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

#include "ug-mp-track-list.h"
#include "mp-media-info.h"
#include "ug-mp-common.h"

typedef struct{
	struct ug_data *ugd;

	Evas_Object *no_content;
	Evas_Object *genlist;
	Evas_Object *btn_done;

	Elm_Genlist_Item_Class itc;

	mp_track_type_e t_type;
	char *type_str;
	int playlist_id;
	bool multiple;

	mp_media_list_h track_list;

	Ecore_Timer *destroy_idler;
}track_list_data_t;

typedef struct
{
	//Elm_Object_Item *it;	// Genlist Item pointer
	Eina_Bool checked;	// Check status
	mp_media_info_h media;
} list_item_data_t;

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
static Evas_Object *_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *content = NULL;
	char *thumbpath = NULL;

	list_item_data_t *it_data = data;
	MP_CHECK(it_data);

	mp_media_info_h media = it_data->media;

	if (!strcmp(part, "elm.icon"))
	{
		content = elm_bg_add(obj);
		elm_bg_load_size_set(content, ICON_SIZE, ICON_SIZE);

		mp_media_info_get_thumbnail_path(media, &thumbpath);

		if(ug_mp_check_image_valid(	evas_object_evas_get(obj), thumbpath))
			elm_bg_file_set(content, thumbpath, NULL);
		else
			elm_bg_file_set(content, DEFAULT_THUMBNAIL, NULL);
	}

	if (elm_genlist_decorate_mode_get(obj) )
	{			// if edit mode
		if (!strcmp(part, "elm.edit.icon.1"))
		{		// swallow checkbox or radio button
			content = elm_check_add(obj);
			elm_check_state_pointer_set(content, &it_data->checked);
 			return content;
		}
	}

	return content;
}

static char *_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	char *text = NULL;

	list_item_data_t *it_data = data;
	MP_CHECK(it_data);

	mp_media_info_h media = it_data->media;

	int ret = 0;
	if (strcmp(part, "elm.text.1") == 0) {
		ret = mp_media_info_get_title(media, &text);
		MP_CHECK_NULL(ret==0);
		return g_strdup(text);
	}else if (strcmp(part, "elm.text.2") == 0) {
		ret = mp_media_info_get_artist(media, &text);
		MP_CHECK_NULL(ret==0);
		return g_strdup(text);
	}/*else if(strcmp(part, "elm.text.3") == 0) {
		int dur;
		char time[TIME_FORMAT_LEN+1] = "";
		ret = mp_media_info_get_duration(media, &dur);
		MP_CHECK_NULL(ret==0);
		_format_duration(time, dur);
		time[TIME_FORMAT_LEN] = '\0';
		return g_strdup(time);
	}*/
	return NULL;
}

static void _gl_del(void *data, Evas_Object *obj)
{
	list_item_data_t *it_data = data;
	IF_FREE(it_data);
}

static Eina_Bool
_destory_idler_cb(void *data)
{
	track_list_data_t *ld  = data;
	MP_CHECK_FALSE(ld);
	ld->destroy_idler = NULL;
	ug_destroy_me(ld->ugd->ug);
	return EINA_FALSE;
}

static unsigned int
_get_select_count(Evas_Object *genlist)
{
	unsigned int count = 0;
	Elm_Object_Item *item;
	list_item_data_t *data = NULL;

	item = mp_list_first_item_get(genlist);
	while(item)
	{
		data = elm_object_item_data_get(item);
		item = mp_list_item_next_get(item);
		if(data && data->checked)
		{
			count++;
		}
	}
	return count;
}

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	track_list_data_t *ld  = data;
	char *path = NULL;

	elm_genlist_item_selected_set(event_info, EINA_FALSE);
	MP_CHECK(ld);
	MP_CHECK(!ld->destroy_idler);

	list_item_data_t *it_data = elm_object_item_data_get(event_info);
	MP_CHECK(it_data);

	if (elm_genlist_decorate_mode_get(obj) )
	{
		it_data->checked = !it_data->checked;
 		elm_genlist_item_fields_update(event_info, "elm.edit.icon.1", ELM_GENLIST_ITEM_FIELD_CONTENT);
 		/*
		if(_get_select_count(obj))
			elm_object_disabled_set(ld->btn_done, false);
		else
			elm_object_disabled_set(ld->btn_done, true);*/
 		return;
	}

	mp_media_info_h media = it_data->media;
	MP_CHECK(media);
	mp_media_info_get_file_path(media, &path);
	DEBUG_TRACE("path: %s", path);

	service_h service = NULL;
	service_create(&service);
	service_add_extra_data(service, "uri", path);
	service_add_extra_data(service, UG_MP_OUTPUT_KEY, path);

	ug_send_result(ld->ugd->ug, service);
	ld->destroy_idler = ecore_timer_add(0.1, _destory_idler_cb, ld);

	service_destroy(service);
}

static void
_layout_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	track_list_data_t *ld  = data;
	MP_CHECK(ld);

	IF_FREE(ld->type_str);

	free(ld);
}

static void  _done_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	char *fmt = ";%s";
	GString *path = NULL;
	track_list_data_t *ld  = data;
	Elm_Object_Item *item;
	MP_CHECK(ld);

	item = mp_list_first_item_get(ld->genlist);
	while(item)
	{
		list_item_data_t *it_data = elm_object_item_data_get(item);
		item = mp_list_item_next_get(item);
		if(it_data && it_data->checked)
		{
			char *tmp = NULL;
			mp_media_info_h media = it_data->media;
			MP_CHECK(media);
			mp_media_info_get_file_path(media, &tmp);
			DEBUG_TRACE("path: %s", tmp);
			if (path == NULL)
				path = g_string_new(tmp);
			else
				g_string_append_printf(path, fmt, tmp);
		}
	}

	MP_CHECK(path);

	service_h service = NULL;
	service_create(&service);
	service_add_extra_data(service, "uri", path->str);
	service_add_extra_data(service, UG_MP_OUTPUT_KEY, path->str);

	ug_send_result(ld->ugd->ug, service);
	ld->destroy_idler = ecore_timer_add(0.1, _destory_idler_cb, ld);

	service_destroy(service);

	g_string_free(path, TRUE);

}

Evas_Object *ug_mp_track_list_create(Evas_Object *parent, struct ug_data *ugd, Elm_Object_Item *navi_it)
{
	startfunc;
	Evas_Object *layout ;
	track_list_data_t *ld = NULL;

	MP_CHECK_NULL(parent);
	MP_CHECK_NULL(ugd);

	layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, "layout", "application", "default");
	MP_CHECK_NULL(layout);

	ld = calloc(1, sizeof(track_list_data_t));
	MP_CHECK_NULL(ld);

	ld->ugd = ugd;
	if(ugd->select_type == UG_MP_SELECT_MULTI)
		ld->multiple = true;

	evas_object_data_set(layout, "list_data", ld);
	evas_object_event_callback_add(layout, EVAS_CALLBACK_FREE, _layout_del_cb, ld);

	ld->itc.func.content_get = _gl_content_get;
	ld->itc.item_style = "2text.1icon.4";
	ld->itc.func.text_get = _gl_text_get;
	ld->itc.func.del = _gl_del;
	ld->itc.decorate_all_item_style = "edit_default";

	if(ugd->select_type == UG_MP_SELECT_MULTI)
	{
		Evas_Object *btn = elm_button_add(ugd->navi_bar);
		elm_object_style_set(btn, "naviframe/toolbar/default");
		elm_object_text_set(btn, GET_SYS_STR("IDS_COM_POP_DONE"));
		evas_object_smart_callback_add(btn, "clicked", _done_cb, ld);
		elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
		//elm_object_disabled_set(btn, true);
		ld->btn_done = btn;
	}
	return layout;
}

int ug_mp_track_list_update(Evas_Object *list)
{
	startfunc;
	Evas_Object *content;

	int count = 0;
	track_list_data_t *ld  = GET_LIST_DATA(list);
	MP_CHECK_VAL(ld, -1);

	if(ld->track_list)
	{
		mp_media_info_list_destroy(ld->track_list);
		ld->track_list = NULL;
	}

	content = elm_layout_content_get(list, "elm.swallow.content");
	evas_object_del(content);

	mp_media_info_list_count(ld->t_type, ld->type_str, NULL, NULL, ld->playlist_id, &count);
	if(count)
	{
		ld->genlist = content = _ug_mp_create_genlist(list);
		mp_media_info_list_create(&ld->track_list, ld->t_type, ld->type_str, NULL, NULL, ld->playlist_id, 0, count);
		int i = 0;
		for(i=0; i<count; i++)
		{
			mp_media_info_h media =  mp_media_info_list_nth_item(ld->track_list, i);
			list_item_data_t *data = calloc(1, sizeof(list_item_data_t));
			data->media = media;

			elm_genlist_item_append(content, &ld->itc, data, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, ld);
		}

		if(ld->multiple)
			elm_genlist_decorate_mode_set(content, true);
	}
	else
		content = ug_mp_widget_no_content_add(list, NO_CONTENT_SONG);

	elm_layout_content_set(list, "elm.swallow.content", content);

	return 0;
}

int ug_mp_track_list_set_data(Evas_Object *list, int track_type, const char *type_str, int playlist_id)
{
	startfunc;
	track_list_data_t *ld  = GET_LIST_DATA(list);
	MP_CHECK_VAL(ld, -1);

	ld->t_type = track_type;
	IF_FREE(ld->type_str);
	ld->type_str = g_strdup(type_str);
	ld->playlist_id = playlist_id;

	return 0;
}


