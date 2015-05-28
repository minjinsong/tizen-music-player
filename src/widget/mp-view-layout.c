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


#include "mp-view-layout.h"
#include "mp-player-debug.h"
#include "mp-ta.h"
#include "mp-library.h"
#include "mp-widget.h"
#include "mp-util.h"
#include "mp-common.h"
#include "mp-search.h"
#include "mp-play-view.h"
#include "mp-playlist-mgr.h"
#include "mp-view-manager.h"
#include "mp-playlist-view.h"
#include "mp-player-control.h"
#include "mp-group-view.h"
#include "mp-player-mgr.h"
#include "mp-popup.h"
#include "mp-play.h"


#define MP_INIT_ITEM_LOAD_COUNT 9
#define MP_SYS_PLAYLIST_COUNT 4
#define MP_GENLIST_DEFALT_BLOCK_SIZE 81
#define MP_MAX_TEXT_PRE_FORMAT_LEN 256
#define MP_MAX_ARTIST_NAME_WIDTH 320
#define MP_LABEL_SLIDE_DURATION 5
#define MP_ALBUM_SONGS_MAX_LEN 10

/* get layout_data of landscape square view */

static void _mp_view_layout_reorder(void *data, Evas_Object * obj, void *event_info);
static void _mp_view_layout_load_search_list_item(Evas_Object * view_layout);
static void _mp_view_layout_load_list_item(Evas_Object * view_layout);
static void _mp_view_layout_load_search_item(Evas_Object * view_layout);
static Evas_Object *_mp_view_layout_create_fastscroll_index(Evas_Object * parent, mp_layout_data_t * layout_data);
static void _mp_view_layout_gl_del(void *data, Evas_Object * obj);
static void _mp_view_layout_update_icon(Evas_Object * view_layout);

static Eina_Bool
_mp_view_layout_update_list_idler_cb(void *data)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK_LAYOUT_DATA(layout_data);

	_mp_view_layout_load_search_list_item(layout_data->layout);

	DEBUG_TRACE("layout_data->filter_str: %s", layout_data->filter_str);
	if (!layout_data->filter_str || !strlen(layout_data->filter_str))
		mp_search_show_imf_pannel(layout_data->search_bar);

	layout_data->search_idler_handle = NULL;
	return EINA_FALSE;
}

void
mp_view_layout_search_changed_cb(void *data, Evas_Object * obj, void *event_info)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK_LAYOUT_DATA(layout_data);
	const char *search_str = NULL;

	search_str = mp_search_text_get(layout_data->search_bar);

	DEBUG_TRACE("search_str: %s", search_str);
	if (search_str)
	{
		if (layout_data->filter_str)
			free(layout_data->filter_str);
		layout_data->filter_str = strdup(search_str);
		if (layout_data->search_idler_handle)
			ecore_idler_del(layout_data->search_idler_handle);
		layout_data->search_idler_handle = ecore_idler_add(_mp_view_layout_update_list_idler_cb, data);
	}
}

static Eina_Bool
_mp_view_layout_reorder_item(void *data)
{
	DEBUG_TRACE("");
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK_FALSE(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	Elm_Object_Item *list_item = NULL;
	int count = 0;
	int err = -1;
	int old_order;
	int new_order = 1;
	int uid;

	mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, layout_data->filter_str, layout_data->playlist_id, &count);
	if (count > 0)
	{
		list_item = elm_genlist_first_item_get(layout_data->genlist);

		if (list_item != NULL)
		{
			do
			{
				mp_genlist_item_data_t *gl_item =
					(mp_genlist_item_data_t *) elm_object_item_data_get(list_item);
				MP_CHECK_FALSE(gl_item);
				mp_media_info_h item = (gl_item->handle);
				MP_CHECK_FALSE(item);
				if (item != NULL)
				{
					if (gl_item->index+1 == new_order)
					{
						new_order++;
						continue;
					}
					err = mp_media_info_get_playlist_member_id(item, &uid);
					DEBUG_TRACE("uid: %d, old_order: %d", uid, gl_item->index+1);
					if (err != 0)
					{
						ERROR_TRACE("Error in mp_media_info_playlist_get_play_order (%d)\n", err);
						break;
					}
					err = mp_media_info_playlist_set_play_order(layout_data->playlist_handle, uid, new_order++);
					if (err != 0)
					{
						ERROR_TRACE("Error in mp_media_info_playlist_set_play_order (%d)\n", err);
						break;
					}
				}
			}
			while ((list_item = elm_genlist_item_next_get(list_item)) != NULL);

			mp_media_info_playlist_update_db(layout_data->playlist_handle);
		}
	}
	return EINA_FALSE;
}

static void
_mp_view_layout_reorder(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	mp_layout_data_t *layout_data = data;
	MP_CHECK_LAYOUT_DATA(layout_data);

	layout_data->reordered = true;

}

static void
_mp_view_layout_genlist_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	if (layout_data)
	{
		DEBUG_TRACE("category: %d, track_type: %d, group_type: %d", layout_data->category,
			    layout_data->track_type, layout_data->group_type);
		MP_CHECK_LAYOUT_DATA(layout_data);

		mp_ecore_timer_del(layout_data->progress_timer);
		mp_ecore_idler_del(layout_data->load_item_idler);
		mp_ecore_idler_del(layout_data->search_idler_handle);
		mp_ecore_idler_del(layout_data->block_size_idler);

		SAFE_FREE(layout_data->type_str);
		SAFE_FREE(layout_data->type_str2);
		SAFE_FREE(layout_data->filter_str);
		if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
			mp_media_info_group_list_destroy(layout_data->svc_handle);
		else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
			mp_media_info_group_list_destroy(layout_data->svc_handle);

		else
			mp_media_info_list_destroy(layout_data->svc_handle);

		if (layout_data->track_handle)
			mp_media_info_list_destroy(layout_data->track_handle);
		if (layout_data->artist_handle)
			mp_media_info_group_list_destroy(layout_data->artist_handle);
		if (layout_data->album_handle)
			mp_media_info_group_list_destroy(layout_data->album_handle);

		SAFE_FREE(layout_data->old_name);
		IF_FREE(layout_data->fast_scrooll_index);
		IF_FREE(layout_data->navibar_title);

		if (layout_data->itc) {
			elm_genlist_item_class_free(layout_data->itc);
			layout_data->itc = NULL;
		}

		if (layout_data->edit_thread) {
			ecore_thread_cancel(layout_data->edit_thread);
			layout_data->edit_thread = NULL;
		}

		if(layout_data->playlists)
			mp_media_info_group_list_destroy(layout_data->playlists);

		free(layout_data);
	}
	endfunc;
}

static void
_mp_view_layout_append_auto_playlists(mp_layout_data_t * layout_data)
{
	int i;
	mp_media_list_h plst;
	int playlist_state = 0;

	MP_CHECK(layout_data);

	if (layout_data->default_playlists)
		mp_media_info_group_list_destroy(layout_data->default_playlists);

	layout_data->default_playlist_count = 0;

	mp_setting_playlist_get_state(&playlist_state);

	mp_media_info_group_list_create(&plst, MP_GROUP_BY_SYS_PLAYLIST, NULL, NULL, 0, 0);
	layout_data->default_playlists = plst;

	for (i = 0; i < MP_SYS_PLAYLIST_COUNT; i++)
	{
		int enable = playlist_state&(1<<i);
		DEBUG_TRACE("index: %d, state: %d",i, enable);
		if(!enable)
		{
			continue;
		}

		mp_media_info_h item;
		item = mp_media_info_group_list_nth_item(plst, i);

		mp_genlist_item_data_t *item_data;
		item_data = calloc(1, sizeof(mp_genlist_item_data_t));
		MP_CHECK(item_data);
		item_data->handle = item;
		item_data->unregister_lang_mgr = true;

		// func.del shouldn't be used in other place....
		layout_data->auto_playlist_item_class.func.del = _mp_view_layout_gl_del;

		item_data->it = elm_genlist_item_append(layout_data->genlist, &(layout_data->auto_playlist_item_class),
							item_data, NULL,
							ELM_GENLIST_ITEM_NONE, layout_data->cb_func.auto_playlist_cb,
							(void *)i);
		mp_language_mgr_register_genlist_item(item_data->it);
		layout_data->default_playlist_count++;
		if (layout_data->edit_mode)
			elm_object_item_disabled_set(item_data->it, EINA_TRUE);
	}
}

static Evas_Object *
_mp_view_layout_sentinel_add(mp_layout_data_t * layout_data)
{
	DEBUG_TRACE_FUNC();
	Evas_Object *no_contents = NULL;

	no_contents = elm_layout_add(layout_data->box);
	evas_object_size_hint_weight_set(no_contents, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(no_contents, EVAS_HINT_FILL, EVAS_HINT_FILL);


	if (layout_data->view_mode == MP_VIEW_MODE_SEARCH)
	{
		elm_layout_file_set(no_contents, EDJ_NAME, "no_result");
		edje_object_part_text_set(_EDJ(no_contents), "elm.text", GET_SYS_STR("IDS_COM_BODY_NO_SEARCH_RESULTS"));
		mp_language_mgr_register_object(no_contents, OBJ_TYPE_EDJE_OBJECT, "elm.text", "IDS_COM_BODY_NO_SEARCH_RESULTS");
	}
	else
	{

		if (layout_data->ad->screen_mode == MP_SCREEN_MODE_PORTRAIT)
			elm_layout_file_set(no_contents, EDJ_NAME, "no_content");
		else {
				elm_layout_file_set(no_contents, EDJ_NAME, "no_content_landscape");
		}

		edje_object_part_text_set(_EDJ(no_contents), "elm.text", GET_SYS_STR("IDS_COM_BODY_NO_CONTENTS"));
		mp_language_mgr_register_object(no_contents, OBJ_TYPE_EDJE_OBJECT, "elm.text", "IDS_COM_BODY_NO_CONTENTS");
	}

	if (layout_data->genlist)
	{
		elm_box_unpack(layout_data->box, layout_data->genlist);
		evas_object_hide(layout_data->genlist);
	}

	elm_box_pack_end(layout_data->box, no_contents);
	evas_object_show(no_contents);

	return no_contents;
}

static int
_mp_view_layout_set_sentinel(mp_layout_data_t * layout_data, int count)
{
	if (0 >= count
	    && (layout_data->category != MP_LAYOUT_PLAYLIST_LIST
		|| (layout_data->filter_str && strlen(layout_data->filter_str))))
	{
		ERROR_TRACE("no tracks");
		if (!layout_data->sentinel)
			layout_data->sentinel = _mp_view_layout_sentinel_add(layout_data);
		return -1;
	}

	if (layout_data->sentinel)
	{
		elm_box_unpack(layout_data->box, layout_data->sentinel);
		evas_object_del(layout_data->sentinel);
		layout_data->sentinel = NULL;
		elm_box_pack_end(layout_data->box, layout_data->genlist);
		evas_object_show(layout_data->genlist);
	}

	return 0;
}

static Eina_Bool
_mp_view_layout_set_block_count_idle_cb(void *data)
{
	mp_layout_data_t * layout_data = (mp_layout_data_t *)data;
	MP_CHECK_FALSE(layout_data);
	elm_genlist_block_count_set(layout_data->genlist, MP_GENLIST_DEFALT_BLOCK_SIZE);
	layout_data->block_size_idler = NULL;
	return FALSE;
}

char *
_mp_view_layout_gl_label_get_title(void *data, Evas_Object * obj, const char *part)
{
	mp_genlist_item_data_t *item_data = data;
	char *text = NULL;

	MP_CHECK_NULL(item_data);

	if (!strcmp(part, "elm.text"))
	{
		if(item_data->group_title_text_ID && strstr(item_data->group_title_text_ID, "IDS_COM"))
			text = GET_SYS_STR(item_data->group_title_text_ID);
		else
			text = GET_STR(item_data->group_title_text_ID);

		return g_strdup(text);
	}
	return NULL;
}

static void
_mp_view_layout_gl_del(void *data, Evas_Object * obj)
{
	mp_genlist_item_data_t *item_data = (mp_genlist_item_data_t *) data;
	if(item_data->unregister_lang_mgr)
		mp_language_mgr_unregister_genlist_item(item_data->it);
	IF_FREE(item_data);
}

static void
_mp_view_layout_group_gl_del(void *data, Evas_Object * obj)
{
	mp_genlist_item_data_t *item_data = data;
	MP_CHECK(item_data);
	mp_language_mgr_unregister_genlist_item(item_data->it);
	free(item_data);
}

static void
_mp_view_layout_append_group_title(mp_layout_data_t * layout_data, char *text_ID)
{
	static Elm_Genlist_Item_Class itc;
	mp_genlist_item_data_t *item_data = NULL;

	itc.version = ELM_GENGRID_ITEM_CLASS_VERSION;
	itc.refcount = 0;
	itc.delete_me = EINA_FALSE;
	itc.item_style = "music_player/grouptitle";
	itc.func.text_get = _mp_view_layout_gl_label_get_title;
	itc.func.del = _mp_view_layout_group_gl_del;

	item_data = calloc(1, sizeof(mp_genlist_item_data_t));
	item_data->group_title_text_ID = text_ID;
	item_data->item_type = MP_GENLIST_ITEM_TYPE_GROUP_TITLE;

	item_data->it = layout_data->search_group_git =
		elm_genlist_item_append(layout_data->genlist, &itc, item_data, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);

	mp_language_mgr_register_genlist_item(item_data->it);

	MP_CHECK(layout_data->search_group_git);

}

static Evas_Object *
_mp_view_layout_get_label_slide(Evas_Object *parent, char *name, int font_size, char *color, int max_size)
{
	Evas_Object *label = NULL;
	char *label_str = NULL;
	char *pre_format = NULL;
	pre_format = (char *)g_malloc(MP_MAX_TEXT_PRE_FORMAT_LEN);
	mp_retvm_if(!pre_format, NULL, "Fail to allocate memory");
	memset(pre_format, 0, MP_MAX_TEXT_PRE_FORMAT_LEN);
	const char *last_format = "</color></font_size>";
	snprintf(pre_format, MP_MAX_TEXT_PRE_FORMAT_LEN, "<font_size=%d><color=#%s>", font_size, color);
	int str_length = strlen(pre_format) + strlen(name) + strlen(last_format) + 1;
	label_str = (char *)g_malloc(str_length);
	if (!label_str) {
		DEBUG_TRACE("Fail to allocate memory");
		g_free(pre_format);
		return NULL;
	}
	memset(label_str, 0, str_length);
	snprintf(label_str, str_length, "%s%s%s", pre_format, name, last_format);
	label = elm_label_add(parent);
	elm_object_style_set(label, "slide_bounce");
	elm_object_text_set(label, label_str);
	elm_label_wrap_width_set(label, MP_MAX_ARTIST_NAME_WIDTH);
	if (strlen(name) > max_size) {
		elm_label_slide_duration_set(label, (strlen(name) / max_size) * MP_LABEL_SLIDE_DURATION);
		elm_label_slide_set(label, EINA_TRUE);
	}
	g_free(pre_format);
	g_free(label_str);
	return label;
}

static char *
_mp_view_layout_album_list_label_get(void *data, Evas_Object * obj, const char *part)
{
	MP_CHECK_NULL(data);
	char *name = NULL;
	int ret = 0;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	mp_media_info_h svc_item = mp_media_info_list_nth_item(layout_data->svc_handle, 0);
	MP_CHECK_NULL(svc_item);

	if (!g_strcmp0(part, "elm.text.1")) {
		ret = mp_media_info_get_album(svc_item, &name);
		if (!name || !strlen(name))
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");
		return strdup(name);

	} else if (!g_strcmp0(part, "elm.text.3")) {
		return g_strdup_printf("%d %s", layout_data->item_count, GET_STR("IDS_MUSIC_HEADER_SONGS"));
	}

	DEBUG_TRACE("Unusing part: %s", part);
	return NULL;
}

static char *
_mp_view_layout_all_songs_label_get(void *data, Evas_Object * obj, const char *part)
{
	MP_CHECK_NULL(data);
	char *name = NULL;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	mp_media_info_h svc_item = mp_media_info_list_nth_item(layout_data->svc_handle, 0);
	MP_CHECK_NULL(svc_item);

	if (!g_strcmp0(part, "elm.text.1")) {
		name = GET_SYS_STR("All songs");
		return strdup(name);

	} else if (!g_strcmp0(part, "elm.text.3")) {
		int count;
		mp_media_info_list_count(MP_TRACK_BY_ARTIST, layout_data->type_str, NULL, NULL, 0, &count);
		return g_strdup_printf("(%d)", count);
	}

	DEBUG_TRACE("Unusing part: %s", part);
	return NULL;
}

static Evas_Object *
_mp_view_layout_album_list_icon_get(void *data, Evas_Object * obj, const char *part)
{
	startfunc;
	MP_CHECK_NULL(data);
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	mp_media_info_h svc_item = mp_media_info_list_nth_item(layout_data->svc_handle, 0);
	MP_CHECK_NULL(svc_item);

	Evas_Object *icon = NULL;
	int ret = 0;
	if (!g_strcmp0(part, "elm.icon")) {
		char *thumb_name = NULL;
		ret = mp_media_info_get_thumbnail_path(svc_item, &thumb_name);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		icon = mp_util_create_thumb_icon(obj, thumb_name, MP_ALBUM_LIST_ICON_SIZE, MP_ALBUM_LIST_ICON_SIZE);
	} else if (!g_strcmp0(part, "elm.text.swallow")) {
		char *name = NULL;
		mp_media_info_get_artist(svc_item, &name);
		if(!name)
			name = GET_SYS_STR("IDS_COM_BODY_UNKNOWN");
		icon = _mp_view_layout_get_label_slide(obj, name, 28, "5A6368", 42);
	}
	return icon;
}

static void
_mp_view_layout_append_album_group_title(mp_layout_data_t * layout_data)
{
	startfunc;
	MP_CHECK(layout_data);
	static Elm_Genlist_Item_Class album_group_itc = {
		.version = ELM_GENGRID_ITEM_CLASS_VERSION,
		.refcount = 0,
		.delete_me = EINA_FALSE,
		.item_style = "music_player/album_title",
		.func.text_get = _mp_view_layout_album_list_label_get,
		.func.content_get = _mp_view_layout_album_list_icon_get,
	};
	layout_data->album_group =
			elm_genlist_item_append(layout_data->genlist, &album_group_itc, layout_data,
						NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	MP_CHECK(layout_data->album_group);
	elm_genlist_item_select_mode_set(layout_data->album_group, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	endfunc;
}

static void
_mp_view_layout_append_all_song(mp_layout_data_t * layout_data)
{
	startfunc;
	MP_CHECK(layout_data);
	static Elm_Genlist_Item_Class album_group_itc = {
		.version = ELM_GENGRID_ITEM_CLASS_VERSION,
		.refcount = 0,
		.delete_me = EINA_FALSE,
		.item_style = "2text",
		.func.text_get = _mp_view_layout_all_songs_label_get,
	};
	layout_data->album_group =
			elm_genlist_item_append(layout_data->genlist, &album_group_itc, layout_data,
						NULL, ELM_GENLIST_ITEM_GROUP, mp_group_view_group_list_select_cb, (const void *)-1);
	MP_CHECK(layout_data->album_group);
	endfunc;
}

static void
_mp_view_layout_load_search_list_item(Evas_Object * view_layout)
{

	DEBUG_TRACE("");

	static Elm_Genlist_Item_Class itc;

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_retm_if(!layout_data, "layout_data is null");
	MP_CHECK_LAYOUT_DATA(layout_data);

	if (layout_data->filter_str && strlen(layout_data->filter_str))
	{
		_mp_view_layout_load_search_item(view_layout);
		edje_object_signal_emit(_EDJ(layout_data->layout), "hide.screen", "music_app");
	}
	else
	{
		memset(&itc, 0, sizeof(Elm_Genlist_Item_Class));
		itc.version = ELM_GENGRID_ITEM_CLASS_VERSION;
		itc.refcount = 0;
		itc.delete_me = EINA_FALSE;
		itc.item_style = "3text.1icon.1";
		itc.func.text_get = mp_common_track_list_label_get;
		itc.func.content_get = mp_common_track_list_icon_get;

		mp_genlist_cb_t genlist_cbs;
		memset(&genlist_cbs, 0, sizeof(mp_genlist_cb_t));
		genlist_cbs.selected_cb = mp_common_track_genlist_sel_cb;

		mp_view_layout_set_layout_data(view_layout,
					       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_TRACK_LIST,
					       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_ALL,
					       MP_LAYOUT_LIST_CB, &genlist_cbs, MP_LAYOUT_GENLIST_ITEMCLASS, &itc, -1);

		_mp_view_layout_load_list_item(view_layout);

		edje_object_signal_emit(_EDJ(layout_data->layout), "show.screen", "music_app");
	}
}

static void
_mp_view_layout_index_item_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *gl_item = data;
	MP_CHECK(gl_item);

	elm_genlist_item_bring_in(gl_item, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
}

static Eina_Bool
_mp_view_layout_load_item_idler_cb(void *data)
{
	startfunc;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	int index;
	int ret = -1;
	genlist_cb_t sel_cb;
	Elm_Genlist_Item_Class *itc;

	if (!layout_data)
	{
		WARN_TRACE("layout_data is null...");
		return EINA_FALSE;
	}

	itc = layout_data->itc;
	sel_cb = layout_data->cb_func.selected_cb;

	for (index = MP_INIT_ITEM_LOAD_COUNT; index < layout_data->item_count; index++)
	{
		mp_media_info_h item = NULL;
		Elm_Object_Item *list_item = NULL;
		char *title = NULL;

		if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
		{
			item = mp_media_info_group_list_nth_item(layout_data->svc_handle, index);
			if (!item)
			{
				DEBUG_TRACE("Fail to mp_media_info_group_list_nth_item, index[%d]", index);
				goto END;
			}
			mp_media_info_group_get_main_info(item, &title);
		}
		else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
		{
			item = mp_media_info_group_list_nth_item(layout_data->svc_handle, index);
			if (item == NULL)
			{
				DEBUG_TRACE("Fail to mp_media_info_group_list_nth_item, index[%d]", index);
				goto END;
			}
			ret = mp_media_info_group_get_main_info(item, &title);
		}

		else
		{
			item = mp_media_info_list_nth_item(layout_data->svc_handle, index);
			if (!item)
			{
				DEBUG_TRACE("Fail to mp_media_info_list_nth_item, ret[%d], index[%d]", ret, index);
				goto END;
			}
			mp_media_info_get_title(item, &title);
		}
		bool make_group_title = FALSE;

		if (layout_data->fast_scrooll_index == NULL)
		{
			make_group_title = TRUE;
			layout_data->fast_scrooll_index = mp_util_get_utf8_initial(title);
		}
		else
		{
			char *title_initial = mp_util_get_utf8_initial(title);
			if (title_initial)
			{
				if (strcmp(layout_data->fast_scrooll_index, title_initial) != 0)
				{
					make_group_title = TRUE;
					IF_FREE(layout_data->fast_scrooll_index);
					layout_data->fast_scrooll_index = title_initial;
				}
				else
				{
					IF_FREE(title_initial);
				}
			}
		}

		mp_genlist_item_data_t *item_data;
		item_data = calloc(1, sizeof(mp_genlist_item_data_t));
		if (!item_data)
			goto END;
		item_data->handle = item;
		item_data->group_type = layout_data->group_type;
		item_data->index = index;

		// func.del shouldn't be used in other place....
		itc->func.del = _mp_view_layout_gl_del;

		item_data->it = list_item = elm_genlist_item_append(layout_data->genlist, itc, item_data, NULL,
								    ELM_GENLIST_ITEM_NONE, sel_cb, (void *)index);

		if (make_group_title && layout_data->index_fast != NULL)
		{
			elm_index_item_append(layout_data->index_fast, layout_data->fast_scrooll_index, _mp_view_layout_index_item_selected_cb, list_item);
		}
	}

      END:
	layout_data->load_item_idler = NULL;
	endfunc;
	return EINA_FALSE;
}

static void
_mp_view_layout_check_select_all(mp_layout_data_t * layout_data)
{
	MP_CHECK(layout_data);

	Elm_Object_Item *it;
	bool disabled = false;

	if (layout_data->select_all_checked)
		layout_data->checked_count = layout_data->item_count;
	else
		layout_data->checked_count = 0;

	it = elm_genlist_first_item_get(layout_data->genlist);
	while (it)
	{
		if (elm_genlist_item_select_mode_get(it) != ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY) {
			mp_genlist_item_data_t *it_data = NULL;
			it_data = elm_object_item_data_get(it);
			disabled = elm_object_item_disabled_get(it);	//not to select auto playlist.
			if (it_data && !disabled) {
				it_data->checked = layout_data->select_all_checked;
			}
		}
		it = elm_genlist_item_next_get(it);
	}
	// Update all realized items
	elm_genlist_realized_items_update(layout_data->genlist);
	mp_util_create_selectioninfo_with_count(layout_data->layout, layout_data->checked_count);
	mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);

}

static void
_mp_view_layout_select_all_layout_mouse_down_cb(void *data, Evas * evas, Evas_Object * obj, void *event_info)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);

	if(layout_data->rename_git)
	{
		mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
		return;
	}

	layout_data->select_all_checked = !layout_data->select_all_checked;
	elm_check_state_pointer_set(layout_data->select_all_checkbox, &layout_data->select_all_checked);

	_mp_view_layout_check_select_all(layout_data);
}

static void
_mp_view_layout_select_all_check_changed_cb(void *data, Evas_Object * obj, void *event_info)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);

	if(layout_data->rename_git)
	{
		mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
		layout_data->select_all_checked = !layout_data->select_all_checked;
		elm_check_state_pointer_set(layout_data->select_all_checkbox, &layout_data->select_all_checked);
		return;
	}

	_mp_view_layout_check_select_all(data);
}

static void
_mp_view_layout_create_select_all(mp_layout_data_t * layout_data)
{
	MP_CHECK(layout_data);

	layout_data->select_all_layout = elm_layout_add(layout_data->box);
	elm_layout_theme_set(layout_data->select_all_layout, "genlist", "item", "select_all/default");
	evas_object_size_hint_weight_set(layout_data->select_all_layout, EVAS_HINT_EXPAND, EVAS_HINT_FILL);
	evas_object_size_hint_align_set(layout_data->select_all_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_event_callback_add(layout_data->select_all_layout, EVAS_CALLBACK_MOUSE_DOWN,
				       _mp_view_layout_select_all_layout_mouse_down_cb, layout_data);

	Evas_Object *check = layout_data->select_all_checkbox = elm_check_add(layout_data->select_all_layout);
	elm_check_state_pointer_set(check, &layout_data->select_all_checked);
	evas_object_smart_callback_add(check, "changed", _mp_view_layout_select_all_check_changed_cb, layout_data);
	evas_object_propagate_events_set(check, EINA_FALSE);
	elm_object_part_content_set(layout_data->select_all_layout, "elm.icon", check);

	edje_object_part_text_set(elm_layout_edje_get(layout_data->select_all_layout), "elm.text",
				  GET_SYS_STR("IDS_COM_BODY_SELECT_ALL"));

	mp_language_mgr_register_object(layout_data->select_all_layout, OBJ_TYPE_EDJE_OBJECT, "elm.text", "IDS_COM_BODY_SELECT_ALL");

	elm_box_pack_start(layout_data->box, layout_data->select_all_layout);
	evas_object_show(layout_data->select_all_layout);
}

static void
_mp_view_layout_load_search_item(Evas_Object * view_layout)
{
	startfunc;
	int count = 0;
	gint track_count = 0;
	gint artist_count = 0;
	gint album_count = 0;

	gint index = 0;
	int ret = 0;
	mp_media_list_h svc_handle = NULL;

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_retm_if(!layout_data, "layout_data is null");
	MP_CHECK_LAYOUT_DATA(layout_data);
	mp_retm_if(layout_data->b_loading, "func called while loading");
	layout_data->b_loading = TRUE;

	elm_genlist_clear(layout_data->genlist);

	ret = mp_media_info_group_list_count(MP_GROUP_BY_ARTIST, NULL, layout_data->filter_str, &artist_count);
	if (ret != 0)
	{
		DEBUG_TRACE("Fail to create structure");
		goto END;
	}
	ret = mp_media_info_group_list_count(MP_GROUP_BY_ALBUM, NULL, layout_data->filter_str, &album_count);
	if (ret != 0)
	{
		DEBUG_TRACE("Fail to create structure");
		goto END;
	}
	ret = mp_media_info_list_count(MP_TRACK_ALL, NULL, NULL, layout_data->filter_str, 0, &track_count);
	if (ret != 0)
	{
		DEBUG_TRACE("Fail to create structure");
		goto END;
	}

	count = artist_count + album_count + track_count;
	layout_data->track_count = track_count;

	if(_mp_view_layout_set_sentinel(layout_data, count))
		goto END;

	static Elm_Genlist_Item_Class group_class = {
		.version = ELM_GENGRID_ITEM_CLASS_VERSION,
		.refcount = 0,
		.delete_me = EINA_FALSE,
		.item_style = "2text.1icon",
		.func.text_get = mp_group_view_list_label_get,
		.func.content_get = mp_group_view_icon_get,
		.func.del = _mp_view_layout_gl_del,
	};

	static Elm_Genlist_Item_Class track_class = {
		.version = ELM_GENGRID_ITEM_CLASS_VERSION,
		.refcount = 0,
		.delete_me = EINA_FALSE,
		.item_style = "3text.1icon.1",
		.func.text_get = mp_common_track_list_label_get,
		.func.content_get = mp_common_track_list_icon_get,
		.func.del = _mp_view_layout_gl_del,
	};

	if (artist_count)
	{
		DEBUG_TRACE("append artist list items");
		_mp_view_layout_append_group_title(layout_data, ("IDS_MUSIC_TAB4_ARTISTS"));
		ret = mp_media_info_group_list_create(&svc_handle, MP_GROUP_BY_ARTIST, NULL, layout_data->filter_str, 0, artist_count);
		if (ret != 0)
		{
			DEBUG_TRACE("Fail to get items");
			goto END;
		}

		for (index = 0; index < artist_count; index++)
		{
			mp_media_info_h item = NULL;
			Elm_Object_Item *list_item = NULL;
			char *title = NULL;

			item = mp_media_info_group_list_nth_item(svc_handle, index);
			if (item == NULL)
			{
				DEBUG_TRACE("Fail to mp_media_info_group_list_nth_item, ret[%d], index[%d]", ret, index);
				goto END;
			}
			ret = mp_media_info_group_get_main_info(item, &title);

			mp_genlist_item_data_t *item_data;
			item_data = calloc(1, sizeof(mp_genlist_item_data_t));
			MP_CHECK(item_data);
			item_data->handle = item;
			item_data->group_type = MP_GROUP_BY_ARTIST;

			item_data->it = list_item =
				elm_genlist_item_append(layout_data->genlist, &group_class, item_data,
							layout_data->search_group_git, ELM_GENLIST_ITEM_NONE,
							mp_group_view_group_list_select_cb, (void *)index);

		}

		if (layout_data->artist_handle)
		{
			mp_media_info_group_list_destroy(layout_data->artist_handle);
			layout_data->artist_handle = NULL;
		}
		layout_data->artist_handle = svc_handle;

	}

	if (album_count)
	{
		DEBUG_TRACE("append album_count list items");
		_mp_view_layout_append_group_title(layout_data, ("IDS_MUSIC_TAB4_ALBUMS"));

		ret = mp_media_info_group_list_create(&svc_handle, MP_GROUP_BY_ALBUM, NULL, layout_data->filter_str, 0, album_count);
		if (ret != 0)
		{
			DEBUG_TRACE("Fail to get items");
			goto END;
		}

		for (index = 0; index < album_count; index++)
		{
			mp_media_info_h item = NULL;
			Elm_Object_Item *list_item = NULL;
			char *title = NULL;
			item = mp_media_info_group_list_nth_item(svc_handle, index);
			if (item == NULL)
			{
				DEBUG_TRACE("Fail to mp_media_info_group_list_nth_item, ret[%d], index[%d]", ret, index);
				goto END;
			}
			ret = mp_media_info_group_get_main_info(item, &title);

			mp_genlist_item_data_t *item_data;
			item_data = calloc(1, sizeof(mp_genlist_item_data_t));
			MP_CHECK(item_data);
			item_data->handle = item;
			item_data->group_type = MP_GROUP_BY_ALBUM;

			item_data->it = list_item =
				elm_genlist_item_append(layout_data->genlist, &group_class, item_data,
							layout_data->search_group_git, ELM_GENLIST_ITEM_NONE,
							mp_group_view_group_list_select_cb, (void *)index);

		}

		if (layout_data->album_handle)
		{
			mp_media_info_group_list_destroy(layout_data->album_handle);
			layout_data->album_handle = NULL;
		}
		layout_data->album_handle = svc_handle;

	}

	if (track_count)
	{
		DEBUG_TRACE("append track_count list items");
		_mp_view_layout_append_group_title(layout_data, ("IDS_MUSIC_HEADER_SONGS"));

		ret = mp_media_info_list_create(&svc_handle, MP_TRACK_ALL, NULL, NULL, layout_data->filter_str, 0, 0, track_count);
		if (ret != 0)
		{
			DEBUG_TRACE("Fail to get items");
			goto END;
		}

		for (index = 0; index < track_count; index++)
		{
			mp_media_info_h item = NULL;
			item = mp_media_info_list_nth_item(svc_handle, index);

			mp_genlist_item_data_t *item_data;
			item_data = calloc(1, sizeof(mp_genlist_item_data_t));
			MP_CHECK(item_data);
			item_data->handle = item;
			item_data->group_type = MP_GROUP_NONE;

			item_data->it =
				elm_genlist_item_append(layout_data->genlist, &track_class, item_data,
							layout_data->search_group_git, ELM_GENLIST_ITEM_NONE,
							mp_common_track_genlist_sel_cb, layout_data);
		}

		if (layout_data->track_handle)
		{
			mp_media_info_list_destroy(layout_data->track_handle);
			layout_data->track_handle = NULL;
		}
		layout_data->track_handle = svc_handle;

	}

      END:
	layout_data->b_loading = FALSE;
	endfunc;
}


static void
_mp_view_layout_load_list_item(Evas_Object * view_layout)
{
	startfunc;
	int id = 0; //id for playlist or folder
	gint count = -1;
	gint load_count = -1;
	gint index = 0;
	int ret = 0;
	Elm_Genlist_Item_Class *itc;
	genlist_cb_t sel_cb;


	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_retm_if(!layout_data, "layout_data is null");
	MP_CHECK_LAYOUT_DATA(layout_data);
	mp_retm_if(layout_data->b_loading, "func called while loading");
	layout_data->b_loading = TRUE;
	layout_data->checked_count = 0;
	layout_data->select_all_checked = 0;

	if (layout_data->load_item_idler)
	{
		DEBUG_TRACE("unregister idler");
		ecore_idler_del(layout_data->load_item_idler);
		layout_data->load_item_idler = NULL;
	}

	IF_FREE(layout_data->fast_scrooll_index);

	Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
	if (item)
	{
		elm_genlist_item_bring_in(item, ELM_GENLIST_ITEM_SCROLLTO_IN);
		elm_genlist_clear(layout_data->genlist);
	}


	if(layout_data->track_type == MP_TRACK_BY_PLAYLIST)
		id = layout_data->playlist_id;

	if (layout_data->category == MP_LAYOUT_TRACK_LIST)
	{
		mp_media_info_list_count(layout_data->track_type, layout_data->type_str, layout_data->type_str2, layout_data->filter_str, id, &count);
	}
	else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
		mp_media_info_group_list_count(layout_data->group_type, layout_data->type_str, layout_data->filter_str, &count);
	else if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
	{
		mp_media_info_group_list_count(MP_GROUP_BY_PLAYLIST, NULL, layout_data->filter_str, &count);
		_mp_view_layout_append_auto_playlists(layout_data);
	}
	else
	{
		WARN_TRACE("category is not valid: %d", layout_data->category);
		goto END;
	}

	layout_data->item_count = count;
	DEBUG_TRACE("count: %d", count);

	if(_mp_view_layout_set_sentinel(layout_data, count+layout_data->default_playlist_count))
		goto END;

	if(count < 0)
		goto END;

	if (layout_data->category != MP_LAYOUT_PLAYLIST_LIST && layout_data->view_mode != MP_VIEW_MODE_SEARCH
		&& layout_data->track_type != MP_TRACK_BY_ADDED_TIME && layout_data->track_type != MP_TRACK_BY_PLAYED_COUNT
		&& layout_data->track_type != MP_TRACK_BY_PLAYED_TIME && layout_data->track_type != MP_TRACK_BY_PLAYLIST
		&& layout_data->track_type != MP_TRACK_BY_ALBUM && layout_data->track_type != MP_TRACK_BY_ARTIST_ALBUM)
	{
		if (layout_data->index_fast)
			elm_index_item_clear(layout_data->index_fast);
		else
		{
			layout_data->index_fast = _mp_view_layout_create_fastscroll_index(view_layout, layout_data);
		}
	}


	mp_media_list_h svc_handle;

	if (ret != 0)
	{
		DEBUG_TRACE("Fail to create structure");
		goto END;
	}
	if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
		ret = mp_media_info_group_list_create(&svc_handle, MP_GROUP_BY_PLAYLIST, NULL, layout_data->filter_str, 0, count);
	else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
		ret = mp_media_info_group_list_create(&svc_handle, layout_data->group_type, layout_data->type_str, layout_data->filter_str, 0, count);
	else
	{
		ret = mp_media_info_list_create(&svc_handle, layout_data->track_type, layout_data->type_str, layout_data->type_str2, layout_data->filter_str, id, 0, count);
	}
	if (ret != 0)
	{
		DEBUG_TRACE("Fail to get items");
		goto END;
	}

	if (layout_data->svc_handle)
	{
		if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
			mp_media_info_group_list_destroy(layout_data->svc_handle);
		else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
			mp_media_info_group_list_destroy(layout_data->svc_handle);
		else
		{
			mp_media_info_list_destroy(layout_data->svc_handle);
		}
	}
	layout_data->svc_handle = svc_handle;


	itc = layout_data->itc;
	sel_cb = layout_data->cb_func.selected_cb;

	itc->func.del = _mp_view_layout_gl_del;
	itc->decorate_all_item_style = "edit_default";

	evas_object_smart_callback_del(layout_data->genlist, "moved", _mp_view_layout_reorder);
	evas_object_smart_callback_add(layout_data->genlist, "moved", _mp_view_layout_reorder, layout_data);

	static bool first_loading = true;

	if(first_loading)
	{
		load_count = MIN(count, MP_INIT_ITEM_LOAD_COUNT);
		first_loading = false;
	}
	else
	{
		load_count = count;
		elm_genlist_block_count_set(layout_data->genlist, MP_INIT_ITEM_LOAD_COUNT);
		if(!layout_data->block_size_idler)
			layout_data->block_size_idler = ecore_idler_add(_mp_view_layout_set_block_count_idle_cb, layout_data);
	}

	if ( MP_TRACK_BY_ALBUM == layout_data->track_type
		|| MP_TRACK_BY_ARTIST_ALBUM == layout_data->track_type)
		_mp_view_layout_append_album_group_title(layout_data);
	else if(layout_data->group_type == MP_GROUP_BY_ARTIST_ALBUM)
		_mp_view_layout_append_all_song(layout_data);

	for (index = 0; index < load_count; index++)
	{
		mp_media_info_h item = NULL;
		Elm_Object_Item *list_item = NULL;
		char *title = NULL;

		if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST || layout_data->category == MP_LAYOUT_GROUP_LIST)
		{
			item = mp_media_info_group_list_nth_item(svc_handle, index);
			if (!item)
			{
				DEBUG_TRACE("Fail to mp_media_info_group_list_nth_item, ret[%d], index[%d]", ret, index);
				goto END;
			}
			mp_media_info_group_get_main_info(item, &title);
		}


		else
		{
			item = mp_media_info_list_nth_item(svc_handle, index);
			ret = mp_media_info_get_title(item, &title);
			if (ret != 0)
			{
				DEBUG_TRACE("Fail to mp_media_info_get_title, ret[%d], index[%d]", ret, index);
				goto END;
			}
		}

		bool make_group_title = FALSE;

		if (layout_data->fast_scrooll_index == NULL)
		{
			make_group_title = TRUE;
			layout_data->fast_scrooll_index = mp_util_get_utf8_initial(title);
		}
		else
		{
			char *title_initial = mp_util_get_utf8_initial(title);
			if (title_initial)
			{
				if (strcmp(layout_data->fast_scrooll_index, title_initial) != 0)
				{
					make_group_title = TRUE;
					IF_FREE(layout_data->fast_scrooll_index);
					layout_data->fast_scrooll_index = title_initial;
				}
				else
				{
					IF_FREE(title_initial);
				}
			}
		}



		mp_genlist_item_data_t *item_data;
		item_data = calloc(1, sizeof(mp_genlist_item_data_t));
		MP_CHECK(item_data);
		item_data->handle = item;
		item_data->group_type = layout_data->group_type;
		item_data->index = index;

		Elm_Object_Item *parent_group = NULL;
		if (layout_data->playlist_id < 0
	   	    && MP_LAYOUT_TRACK_LIST == layout_data->category
	    	    && MP_TRACK_BY_ALBUM == layout_data->track_type)
			parent_group = layout_data->album_group;
		item_data->it = list_item = elm_genlist_item_append(layout_data->genlist, itc, item_data, parent_group,
								    ELM_GENLIST_ITEM_NONE, sel_cb, (void *)index);
		elm_object_item_data_set(item_data->it, item_data);

		if (make_group_title && layout_data->index_fast != NULL)
		{
			elm_index_item_append(layout_data->index_fast, layout_data->fast_scrooll_index, _mp_view_layout_index_item_selected_cb, list_item);
		}

	}



	if (count > load_count && layout_data->view_mode != MP_VIEW_MODE_SEARCH)
	{
		if(!layout_data->load_item_idler)
			layout_data->load_item_idler = ecore_idler_add(_mp_view_layout_load_item_idler_cb, layout_data);
	}

	if (layout_data->edit_mode
	    || (layout_data->ad->b_add_tracks && layout_data->view_data->view_type == MP_VIEW_TYPE_SONGS))
	{
		if (!layout_data->select_all_layout)
		{
			_mp_view_layout_create_select_all(layout_data);
		}
		else
		{
			if (layout_data->select_all_checkbox)
				elm_check_state_set(layout_data->select_all_checkbox, false);
		}

		elm_genlist_decorate_mode_set(layout_data->genlist, EINA_TRUE);
		elm_genlist_select_mode_set(layout_data->genlist, ELM_OBJECT_SELECT_MODE_ALWAYS);
	}
	else
	{
		if (layout_data->select_all_layout)
		{
			evas_object_del(layout_data->select_all_layout);
			layout_data->select_all_layout = NULL;
		}
		elm_genlist_decorate_mode_set(layout_data->genlist, EINA_FALSE);
		elm_genlist_select_mode_set(layout_data->genlist, ELM_OBJECT_SELECT_MODE_DEFAULT);
	}

	if (layout_data->reorder && layout_data->playlist_id > 0)	// reordering of favorite list is not allowed..
		elm_genlist_reorder_mode_set(layout_data->genlist, EINA_TRUE);
	else
		elm_genlist_reorder_mode_set(layout_data->genlist, EINA_FALSE);

      END:
	layout_data->b_loading = FALSE;
	endfunc;
}

static void
_mp_view_layout_index_delayed_changed(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	// called on a change but delayed in case multiple changes happen in a
	// short timespan
	elm_genlist_item_bring_in(elm_object_item_data_get(event_info), ELM_GENLIST_ITEM_SCROLLTO_TOP);
}

static void
_mp_view_layout_index_changed(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	// this is calld on every change, no matter how often
	return;
}

static void
_mp_view_layout_index_selected(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	// called on final select
	elm_genlist_item_bring_in(elm_object_item_data_get(event_info), ELM_GENLIST_ITEM_SCROLLTO_TOP);
}

static Evas_Object *
_mp_view_layout_create_fastscroll_index(Evas_Object * parent, mp_layout_data_t * layout_data)
{
	DEBUG_TRACE_FUNC();
	Evas_Object *index = NULL;

	// Create index
	index = elm_index_add(parent);
	elm_object_part_content_set(parent, "elm.swallow.content.index", index);

	evas_object_smart_callback_add(index, "delay,changed", _mp_view_layout_index_delayed_changed, layout_data);
	evas_object_smart_callback_add(index, "changed", _mp_view_layout_index_changed, layout_data);
	evas_object_smart_callback_add(index, "selected", _mp_view_layout_index_selected, layout_data);
	elm_index_level_go(index, 0);

	return index;
}

static void
_mp_view_layout_now_playing_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	startfunc;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	struct appdata *ad = layout_data->ad;
	MP_CHECK(ad);

	if (!strcmp(emission, SIGNAL_NOW_PLAYING_CLICKED)) {
		if(layout_data->rename_git)
		{
			mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
			return;
		}
		mp_common_hide_search_ise_context(layout_data->view_data);
		mp_util_reset_genlist_mode_item(layout_data->genlist);
		mp_play_view_load(layout_data->ad);
	} else if (!strcmp(emission, "play_pause_clicked")){
		if (ad->player_state == PLAY_STATE_PLAYING)
		{
			mp_play_control_play_pause(ad, false);
		}
		else
		{
			mp_play_control_play_pause(ad, true);
		}
	}

	endfunc;
}

static Evas_Object *
_mp_view_layout_create_now_playing(Evas_Object * parent, mp_layout_data_t * layout_data)
{
	Evas_Object *playing_pannel = NULL;
	int r = -1;

	DEBUG_TRACE_FUNC();

	playing_pannel = elm_layout_add(parent);
	if (playing_pannel)
	{
		r = elm_layout_file_set(playing_pannel, PLAY_VIEW_EDJ_NAME, "mp_now_playing");
		if (!r)
		{
			evas_object_del(playing_pannel);
			return NULL;
		}
		evas_object_size_hint_weight_set(playing_pannel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	}

	Evas_Object *thumbnail = elm_bg_add(playing_pannel);
	elm_bg_load_size_set(thumbnail, MP_NOW_PLAYING_ICON_SIZE, MP_NOW_PLAYING_ICON_SIZE);
	elm_object_part_content_set(playing_pannel, "thumb_image", thumbnail);
	layout_data->now_playing_icon = thumbnail;

	Evas_Object *progress = elm_progressbar_add(playing_pannel);
	elm_object_style_set(progress, "music/list_progress");
	elm_progressbar_horizontal_set(progress, EINA_TRUE);
	evas_object_size_hint_align_set(progress, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progress, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_value_set(progress, 0.0);
	evas_object_show(progress);

	elm_object_part_content_set(playing_pannel, "progress_bar", progress);
	layout_data->now_playing_progress = progress;

	mp_retvm_if(playing_pannel == NULL, NULL, "now playing view is NULL");

	edje_object_signal_callback_add(_EDJ(playing_pannel), SIGNAL_NOW_PLAYING_CLICKED, "*",
					_mp_view_layout_now_playing_cb, layout_data);
	edje_object_signal_callback_add(_EDJ(playing_pannel), "play_pause_clicked", "*",
					_mp_view_layout_now_playing_cb, layout_data);

	evas_object_show(playing_pannel);

	return playing_pannel;
}

static void
_mp_widget_gl_drag_cb(void *data, Evas_Object * obj, void *event_info)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);

	if(layout_data->rename_git)
	{
		mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
		return;
	}
}

static Evas_Object *
_mp_view_layout_create_layout(Evas_Object * parent, view_data_t * view_data, mp_layout_data_t * layout_data,
			      mp_view_mode_t view_mode)
{
	Evas_Object *layout = NULL;
	MP_CHECK_NULL(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	if (view_mode == MP_VIEW_MODE_SEARCH)
	{
		layout = elm_layout_add(parent);
		mp_retvm_if(layout == NULL, NULL, "layout is NULL");
		elm_layout_file_set(layout, EDJ_NAME, "main_layout_with_searchbar");

		layout_data->genlist = mp_widget_genlist_create(layout_data->ad, layout, false, false);
		evas_object_data_set(layout_data->genlist, "layout_data", layout_data);

		Evas_Object *box = elm_box_add(layout);
		evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(box);
		layout_data->box = box;

		elm_box_pack_end(box, layout_data->genlist);

		elm_object_part_content_set(layout, "list_content", box);

		Evas_Object *search_bar = mp_search_create_new(layout,
			mp_view_layout_search_changed_cb, layout_data, mp_common_back_button_cb, view_data);
		if (!search_bar) {
			mp_error("fail to create search bar");
			mp_evas_object_del(layout);
			return NULL;
		}
		elm_object_part_content_set(layout, "search_bar", search_bar);
		evas_object_show(search_bar);
		layout_data->search_bar = search_bar;

	}
	else			// use conformant to use auto scroll
	{
		TA_S(8, "elm_layout_add");
		layout = elm_layout_add(parent);
		mp_retvm_if(layout == NULL, NULL, "layout is NULL");
		elm_layout_file_set(layout, EDJ_NAME, "main_layout");
		TA_E(8, "elm_layout_add");

		TA_S(8, "mp_widget_genlist_create");
		layout_data->genlist = mp_widget_genlist_create(layout_data->ad, layout, true, true);
		evas_object_data_set(layout_data->genlist, "layout_data", layout_data);
		TA_E(8, "mp_widget_genlist_create");

		TA_S(8, "elm_box_add");
		Evas_Object *box = elm_box_add(layout);
		evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(box);
		TA_E(8, "elm_box_add");
		layout_data->box = box;

		TA_S(8, "elm_box_pack_end");
		elm_box_pack_end(box, layout_data->genlist);
		TA_E(8, "elm_box_pack_end");

		elm_object_part_content_set(layout, "list_content", box);
	}
	evas_object_event_callback_add(layout_data->genlist, EVAS_CALLBACK_FREE, _mp_view_layout_genlist_del_cb,
				       layout_data);
	evas_object_smart_callback_add(layout_data->genlist, "drag,start,up", _mp_widget_gl_drag_cb, layout_data);
	evas_object_smart_callback_add(layout_data->genlist, "drag,start,down", _mp_widget_gl_drag_cb, layout_data);

	return layout;
}

Evas_Object *
mp_view_layout_create(Evas_Object * parent, view_data_t * view_data, mp_view_mode_t view_mode)
{
	startfunc;
	MP_CHECK_NULL(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	mp_layout_data_t *layout_data = calloc(1, sizeof(mp_layout_data_t));
	MP_CHECK_NULL(layout_data);
	MP_SET_LAYOUT_DATA_MAGIC(layout_data);

	layout_data->ad = view_data->ad;
	layout_data->view_data = view_data;
	layout_data->view_mode = view_mode;
	layout_data->playlist_id = -1;
	TA_S(7, "_mp_view_layout_create_layout");
	Evas_Object *layout = _mp_view_layout_create_layout(parent, view_data, layout_data, view_mode);
	TA_E(7, "_mp_view_layout_create_layout");
	if (!layout) {
		mp_error("fail to create layout");
		SAFE_FREE(layout_data);
		return NULL;
	}
	layout_data->layout = layout;

	evas_object_show(layout);


	evas_object_data_set(layout, "layout_data", layout_data);
	DEBUG_TRACE("layout_data: 0x%x", layout_data);

	endfunc;
	return layout;
}

static void
_set_playlist_handle(mp_layout_data_t *layout_data)
{
	int res = 0;
	int i, count = 0;
	mp_media_list_h list = NULL;
	mp_media_info_h media = NULL;

	res = mp_media_info_group_list_count(MP_GROUP_BY_PLAYLIST, NULL, NULL, &count);
	MP_CHECK(res == 0);

	res = mp_media_info_group_list_create(&list, MP_GROUP_BY_PLAYLIST, NULL, NULL, 0, count);
	MP_CHECK(res == 0);

	for(i=0; i<count; i++)
	{
		int playlist_id;
		media = mp_media_info_group_list_nth_item(list, i);
		mp_media_info_group_get_playlist_id(media, &playlist_id);
		if(playlist_id == layout_data->playlist_id)
			break;
	}
	if(layout_data->playlists)
		mp_media_info_group_list_destroy(layout_data->playlists);

	layout_data->playlists = list;
	layout_data->playlist_handle = media;
}

void
mp_view_layout_set_layout_data(Evas_Object * view_layout, ...)
{
	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	va_list var_args;
	int field;

	va_start(var_args, view_layout);
	do
	{
		field = va_arg(var_args, int);
		if (field < 0)
		{
			break;
		}

		switch (field)
		{
		case MP_LAYOUT_CATEGORY_TYPE:
			{
				int val = va_arg((var_args), int);
				layout_data->category = val;
				DEBUG_TRACE("layout_data->category = %d", layout_data->category);
				break;
			}
		case MP_LAYOUT_TRACK_LIST_TYPE:
			{
				int val = va_arg((var_args), int);

				layout_data->track_type = val;
				DEBUG_TRACE("layout_data->track_type = %d", layout_data->track_type);
				break;
			}
		case MP_LAYOUT_GROUP_LIST_TYPE:
			{
				int val = va_arg((var_args), int);
				layout_data->group_type = val;
				DEBUG_TRACE("layout_data->group_type = %d", layout_data->group_type);
				break;
			}
		case MP_LAYOUT_PLAYLIT_ID:
			{
				int val = va_arg((var_args), int);
				layout_data->playlist_id = val;
				DEBUG_TRACE("layout_data->playlist_id = %d", layout_data->playlist_id);

				_set_playlist_handle(layout_data);

				break;
			}
		case MP_LAYOUT_TYPE_STR:
			{
				char *val = va_arg((var_args), char *);
				SAFE_FREE(layout_data->type_str);
				layout_data->type_str = g_strdup(val);
				DEBUG_TRACE("layout_data->type_str = %s", layout_data->type_str);

				break;
			}
		case MP_LAYOUT_TYPE_STR2:
			{
				char *val = va_arg((var_args), char *);
				SAFE_FREE(layout_data->type_str2);
				layout_data->type_str2 = g_strdup(val);
				DEBUG_TRACE("layout_data->type_str = %s", layout_data->type_str2);

				break;
			}
		case MP_LAYOUT_FILTER_STR:
			{
				char *val = va_arg((var_args), char *);
				SAFE_FREE(layout_data->filter_str);
				layout_data->filter_str = g_strdup(val);
				DEBUG_TRACE("layout_data->filter_str = %s", layout_data->filter_str);

				break;
			}
		case MP_LAYOUT_EDIT_MODE:
			{
				int val = va_arg((var_args), int);
				layout_data->edit_mode = val;
				DEBUG_TRACE("layout_data->edit_mode = %d", layout_data->edit_mode);
				break;
			}
		case MP_LAYOUT_REORDER_MODE:
			{
				int val = va_arg((var_args), int);
				layout_data->reorder = val;
				DEBUG_TRACE("layout_data->reorder = %d", layout_data->reorder);
				break;
			}
		case MP_LAYOUT_GENLIST_ITEMCLASS:
			{
				Elm_Genlist_Item_Class *itc = va_arg((var_args), Elm_Genlist_Item_Class *);
				if (itc) {
					if (layout_data->itc) {
						elm_genlist_item_class_free(layout_data->itc);
						layout_data->itc = NULL;
					}
					layout_data->itc = itc;
				}
				break;
			}
		case MP_LAYOUT_GENLIST_AUTO_PLAYLIST_ITEMCLASS:
			{
				Elm_Genlist_Item_Class *itc = va_arg((var_args), Elm_Genlist_Item_Class *);
				if (itc)
					memcpy(&(layout_data->auto_playlist_item_class), itc,
					       sizeof(Elm_Genlist_Item_Class));
				break;
			}
		case MP_LAYOUT_LIST_CB:
			{
				mp_genlist_cb_t *cb_func = va_arg((var_args), mp_genlist_cb_t *);
				if (cb_func)
					memcpy(&(layout_data->cb_func), cb_func, sizeof(layout_data->cb_func));
				break;
			}
		default:
			DEBUG_TRACE("Invalid arguments");
		}

	}
	while (field);

	va_end(var_args);

}

void
mp_view_layout_destroy(Evas_Object * view_layout)
{
	mp_retm_if(!view_layout, "");
	if (view_layout)
		evas_object_del(view_layout);
}

void
mp_view_layout_update(Evas_Object * view_layout)
{
	startfunc;
	MP_CHECK(view_layout);
	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_retm_if(!layout_data, "layout_data is null!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);

		if (layout_data->view_mode != MP_VIEW_MODE_SEARCH) {
			{
				_mp_view_layout_load_list_item(view_layout);
			}

			if (layout_data->ad->show_now_playing)
			{
				//if (!layout_data->ad->b_add_tracks && !layout_data->edit_mode)
				if (!layout_data->ad->b_add_tracks)
				{
					mp_view_layout_show_now_playing(view_layout);
					mp_view_layout_set_now_playing_info(view_layout);
				}
				else
					mp_view_layout_hide_now_playing(view_layout);
			}
			else
			{
				mp_view_layout_hide_now_playing(view_layout);
			}
		}
		else
			_mp_view_layout_load_search_list_item(view_layout);
		DEBUG_TRACE("category: %d, track_type: %d, group_type: %d", layout_data->category,
			    layout_data->track_type, layout_data->group_type);

	if (view_layout == mp_view_manager_get_last_view_layout(layout_data->ad))
	{
		mp_view_layout_progress_timer_thaw(view_layout);
		mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);
	}

	/* update the first controlba item */
	mp_view_manager_update_first_controlbar_item(layout_data);
	endfunc;
}

void
mp_view_layout_clear(Evas_Object * view_layout)
{
	mp_retm_if(!view_layout, "");

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_retm_if(!layout_data, "");
	MP_CHECK_LAYOUT_DATA(layout_data);

	Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
	if (item)
		elm_genlist_item_bring_in(item, ELM_GENLIST_ITEM_SCROLLTO_IN);

	elm_genlist_clear(layout_data->genlist);
	SAFE_FREE(layout_data->filter_str);
	mp_search_text_set(layout_data->search_bar, "");

	if (layout_data->progress_timer)
		ecore_timer_freeze(layout_data->progress_timer);
	mp_view_layout_set_edit_mode(layout_data, false);

}

static void
_mp_view_layout_set_progress_pos(void *data)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK(layout_data->ad);

	double pos = layout_data->ad->music_pos;
	double duration = layout_data->ad->music_length;
	double value = 0.0;

	if (duration > 0.0)
		value = pos / duration;

	elm_progressbar_value_set(layout_data->now_playing_progress, value);
}


static Eina_Bool
_mp_view_layout_progress_timer_cb(void *data)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK_FALSE(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	MP_CHECK_FALSE(layout_data->ad);

	layout_data->ad->music_pos = mp_player_mgr_get_position() / 1000.0;
	layout_data->ad->music_length = mp_player_mgr_get_duration() / 1000.0;

	_mp_view_layout_set_progress_pos(data);
	return ECORE_CALLBACK_RENEW;
}

void
mp_view_layout_show_now_playing(Evas_Object * view_layout)
{
	mp_retm_if(!view_layout, "view_layout is null!!");

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK(layout_data);
	/* get layout_data of landscape square view */

	if(!layout_data->now_playing)
	{
		layout_data->now_playing = _mp_view_layout_create_now_playing(layout_data->layout, layout_data);
		elm_object_part_content_set(layout_data->layout, "now_playing", layout_data->now_playing);
	}

	MP_CHECK(layout_data->now_playing_progress);

	edje_object_signal_emit(_EDJ(layout_data->layout), "SHOW_NOW_PLAING", "music_app");

	_mp_view_layout_set_progress_pos(layout_data);

	if (!layout_data->progress_timer)
		layout_data->progress_timer = ecore_timer_add(0.5, _mp_view_layout_progress_timer_cb, layout_data);

	MP_CHECK(layout_data->ad);
	if(layout_data->ad->player_state != PLAY_STATE_PLAYING)
		ecore_timer_freeze(layout_data->progress_timer);

	_mp_view_layout_update_icon(view_layout);

}

void
mp_view_layout_hide_now_playing(Evas_Object * view_layout)
{
	mp_retm_if(!view_layout, "view_layout is null!!");

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	/* get layout_data of landscape square view */

	if (layout_data->progress_timer)
	{
		ecore_timer_del(layout_data->progress_timer);
		layout_data->progress_timer = NULL;
	}

	edje_object_signal_emit(_EDJ(layout_data->layout), "HIDE_NOW_PLAING", "music_app");
}

void
mp_view_layout_progress_timer_thaw(Evas_Object * view_layout)
{
	mp_retm_if(!view_layout, "view_layout is null!!");

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	MP_CHECK(layout_data->ad);

	/* get layout_data of landscape square view */


	if (layout_data->progress_timer && layout_data->ad->player_state == PLAY_STATE_PLAYING)
		ecore_timer_thaw(layout_data->progress_timer);

	_mp_view_layout_set_progress_pos(layout_data);
	_mp_view_layout_update_icon(view_layout);
}

void
mp_view_layout_progress_timer_freeze(Evas_Object * view_layout)
{
	mp_retm_if(!view_layout, "view_layout is null!!");

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	/* get layout_data of landscape square view */

	if (layout_data->progress_timer)
		ecore_timer_freeze(layout_data->progress_timer);
	_mp_view_layout_set_progress_pos(layout_data);
	_mp_view_layout_update_icon(view_layout);
}

void
mp_view_layout_set_now_playing_info(Evas_Object * view_layout)
{
	startfunc;
	mp_retm_if(!view_layout, "view_layout is null!!");
	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_retm_if(!layout_data, "Layout data is null !!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);
	mp_retm_if(!layout_data->ad, "layout_data->ad is null !!!!");

	/* get layout_data of landscape square view */
	MP_CHECK(layout_data->now_playing);
	MP_CHECK(layout_data->now_playing_icon);

	mp_track_info_t *current_item = layout_data->ad->current_track_info;
	MP_CHECK(current_item);


	if (!mp_util_is_image_valid(layout_data->ad->evas, current_item->thumbnail_path))
		elm_bg_file_set(layout_data->now_playing_icon, DEFAULT_THUMBNAIL, NULL);
	else
		elm_bg_file_set(layout_data->now_playing_icon, current_item->thumbnail_path, NULL);

	char *label =
		g_strdup_printf("%s / %s",
				(current_item->title ? current_item->title : GET_SYS_STR("IDS_COM_BODY_UNKNOWN")),
				(current_item->artist ? current_item->artist : GET_SYS_STR("IDS_COM_BODY_UNKNOWN")));

	edje_object_part_text_set(_EDJ(layout_data->now_playing), "now_playing_label", label);
	IF_FREE(label);

}

static void
_mp_view_layout_update_icon(Evas_Object * view_layout)
{
	startfunc;
	mp_retm_if(!view_layout, "view_layout is null!!");
	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_retm_if(!layout_data, "Layout data is null !!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);
	mp_retm_if(!layout_data->ad, "layout_data->ad is null !!!!");

	/* get layout_data of landscape square view */

	MP_CHECK(layout_data->now_playing);

	if(layout_data->ad->player_state != PLAY_STATE_PLAYING)
		edje_object_signal_emit(_EDJ(layout_data->now_playing), "pause", "playing_icon");
	else
		edje_object_signal_emit(_EDJ(layout_data->now_playing), "play", "playing_icon");
}

void
mp_view_layout_reset_select_all(mp_layout_data_t * layout_data)
{
	MP_CHECK(layout_data);
	MP_CHECK(layout_data->select_all_checkbox);

	layout_data->checked_count = 0;
	layout_data->select_all_checked = 0;
	elm_check_state_set(layout_data->select_all_checkbox, false);
}

void
mp_view_layout_set_edit_mode(mp_layout_data_t * layout_data, bool edit_mode)
{
	startfunc;
	MP_CHECK(layout_data);
	if (edit_mode)
	{
		if (!layout_data->select_all_layout)
		{
			_mp_view_layout_create_select_all(layout_data);
		}
		else
		{
			if (layout_data->select_all_checkbox)
				elm_check_state_set(layout_data->select_all_checkbox, false);
		}

		Elm_Object_Item *sweeped_item = (Elm_Object_Item *)elm_genlist_decorated_item_get(layout_data->genlist);
		if (sweeped_item)
		{
			elm_genlist_item_decorate_mode_set(sweeped_item, "slide", EINA_FALSE);
			elm_genlist_item_select_mode_set(sweeped_item, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_update(sweeped_item);
		}

		if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
		{
			int i = 0;
			Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
			for (i = 0; i < layout_data->default_playlist_count; i++)
			{
				Elm_Object_Item *next = NULL;
				next = elm_genlist_item_next_get(item);
				elm_object_item_disabled_set(item, EINA_TRUE);
				item = next;
			}
		}
		else if(layout_data->group_type == MP_GROUP_BY_ARTIST_ALBUM)
		{
			Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
			elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		}

		if (layout_data->reorder)
			elm_genlist_reorder_mode_set(layout_data->genlist, EINA_TRUE);
		elm_genlist_decorate_mode_set(layout_data->genlist, EINA_TRUE);

		elm_genlist_select_mode_set(layout_data->genlist, ELM_OBJECT_SELECT_MODE_ALWAYS);
	}
	else
	{

		if (layout_data->select_all_layout)
		{
			evas_object_del(layout_data->select_all_layout);
			layout_data->select_all_layout = NULL;
		}

		elm_genlist_decorate_mode_set(layout_data->genlist, EINA_FALSE);
		layout_data->edit_mode = 0;
		elm_genlist_select_mode_set(layout_data->genlist, ELM_OBJECT_SELECT_MODE_DEFAULT);

		if(layout_data->reordered)
		{
			_mp_view_layout_reorder_item(layout_data);
			layout_data->reordered = false;
		}

		layout_data->reorder = 0;
		if (layout_data->checked_count)
		{
			Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
			while (item)
			{
				if(elm_genlist_item_select_mode_get(item) != ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
				{
					mp_genlist_item_data_t *item_data =
						(mp_genlist_item_data_t *) elm_object_item_data_get(item);
					MP_CHECK(item_data);
					item_data->checked = EINA_FALSE;
				}
				item = elm_genlist_item_next_get(item);
			}
			layout_data->checked_count = 0;
		}

		layout_data->selected_count = 0;
		layout_data->select_all_checked = false;

		mp_view_manager_set_title_and_buttons(layout_data->view_data, layout_data->navibar_title,
						      layout_data->callback_data);
		mp_common_set_toolbar_button_sensitivity(layout_data, 0);

		if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
		{
			int i = 0;
			Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
			for (i = 0; i < layout_data->default_playlist_count; i++)
			{
				Elm_Object_Item *next = NULL;
				next = elm_genlist_item_next_get(item);
				elm_object_item_disabled_set(item, EINA_FALSE);
				item = next;
			}
		}
		else if(layout_data->group_type == MP_GROUP_BY_ARTIST_ALBUM)
		{
			Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
			elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DEFAULT);
		}
		mp_util_create_selectioninfo_with_count(layout_data->layout, 0);

	}

	endfunc;
}


