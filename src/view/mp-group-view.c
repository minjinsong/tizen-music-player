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


#include "mp-library.h"
#include "mp-ta.h"
#include "mp-player-debug.h"
#include "mp-common.h"
#include "music.h"
#include "mp-view-layout.h"
#include "mp-search.h"
#include "mp-menu.h"
#include "mp-edit-view.h"
#include "mp-util.h"
#include "mp-widget.h"
#include "mp-play-view.h"
#include "mp-group-view.h"

static void _mp_group_view_push_item_content(view_data_t * view_data, Evas_Object * content, char *title);
static char * _mp_group_view_album_album_list_label_get(void *data, Evas_Object * obj, const char *part);

static Elm_Genlist_Item_Class *
_mp_group_view_get_track_list_itc(int list_type)
{
	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	MP_CHECK_NULL(itc);

	if (list_type == MP_TRACK_BY_ALBUM || list_type == MP_TRACK_BY_ARTIST_ALBUM) {
		itc->item_style = "2text.2";
		itc->decorate_item_style = "mode/slide4";
	} else if (list_type == MP_TRACK_BY_FOLDER) {
		itc->item_style = "2text.1icon";
		itc->decorate_item_style = "mode/slide4";
	} else {
		itc->item_style = "3text.1icon.1";
		itc->decorate_item_style = "mode/slide4";
	}

	itc->func.text_get = mp_common_track_list_label_get;
	itc->func.content_get = mp_common_track_list_icon_get;

	return itc;
}

static Elm_Genlist_Item_Class *
_mp_group_view_get_artist_album_itc()
{
	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	MP_CHECK_NULL(itc);

	itc->item_style = "2text.1icon";
	itc->decorate_item_style = "mode/slide";

	itc->func.text_get = _mp_group_view_album_album_list_label_get;
	itc->func.content_get = mp_group_view_icon_get;

	return itc;
}

static int
_mp_group_view_get_list_type_by_group_type(int type)
{
	int list_type = 0;
	switch (type)
	{
	case MP_GROUP_BY_ALBUM:
		list_type = MP_TRACK_BY_ALBUM;
		break;
	case MP_GROUP_BY_ARTIST:
		list_type = MP_TRACK_BY_ARTIST;
		break;
	case MP_GROUP_BY_ARTIST_ALBUM:
		list_type = MP_TRACK_BY_ARTIST_ALBUM;
		break;
	case MP_GROUP_BY_GENRE:
		list_type = MP_TRACK_BY_GENRE;
		break;
	case MP_GROUP_BY_YEAR:
		list_type = MP_TRACK_BY_YEAR;
		break;
	case MP_GROUP_BY_COMPOSER:
		list_type = MP_TRACK_BY_COMPOSER;
		break;
	case MP_GROUP_BY_FOLDER:
		list_type = MP_TRACK_BY_FOLDER;
		break;

	default:
		WARN_TRACE("Unhandled type: %d", type);
	}
	return list_type;
}

static Evas_Object *
_mp_group_create_all_song_view_layout(view_data_t * view_data, char *type_str)
{
	int category = MP_LAYOUT_TRACK_LIST;
	Elm_Genlist_Item_Class *itc = NULL;
	mp_genlist_cb_t genlist_cbs;

	MP_CHECK_NULL(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	Evas_Object *layout = mp_view_layout_create(view_data->navibar, view_data, MP_VIEW_MODE_DEFAULT);
	mp_assert(layout);

	memset(&genlist_cbs, 0, sizeof(mp_genlist_cb_t));

	itc = _mp_group_view_get_track_list_itc(MP_TRACK_BY_ARTIST);
	genlist_cbs.selected_cb = mp_common_track_genlist_sel_cb;

	mp_view_layout_set_layout_data(layout,
				       MP_LAYOUT_CATEGORY_TYPE, category,
				       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_BY_ARTIST,
				       MP_LAYOUT_TYPE_STR, type_str,
				       MP_LAYOUT_LIST_CB, &genlist_cbs, MP_LAYOUT_GENLIST_ITEMCLASS, itc, -1);

	mp_view_layout_update(layout);

	return layout;

}

static Evas_Object *
_mp_group_create_detail_view_layout(view_data_t * view_data, char *type_str, char *type_str2, mp_group_type_e type)
{
	mp_track_type_e list_type = 0;
	mp_group_type_e g_type = 0;
	int category = MP_LAYOUT_TRACK_LIST;
	Elm_Genlist_Item_Class *itc = NULL;
	mp_genlist_cb_t genlist_cbs;

	MP_CHECK_NULL(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	Evas_Object *layout = mp_view_layout_create(view_data->navibar, view_data, MP_VIEW_MODE_DEFAULT);
	mp_assert(layout);

	memset(&genlist_cbs, 0, sizeof(mp_genlist_cb_t));

	if(type == MP_GROUP_BY_ARTIST)
	{
		category = MP_LAYOUT_GROUP_LIST;
		g_type = MP_GROUP_BY_ARTIST_ALBUM;
		itc = _mp_group_view_get_artist_album_itc();
		genlist_cbs.selected_cb = mp_group_view_group_list_select_cb;
	}
	else
	{
		list_type = _mp_group_view_get_list_type_by_group_type(type);
		itc = _mp_group_view_get_track_list_itc(list_type);
		genlist_cbs.selected_cb = mp_common_track_genlist_sel_cb;
	}
	mp_view_layout_set_layout_data(layout,
				       MP_LAYOUT_CATEGORY_TYPE, category,
				       MP_LAYOUT_TRACK_LIST_TYPE, list_type,
				       MP_LAYOUT_GROUP_LIST_TYPE, g_type,
				       MP_LAYOUT_TYPE_STR, type_str,
				       MP_LAYOUT_TYPE_STR2, type_str2,
				       MP_LAYOUT_LIST_CB, &genlist_cbs, MP_LAYOUT_GENLIST_ITEMCLASS, itc, -1);

	mp_view_layout_update(layout);

	return layout;

}

static char *
_get_folder_name_by_id(char *folder_id)
{
	mp_retvm_if(!folder_id, NULL, "File path is null...");
	mp_media_list_h media_list = NULL;
	int count;
	char *folder_name = NULL;

	mp_media_info_group_list_count(MP_GROUP_BY_FOLDER, NULL, NULL, &count);
	mp_media_info_group_list_create(&media_list, MP_GROUP_BY_FOLDER, NULL, NULL, 0, count);

	int i = 0;
	for (i = 0; i < count; i++)
	{
		mp_media_info_h info;
		char *id = NULL;
		info = mp_media_info_group_list_nth_item(media_list, i);
		mp_media_info_group_get_folder_id(info, & id);
		if(!g_strcmp0(folder_id, id)){
			mp_media_info_group_get_main_info (info, &folder_name);
			folder_name = g_strdup(folder_name);
			IF_FREE(id);
			break;
		}
		IF_FREE(id);
	}
	mp_media_info_group_list_destroy(media_list);
	return folder_name;
}

bool
mp_group_view_create_by_group_name(Evas_Object * obj, char *group_name, mp_group_type_e type)
{
	startfunc;

	view_data_t *view_data = evas_object_data_get(obj, "view_data");
	MP_CHECK_FALSE(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	Evas_Object *view_layout = _mp_group_create_detail_view_layout(view_data, group_name, NULL, type);

	if(type == MP_GROUP_BY_FOLDER)
	{
		char *folder_name = _get_folder_name_by_id(group_name);
		_mp_group_view_push_item_content(view_data, view_layout, folder_name);
		IF_FREE(folder_name);
	}
	else
		_mp_group_view_push_item_content(view_data, view_layout, group_name);
	endfunc;

	return true;
}


void
mp_group_view_group_list_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	int ret = 0;
	int index = (int)data;
	char *name = NULL;
	char *artist = NULL;
	char *title = NULL;
	char *folder_id = 0;

	Elm_Object_Item *gli = (Elm_Object_Item *) event_info;
	MP_CHECK(gli);
	elm_genlist_item_selected_set(gli, FALSE);

	mp_genlist_item_data_t *gli_data = elm_object_item_data_get(gli);
	MP_CHECK(gli_data);

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_retm_if(!layout_data, "layout_data is NULL !!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	if (layout_data->edit_mode)
	{
		mp_edit_view_genlist_sel_cb(data, obj, event_info);
		return;
	}

	mp_media_info_h item_handle = NULL;
	view_data_t *view_data = evas_object_data_get(layout_data->ad->naviframe, "view_data");
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	if (layout_data->view_mode == MP_VIEW_MODE_SEARCH)
	{
		layout_data->group_type = gli_data->group_type;
		if (gli_data->group_type == MP_GROUP_BY_ARTIST)
		{
			item_handle = mp_media_info_group_list_nth_item(layout_data->artist_handle, index);
			ret = mp_media_info_group_get_main_info(item_handle, &name);
		}
		else if (gli_data->group_type == MP_GROUP_BY_ALBUM)
		{
			item_handle = mp_media_info_group_list_nth_item(layout_data->album_handle, index);
			ret = mp_media_info_group_get_main_info(item_handle, &name);
 		}
		else
		{
			ERROR_TRACE("never should be here...");
			return;
		}

		mp_retm_if(ret != 0, "Fail to get value");
		mp_retm_if(name == NULL, "Fail to get value");

		title = name;
	}
	else
	{
		if(index >=0)
		{
			item_handle = mp_media_info_group_list_nth_item(layout_data->svc_handle, index);
			ret = mp_media_info_group_get_main_info(item_handle, &name);
			if (view_data->view_type == MP_VIEW_TYPE_FOLDER)
				ret = mp_media_info_group_get_folder_id(item_handle, &folder_id);

			ret = mp_media_info_group_get_sub_info(item_handle, &artist);

			mp_retm_if(ret != 0, "Fail to get value");
			mp_retm_if(name == NULL, "Fail to get value");

			if(layout_data->group_type == MP_GROUP_BY_ARTIST_ALBUM)
				title = artist;
			else
				title = name;
		}
		else	//All songs in artist view
		{
			artist = layout_data->type_str;
			title = GET_STR("All songs");
		}
	}

	DEBUG_TRACE("name: %s, type: %d", name, layout_data->group_type);

	Evas_Object *view_layout = NULL;
	if (view_data->view_type == MP_VIEW_TYPE_FOLDER && layout_data->view_mode != MP_VIEW_MODE_SEARCH)
	{
		view_layout = _mp_group_create_detail_view_layout(view_data, folder_id, NULL, layout_data->group_type);
	}
	else
	{
		if(index >=0)
			view_layout = _mp_group_create_detail_view_layout(view_data, name, artist, layout_data->group_type);
		else
			view_layout = _mp_group_create_all_song_view_layout(view_data, artist);
	}

	mp_util_reset_genlist_mode_item(layout_data->genlist);

	_mp_group_view_push_item_content(view_data, view_layout, title);

}

static void
_mp_group_view_set_title(view_data_t * view_data)
{
	char *title = NULL;

	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_view_manager_set_title_and_buttons(view_data, NULL, view_data);

}

static void
_mp_group_view_push_item_content(view_data_t * view_data, Evas_Object * content, char *title)
{
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_view_manager_push_view_content(view_data, content, MP_VIEW_CONTENT_LIST);
	mp_view_manager_set_title_and_buttons(view_data, title, view_data);
}

static void
_mp_group_view_playall_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();

	struct appdata *ad = NULL;
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");

	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	ad = layout_data->ad;

	mp_media_info_h handle = data;
	MP_CHECK(handle);

	int count = 0;
	mp_track_type_e track_type = 0;
	char *type_str = NULL;
	int ret = 0;

	track_type = _mp_group_view_get_list_type_by_group_type(layout_data->group_type);
	if(track_type == MP_TRACK_BY_FOLDER)
	{
		ret = mp_media_info_group_get_folder_id(handle, &type_str);
	}
	else
	{
		ret = mp_media_info_group_get_main_info(handle, &type_str);
	}
	MP_CHECK(type_str);

	/* get playlist data by name */
	mp_media_list_h svc_handle = NULL;

	mp_media_info_list_count(track_type, type_str, NULL, NULL, 0, &count);
	mp_media_info_list_create(&svc_handle,
		track_type, type_str, NULL, NULL, 0, 0, count);

	mp_playlist_mgr_clear(ad->playlist_mgr);
	mp_util_append_media_list_item_to_playlist(ad->playlist_mgr, svc_handle, count, 0, NULL);

	mp_play_view_load_and_play(ad, NULL, false);

	/* set the flag for update playlist view when back from playview */
	if (ad->playing_view != NULL)
	{
		ad->playing_view->b_play_all = true;
	}

	if (svc_handle)
	{
		mp_media_info_list_destroy(svc_handle);
	}

	endfunc;
}


Evas_Object *
mp_group_view_icon_get(void *data, Evas_Object * obj, const char *part)
{
	Evas_Object *icon = NULL;

	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h svc_item = (item->handle);

	mp_retv_if(svc_item == NULL, NULL);
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	MP_CHECK_LAYOUT_DATA(layout_data);

	char *thumb_name = NULL;

	mp_media_info_group_get_thumbnail_path(svc_item, &thumb_name);

	const char *slide_part_play_all = "";
	if(layout_data->group_type == MP_GROUP_BY_FOLDER)
		slide_part_play_all = "elm.slide.swallow.2";

	if (!strcmp(part, "elm.icon"))
	{
		icon = mp_util_create_thumb_icon(obj, thumb_name, MP_LIST_ICON_SIZE, MP_LIST_ICON_SIZE);
	}

	Evas_Object *button = NULL;
	if (!strcmp(part, "elm.slide.swallow.1"))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/multiline");
		elm_object_text_set(button, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_MUSIC_BODY_ADD_TO_PLAYLIST");
		evas_object_smart_callback_add(button, "clicked", mp_menu_add_to_playlist_cb, item->handle);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!strcmp(part, slide_part_play_all))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/multiline");
		elm_object_text_set(button, GET_STR("Play all"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "Play all");
		evas_object_smart_callback_add(button, "clicked", _mp_group_view_playall_button_cb, item->handle);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!g_strcmp0(part, "elm.icon.storage"))
	{
		char *folder = NULL;
		icon = NULL;
		int ret = mp_media_info_group_get_sub_info(svc_item, &folder);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (folder) {
			const char *icon_path = NULL;
			if (g_strstr_len(folder, strlen(MP_PHONE_ROOT_PATH), MP_PHONE_ROOT_PATH))
				icon_path = MP_ICON_STORAGE_PHONE;
			else if (g_strstr_len(folder, strlen(MP_MMC_ROOT_PATH), MP_MMC_ROOT_PATH))
				icon_path = MP_ICON_STORAGE_MEMORY;
			else
				icon_path = MP_ICON_STORAGE_EXTERNAL;

			icon = elm_icon_add(obj);
			MP_CHECK_NULL(icon);
			elm_image_file_set(icon, IMAGE_EDJ_NAME, icon_path);
			evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
			return icon;
		}
	}

	Evas_Object *check = NULL;

	if (elm_genlist_decorate_mode_get(obj) || layout_data->ad->b_add_tracks)
	{			// if edit mode
		if (!strcmp(part, "elm.edit.icon.1"))
		{		// swallow checkbox or radio button
			check = elm_check_add(obj);
			elm_check_state_pointer_set(check, &item->checked);
			evas_object_smart_callback_add(check, "changed", mp_common_item_check_changed_cb, item);
			return check;
		}
	}

	return icon;
}

static char *
_mp_group_view_album_album_list_label_get(void *data, Evas_Object * obj, const char *part)
{
	char *name = NULL;
	int ret = 0;

	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h svc_item = (item->handle);

	mp_retv_if(svc_item == NULL, NULL);

	if (!strcmp(part, "elm.text.1") || !strcmp(part, "elm.slide.text.1"))
	{
		ret = mp_media_info_group_get_main_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (!name || !strlen(name))
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");

		if (!strcmp(part, "elm.text.1"))
			return elm_entry_utf8_to_markup(name);
		else
			return g_strdup(name);

	}
	else if (!strcmp(part, "elm.text.2"))
	{
		int count = -1;
		ret = mp_media_info_group_get_main_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");

		ret = mp_media_info_list_count(MP_TRACK_BY_ALBUM, name, NULL, NULL, 0, &count);
		mp_retvm_if(ret != 0, NULL, "Fail to get count");
		mp_retvm_if(count < 0, NULL, "Fail to get count");
		return g_strdup_printf("(%d)", count);
	}

	DEBUG_TRACE("Unusing part: %s", part);
	return NULL;
}


char *
mp_group_view_album_list_label_get(void *data, Evas_Object * obj, const char *part)
{
	char *name = NULL;
	int ret = 0;

	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h svc_item = (item->handle);

	mp_retv_if(svc_item == NULL, NULL);

	if (!strcmp(part, "elm.text.1") || !strcmp(part, "elm.slide.text.1"))
	{
		ret = mp_media_info_group_get_main_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (!name || !strlen(name))
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");

		if (!strcmp(part, "elm.text.1"))
			return elm_entry_utf8_to_markup(name);
		else
			return g_strdup(name);

	}
	else if (!strcmp(part, "elm.text.2"))
	{
		ret = mp_media_info_group_get_sub_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (!name || !strlen(name))
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");
		return g_strdup(name);
	}
	else if (!strcmp(part, "elm.text.3"))
	{
		int count = -1;
		ret = mp_media_info_group_get_main_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");

		ret = mp_media_info_list_count(MP_TRACK_BY_ALBUM, name, NULL, NULL, 0, &count);
		mp_retvm_if(ret != 0, NULL, "Fail to get count");
		mp_retvm_if(count < 0, NULL, "Fail to get count");
		return g_strdup_printf("(%d)", count);
	}

	DEBUG_TRACE("Unusing part: %s", part);
	return NULL;
}

static char *
_mp_group_view_folder_list_label_get(void *data, Evas_Object * obj, const char *part)
{
	char *name = NULL;
	int ret = 0;
	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h svc_item = (item->handle);
	mp_retv_if(svc_item == NULL, NULL);

	if (!strcmp(part, "elm.text.1") || !strcmp(part, "elm.slide.text.1"))
	{
		ret = mp_media_info_group_get_main_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (!name || !strlen(name))
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");
		return strdup(name);
	}
	else if (!strcmp(part, "elm.text.2"))
	{
		ret = mp_media_info_group_get_sub_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (!name || !strlen(name))
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");
		return mp_util_shorten_path(name);
	}
	else if (!strcmp(part, "elm.text.3"))
	{
		int count = -1;
		ret = mp_media_info_group_get_folder_id(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		ret = mp_media_info_list_count(MP_TRACK_BY_FOLDER, name, NULL, NULL, 0, &count);
		mp_retvm_if(ret != 0, NULL, "Fail to get count");
		mp_retvm_if(count < 0, NULL, "Fail to get count");
		return g_strdup_printf("(%d)", count);
	}

	DEBUG_TRACE("Unusing part: %s", part);
	return NULL;
}

char *
mp_group_view_list_label_get(void *data, Evas_Object * obj, const char *part)
{
	char *name = NULL;
	int ret = 0;

	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h svc_item = (item->handle);
	mp_retv_if(svc_item == NULL, NULL);
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	MP_CHECK_NULL(layout_data);

	if (!strcmp(part, "elm.text.1") || !strcmp(part, "elm.slide.text.1"))
	{
		ret = mp_media_info_group_get_main_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (!name || !strlen(name))
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");
		if(layout_data->filter_str)
		{
			char *markup_name = NULL;
			bool res = false;
			markup_name = (char *)mp_util_search_markup_keyword(name, layout_data->filter_str, &res);
			if(res)
				return g_strdup(markup_name);
		} else if (!strcmp(part, "elm.text.1"))
			return elm_entry_utf8_to_markup(name);
		else
			return g_strdup(name);
	}
	else if (!strcmp(part, "elm.text.2"))
	{
		int count = -1;
		ret = mp_media_info_group_get_main_info(svc_item, &name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");

		int list_type = _mp_group_view_get_list_type_by_group_type(item->group_type);
		DEBUG_TRACE("name: %s, list_type: %d", name, list_type);
		ret = mp_media_info_list_count(list_type, name, NULL, NULL, 0, &count);
		mp_retvm_if(ret != 0, NULL, "Fail to get count");
		mp_retvm_if(count < 0, NULL, "Fail to get count");
		return g_strdup_printf("(%d)", count);
	}

	return NULL;
}

static mp_genlist_cb_t g_group_list_cbs = {
	.selected_cb = mp_group_view_group_list_select_cb,
};

static Elm_Genlist_Item_Class*
_mp_group_view_get_group_list_itec(mp_view_type_t view_type)
{
	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	MP_CHECK_NULL(itc);

	const char *group_slide_style = "mode/slide";
	const char *folder_slide_style = "mode/slide2";

	switch (view_type) {
	case MP_VIEW_TYPE_ALBUM:
		itc->item_style = "3text.1icon.1";
		itc->decorate_item_style = group_slide_style;
		itc->func.text_get = mp_group_view_album_list_label_get;
		itc->func.content_get = mp_group_view_icon_get;
		break;

	case MP_VIEW_TYPE_ARTIST:
	case MP_VIEW_TYPE_GENRE:
	case MP_VIEW_TYPE_COMPOSER:
	case MP_VIEW_TYPE_YEAR:
		itc->item_style = "2text.1icon";
		itc->decorate_item_style = group_slide_style;
		itc->func.text_get = mp_group_view_list_label_get;
		itc->func.content_get = mp_group_view_icon_get;
		break;

	case MP_VIEW_TYPE_FOLDER:
		itc->item_style = "3text.1icon.1";
		itc->decorate_item_style = folder_slide_style;
		itc->func.text_get = _mp_group_view_folder_list_label_get;
		itc->func.content_get = mp_group_view_icon_get;
		break;

	default:
		elm_genlist_item_class_free(itc);
		itc = NULL;
	}

	return itc;
}


Evas_Object *
mp_group_view_create(struct appdata *ad, mp_view_type_t view_type)
{
	DEBUG_TRACE("view_type: %d", view_type);
	int list_type;

	Elm_Genlist_Item_Class *itc;

	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	view_data->view_type = view_type;
	Evas_Object *view_layout = mp_view_layout_create(ad->tabbar, view_data, MP_VIEW_MODE_DEFAULT);

	mp_layout_data_t *layout_data = NULL;
	layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK_NULL(layout_data);
	evas_object_data_set(ad->controlbar_layout, "layout_data", layout_data);

	if (view_type == MP_VIEW_TYPE_ALBUM)
	{
		DEBUG_TRACE("album view");
		list_type = MP_GROUP_BY_ALBUM;
	}
	else if (view_type == MP_VIEW_TYPE_ARTIST)
	{
		DEBUG_TRACE("artist view");
		list_type = MP_GROUP_BY_ARTIST;
	}
	else if (view_type == MP_VIEW_TYPE_GENRE)
	{
		DEBUG_TRACE("genre view");
		list_type = MP_GROUP_BY_GENRE;
	}
	else if (view_type == MP_VIEW_TYPE_COMPOSER)
	{
		DEBUG_TRACE("composer view");
		list_type = MP_GROUP_BY_COMPOSER;
	}
	else if (view_type == MP_VIEW_TYPE_YEAR)
	{
		DEBUG_TRACE("year view");
		list_type = MP_GROUP_BY_YEAR;
	}
	else if (view_type == MP_VIEW_TYPE_FOLDER)
	{
		DEBUG_TRACE("folder view");
		list_type = MP_GROUP_BY_FOLDER;
	}
	else
	{
		ERROR_TRACE("unexpected type: %d", view_type);
		return NULL;
	}

	itc = _mp_group_view_get_group_list_itec(view_type);

	mp_view_layout_set_layout_data(view_layout,
				       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_GROUP_LIST,
				       MP_LAYOUT_GROUP_LIST_TYPE, list_type,
				       MP_LAYOUT_LIST_CB, &g_group_list_cbs, MP_LAYOUT_GENLIST_ITEMCLASS, itc, -1);
	_mp_group_view_set_title(view_data);

	return view_layout;
}

void
mp_group_view_destroy(Evas_Object * group_view)
{
	DEBUG_TRACE("");
	evas_object_del(group_view);
}

void
mp_group_view_refresh(Evas_Object * group_view)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) evas_object_data_get(group_view, "view_data");
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_view_manager_update_list_contents(view_data, TRUE);
}
