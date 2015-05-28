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
#include <stdio.h>
#include <glib.h>
#include <system_settings.h>

#include "mp-player-debug.h"
#include "mp-menu.h"
#include "mp-util.h"
#include "mp-popup.h"
#include "mp-playlist-mgr.h"
#include "mp-ug-launch.h"
#include "mp-item.h"
#include "mp-player-drm.h"
#include "mp-widget.h"
#include "mp-ctxpopup.h"

#include "mp-setting-ctrl.h"

#ifndef MP_SOUND_PLAYER
#include "mp-view-layout.h"
#include "mp-playlist-view.h"
#include "mp-common.h"
#include "mp-view-manager.h"
#endif


#define MP_MENU_FID "mp_menu_fid"
#define MP_MENU_PLAY_LIST_FID "mp_menu_playlist_id"
#define MP_MENU_POPUP_PLAY_LIST_HANDLER 		"mp_menu_popup_handler"
#define MP_MENU_GROUP_ITEM_HANDLER    	"mp_menu_group_item_handler"

typedef enum
{
	MP_MENU_FUNC_ADD_TO_LIST = 0,
	MP_MENU_FUNC_DELETE,
} mp_menu_func_type;


mp_track_type_e
mp_menu_get_track_type_by_group(mp_group_type_e group_type)
{
	mp_track_type_e item_type = MP_TRACK_ALL;

	if (group_type == MP_GROUP_BY_ALBUM)
	{
		item_type = MP_TRACK_BY_ALBUM;
	}
	else if (group_type == MP_GROUP_BY_ARTIST)
	{
		item_type = MP_TRACK_BY_ARTIST;
	}
	else if (group_type == MP_GROUP_BY_ARTIST_ALBUM)
	{
		item_type = MP_TRACK_BY_ALBUM;
	}
	else if (group_type == MP_GROUP_BY_GENRE)
	{
		item_type = MP_TRACK_BY_GENRE;
	}
	else if (group_type == MP_GROUP_BY_YEAR)
	{
		item_type = MP_TRACK_BY_YEAR;
	}
	else if (group_type == MP_GROUP_BY_COMPOSER)
	{
		item_type = MP_TRACK_BY_COMPOSER;
	}
	else if (group_type == MP_GROUP_BY_FOLDER)
	{
		item_type = MP_TRACK_BY_FOLDER;
	}

	return item_type;
}

#ifndef MP_SOUND_PLAYER
bool
_mp_menu_func_by_group_handle(int plst_id, mp_layout_data_t * layout_data, mp_media_info_h svc_handle,
			      mp_menu_func_type menu_func)
{
	startfunc;

	MP_CHECK_FALSE(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	struct appdata *ad = layout_data->ad;
	MP_CHECK_FALSE(ad);

	int err = -1;
	int count = 0;
	int index = 0;
	int ret = 0;
	int playlist_item_count = 0;

	mp_media_list_h tracks = NULL;
	mp_track_type_e item_type = MP_TRACK_ALL;

	item_type = mp_menu_get_track_type_by_group(layout_data->group_type);

	char *name = NULL;

	if ((item_type == MP_TRACK_BY_FOLDER))
	{
		ret = mp_media_info_group_get_folder_id(svc_handle, &name);
	}
	else
	{
		ret = mp_media_info_group_get_main_info(svc_handle, &name);
	}

	mp_retvm_if(ret != 0, FALSE, "Fail to get value");
	mp_retvm_if(name == NULL, FALSE, "Fail to get value");
	mp_debug("%s", name);

	ret = mp_media_info_list_count(item_type, name, NULL, NULL, 0, &count);
	MP_CHECK_EXCEP(ret == 0);

	ret = mp_media_info_list_create(&tracks, item_type, name, NULL, NULL, 0, 0, count);
	MP_CHECK_EXCEP(ret == 0);

#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
	if (menu_func == MP_MENU_FUNC_ADD_TO_LIST)
	{
		mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, plst_id, &playlist_item_count);
		if (playlist_item_count >= MP_PLAYLIST_MAX_ITEM_COUNT)
		{
			return false;
		}
	}
#endif

	for (index = 0; index < count; ++index)
	{
		char *fid = 0;
		char *path = NULL;
		mp_media_info_h item;

		item = mp_media_info_list_nth_item(tracks, index);
		mp_media_info_get_media_id(item, &fid);
		mp_media_info_get_file_path(item, &path);

		if (menu_func == MP_MENU_FUNC_ADD_TO_LIST)
		{
#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
			if (playlist_item_count >= MP_PLAYLIST_MAX_ITEM_COUNT)
			{
				goto mp_exception;
			}
#endif
			err = mp_media_info_playlist_add_media(plst_id, fid);
			MP_CHECK_EXCEP(err == 0);
			playlist_item_count++;
		}
		else
		{
			MP_CHECK_EXCEP(path);
			err = remove(path);
			MP_CHECK_EXCEP(err == 0);
		}
	}
	mp_media_info_list_destroy(tracks);

	return true;

      mp_exception:
	mp_media_info_list_destroy(tracks);
	return false;
}

void
mp_menu_add_to_play_list_cancel_create_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");

	Evas_Object *current_navi_bar = NULL;
	current_navi_bar = ad->naviframe;
	MP_CHECK(current_navi_bar);

	view_data_t *view_data = evas_object_data_get(current_navi_bar, "view_data");
	mp_view_manager_pop_view_content(view_data, FALSE, TRUE);
	return;
}

void
mp_menu_add_to_play_list_done_create_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");

	Evas_Object *current_navi_bar = NULL;
	current_navi_bar = ad->naviframe;
	MP_CHECK(current_navi_bar);

	int plst_uid = -1;

	char *converted_name = NULL;
	Evas_Object *entry = mp_widget_editfield_entry_get(ad->editfiled_new_playlist);
	const char *name = elm_entry_entry_get(entry);

	if (name == NULL || strlen(name) == 0)
	{
		name = elm_object_part_text_get(ad->editfiled_new_playlist, "elm.guidetext");
	}
	converted_name = elm_entry_markup_to_utf8(name);

	plst_uid = mp_util_create_playlist(ad, converted_name, NULL);
	IF_FREE(converted_name);
	if (plst_uid < 0)
		return;

	view_data_t *view_data = evas_object_data_get(current_navi_bar, "view_data");
	mp_view_manager_pop_view_content(view_data, FALSE, TRUE);

	if (view_data->view_type == MP_VIEW_TYPE_PLAYLIST)
		mp_view_manager_update_list_contents(view_data, FALSE);

	mp_layout_data_t *layout_data = ad->layout_data;
	mp_list_category_t category = MP_LAYOUT_TRACK_LIST;

	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		category = layout_data->category;
	}

	bool result = false;

	if (category == MP_LAYOUT_TRACK_LIST)
	{
		result = mp_util_add_to_playlist_by_key(plst_uid, ad->fid);
	}
	else if (category == MP_LAYOUT_GROUP_LIST)
	{
		MP_CHECK(ad->group_item_handler);

		result = _mp_menu_func_by_group_handle(plst_uid, layout_data,
						       ad->group_item_handler, MP_MENU_FUNC_ADD_TO_LIST);
	}

	if (result)
	{
		mp_debug("sucess add to play list");
		mp_util_post_status_message(ad, GET_STR("IDS_MUSIC_POP_ADDED"));
	}
	else
	{
		mp_debug("fail add to play list");
#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
		char *fmt_str = GET_STR("IDS_MUSIC_POP_UNABLE_TO_ADD_MORE_THAN_PD_MUSIC_FILE");
		char *noti_str = g_strdup_printf(fmt_str, MP_PLAYLIST_MAX_ITEM_COUNT);
		mp_util_post_status_message(ad, noti_str);
		IF_FREE(noti_str);
#else
		mp_util_post_status_message(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_ADD"));
#endif
	}

	return;
}

static void
_mp_menu_add_playlist_create_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;

	MP_CHECK(data);

	struct appdata *ad = data;
	Evas_Object *current_navi_bar = NULL;
	view_data_t *view_data = NULL;
	char *new_playlist_name = NULL;

	MP_CHECK(ad);
	MP_CHECK(ad->library);

	current_navi_bar = ad->naviframe;

	view_data = evas_object_data_get(current_navi_bar, "view_data");

	new_playlist_name = mp_util_get_new_playlist_name();
	Evas_Object *create_plst_layout = mp_common_create_editfield_layout(current_navi_bar, ad, new_playlist_name);
	IF_FREE(new_playlist_name);
	mp_retm_if(create_plst_layout == NULL, "create_plst_layout is NULL");

	Elm_Object_Item *it = mp_view_manager_push_view_content(view_data, create_plst_layout, MP_VIEW_CONTENT_NEW_PLAYLIST_BY_SWEEP);
	elm_object_item_text_set(it, GET_STR("IDS_MUSIC_BODY_CREATE_PLAYLIST"));
	mp_language_mgr_register_object_item(it, "IDS_MUSIC_BODY_CREATE_PLAYLIST");

	Evas_Object *btn = mp_widget_create_button(create_plst_layout, "naviframe/toolbar/default", GET_SYS_STR("IDS_COM_OPT_SAVE"), NULL, mp_menu_add_to_play_list_done_create_cb, ad);
	elm_object_item_part_content_set(it, "title_toolbar_button1", btn);
	btn = mp_widget_create_button(create_plst_layout, "naviframe/back_btn/default", NULL, NULL, mp_playlist_view_create_new_cancel_cb, view_data);
	elm_object_item_part_content_set(it, "title_prev_btn", btn);

	mp_common_add_keypad_state_callback(ad->conformant, create_plst_layout, it);

	mp_view_manager_set_back_button(create_plst_layout, it, mp_playlist_view_create_new_cancel_cb, view_data);

	evas_object_show(create_plst_layout);

	ad->layout_data = evas_object_data_get(obj, "layout_data");
	mp_layout_data_t *layout_data = ad->layout_data;

	if(layout_data)
	{
		if (layout_data->category == MP_LAYOUT_TRACK_LIST)
		{
			IF_FREE(ad->fid);
			ad->fid = (char *)evas_object_data_get(obj, MP_MENU_FID);
		}
		else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
		{
			ad->group_item_handler = evas_object_data_get(obj, MP_MENU_GROUP_ITEM_HANDLER);
		}

		if (layout_data->genlist)
			mp_util_reset_genlist_mode_item(layout_data->genlist);
	}
	else
		ad->fid = (char *)evas_object_data_get(obj, MP_MENU_FID);

	mp_popup_destroy(ad);

	endfunc;
}


static void
_mp_menu_add_playlist_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	struct appdata *ad = NULL;
	int ret = 0;
	int playlist_id = -1;

	MP_CHECK(data);

	mp_media_info_h item_handler = data;

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_list_category_t category = MP_LAYOUT_TRACK_LIST;

	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		ad = layout_data->ad;
		category = layout_data->category;

	}
	else
	{
		ad = evas_object_data_get(obj, "ad");
	}
	MP_CHECK(ad);

	char *playlist_name = NULL;
	int item_count = 0;
	ret = mp_media_info_group_get_main_info(item_handler, &playlist_name);
	ret = mp_media_info_group_get_playlist_id(item_handler, &playlist_id);

	mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, playlist_id, &item_count);

#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
	if (item_count > MP_PLAYLIST_MAX_ITEM_COUNT)
	{
		char *fmt_str = GET_STR("IDS_MUSIC_POP_UNABLE_TO_ADD_MORE_THAN_PD_MUSIC_FILE");
		char *noti_str = g_strdup_printf(fmt_str, MP_PLAYLIST_MAX_ITEM_COUNT);
		mp_util_post_status_message(ad, noti_str);
		IF_FREE(noti_str);
		goto END;
	}
#endif

	mp_retm_if(ret != 0, "Fail to get value");

	bool result = false;

	if (category == MP_LAYOUT_TRACK_LIST)
	{
		char *fid = (char *)evas_object_data_get(obj, MP_MENU_FID);
		result = mp_util_add_to_playlist_by_key(playlist_id, fid);
		IF_FREE(fid);
	}
	else if (category == MP_LAYOUT_GROUP_LIST)
	{
		mp_media_info_h group_item_handler = evas_object_data_get(obj, MP_MENU_GROUP_ITEM_HANDLER);

		MP_CHECK(group_item_handler);

		result = _mp_menu_func_by_group_handle(playlist_id, layout_data,
						       group_item_handler, MP_MENU_FUNC_ADD_TO_LIST);
	}

	if (result)
	{
		mp_debug("sucess add to play list");

		if(layout_data && layout_data->track_type == MP_TRACK_BY_PLAYLIST && layout_data->category == MP_LAYOUT_TRACK_LIST)
			mp_view_layout_update(obj);

		if (playlist_name)
		{
			mp_util_post_status_message(ad, GET_STR("IDS_MUSIC_POP_ADDED"));
		}
	}
	else
	{
		mp_debug("fail add to play list");
#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
		char *fmt_str = GET_STR("IDS_MUSIC_POP_UNABLE_TO_ADD_MORE_THAN_PD_MUSIC_FILE");
		char *noti_str = g_strdup_printf(fmt_str, MP_PLAYLIST_MAX_ITEM_COUNT);
		mp_util_post_status_message(ad, noti_str);
		IF_FREE(noti_str);
#else
		mp_util_post_status_message(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_ADD"));
#endif
	}

	mp_media_list_h playlists = NULL;

#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
      END:
#endif
	playlists = evas_object_data_get(obj, MP_MENU_POPUP_PLAY_LIST_HANDLER);
	mp_media_info_group_list_destroy(playlists);

	mp_popup_destroy(ad);
	return;

}

static void
_mp_menu_excute_delete_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();

	mp_media_info_h handle = data;
	MP_CHECK(handle);

	struct appdata *ad = NULL;
	char *fid = NULL;
	int ret = 0;

	Evas_Object *popup = obj;
	mp_layout_data_t *layout_data = evas_object_data_get(popup, "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	ad = layout_data->ad;
	ad->popup_delete = NULL;

	mp_evas_object_del(popup);
	int response = (int)event_info;

	if (response == MP_POPUP_YES)
	{
		bool result = TRUE;

		if (layout_data->category == MP_LAYOUT_TRACK_LIST)
		{
			if (layout_data->track_type == MP_TRACK_BY_PLAYLIST)
			{
				int member_id = 0;
				ret = mp_media_info_get_playlist_member_id(handle, &member_id);
				mp_media_info_playlist_remove_media(layout_data->playlist_handle, member_id);
			}
			else if (layout_data->track_type == MP_TRACK_BY_ADDED_TIME)
			{
				ret = mp_media_info_set_added_time(handle, 0);
			}
			else if (layout_data->track_type == MP_TRACK_BY_FAVORITE)
			{
				ret = mp_media_info_set_favorite(handle, false);
			}
			else if (layout_data->track_type == MP_TRACK_BY_PLAYED_TIME)
			{
				ret = mp_media_info_set_played_time(handle, 0);
			}
			else if (layout_data->track_type == MP_TRACK_BY_PLAYED_COUNT)
			{
				ret = mp_media_info_set_played_count(handle, 0);
			}
			else
			{
				char *path = NULL;
				fid = mp_util_get_fid_by_handle(layout_data, handle);
				mp_media_info_get_file_path(handle, &path);
				if (MP_FILE_DELETE_ERR_NONE != mp_util_delete_track(ad, fid, path, FALSE))
					ret = -1;
			}
			if (ret != 0)
				result = false;
			else
				result = true;
		}
		else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
		{
			result = _mp_menu_func_by_group_handle(0, layout_data, handle, MP_MENU_FUNC_DELETE);
		}
		else if (layout_data->category == MP_LAYOUT_PLAYLIST_LIST)
		{
			int plst_id = 0;
			ret = mp_media_info_group_get_playlist_id(handle, &plst_id);
			mp_retm_if(ret != 0, "Fail to get value");

			ret = mp_media_info_playlist_delete_from_db(plst_id);
			mp_retm_if(ret != 0, "Fail to delete playlist");
		}

		if (result)
		{
			Elm_Object_Item *it =
				(Elm_Object_Item *)elm_genlist_decorated_item_get((const Evas_Object *)layout_data->genlist);

			if(!it && layout_data->category == MP_LAYOUT_PLAYLIST_LIST) {
				Elm_Object_Item *item = elm_genlist_first_item_get(layout_data->genlist);
				for(; item != NULL; item = elm_genlist_item_next_get(item)) {
					mp_genlist_item_data_t *item_data = NULL;
					item_data = (mp_genlist_item_data_t *)elm_object_item_data_get(item);
					if(item_data && item_data->handle == handle) {
						it = item;
						break;
					}
				}
			}

			if(it)
			{
				layout_data->item_count--;

				/* update last view when change album track view or artist track view */
				if (MP_TRACK_BY_ALBUM == layout_data->track_type
				    || MP_TRACK_BY_ARTIST == layout_data->track_type) {
					    layout_data->album_delete_flag = TRUE;
					    mp_view_manager_update_list_contents(layout_data->view_data, FALSE);
					    layout_data->album_delete_flag = FALSE;
				}

				//update view in case there is no content. otherwise juse call elm_object_item_del()
				if(layout_data->item_count < 1) {
					mp_view_layout_update(layout_data->layout);
					/* update the first controlba item */
					mp_view_manager_update_first_controlbar_item(layout_data);
				} else {
					if (MP_TRACK_BY_ARTIST == layout_data->track_type) {
						if(!elm_genlist_item_parent_get(elm_genlist_item_next_get(it))
							&& !elm_genlist_item_parent_get(elm_genlist_item_prev_get(it)))
						{
							elm_object_item_del(elm_genlist_item_parent_get(it));
						}
						else
						{
							elm_genlist_item_update(elm_genlist_item_parent_get(it));
						}
					} else if (MP_TRACK_BY_ALBUM == layout_data->track_type) {
						/* update group title */
						elm_genlist_item_update(layout_data->album_group);
					}

					elm_object_item_del(it);
				}
			}
			MP_CHECK(layout_data->view_data);
			mp_util_post_status_message(layout_data->ad, GET_SYS_STR("IDS_COM_POP_DELETED"));
		}
		else
			mp_widget_text_popup(ad, GET_SYS_STR("IDS_COM_POP_FAILED"));

	}
}

#endif

int
_mp_menu_set_isf_entry(mp_layout_data_t * layout_data)
{

	char *init_str = NULL;

	init_str = elm_entry_utf8_to_markup(layout_data->old_name);


	if (init_str)
	{
		elm_entry_entry_set(layout_data->isf_entry, init_str);
		g_free(init_str);
		init_str = NULL;
	}
	else
	{
		DEBUG_TRACE("+++++++ the init str for entry is empty!");
		elm_entry_entry_set(layout_data->isf_entry, "");
	}
	return 0;
}

void
mp_menu_share_list_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	MP_CHECK(data);
	CtxPopup_Data *popup_data = data;
	void *user_data = popup_data->user_data;
	MP_CHECK(user_data);
	const char *label = popup_data->label;

	mp_layout_data_t *layout_data = (mp_layout_data_t *) user_data;

	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	if (label)
	{
		GString *path = NULL;
		char *path_name = NULL;
		char *fmt = NULL;

		if (g_strcmp0(label, BLUETOOTH_SYS) == 0)
			fmt = "?%s";
		else if ( !g_strcmp0(label, EMAIL_SYS) ||!g_strcmp0(label, MESSAGE_SYS))
			fmt = "\n%s";
#ifdef MP_FEATURE_WIFI_SHARE
		else if (g_strcmp0(label, WIFI_SYS) == 0)
			fmt = "|%s";
#endif
		else
		{
			ERROR_TRACE("not available");
			return;
		}

		mp_retm_if(layout_data->genlist == NULL, "genlist is NULL");

		Elm_Object_Item *first_item = elm_genlist_first_item_get(layout_data->genlist);
		MP_CHECK(first_item);

		Elm_Object_Item *current_item = NULL;
		Elm_Object_Item *next_item = NULL;
		mp_genlist_item_data_t *gl_item = NULL;
		int i = 0;

		if (layout_data->checked_count < 1)
		{
			ERROR_TRACE("There is no seleted Item");
			return;
		}

		MP_GENLIST_CHECK_FOREACH_SAFE(first_item, current_item, next_item, gl_item)
		{
			if(elm_genlist_item_select_mode_get(current_item) == ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
				continue;

			if (gl_item->checked)
			{
				mp_media_info_h item = (gl_item->handle);
				MP_CHECK(item);

				if (item)
				{

					int ret = 0;
					ret = mp_media_info_get_file_path(item, &path_name);
					if (ret != 0)
					{
						DEBUG_TRACE("Fail to get file path... ");
						continue;
					}

					if (path_name)
					{
						if (path == NULL)
						{
							path = g_string_new(path_name);
						}
						else
						{
							g_string_append_printf(path, fmt, path_name);
						}
					}
					else
					{
						ERROR_TRACE("path name is NULL");
						continue;
					}


				}
				else
				{
					ERROR_TRACE("item_data is NULL");
					return;
				}
				i++;
			}
		}

		if (path && path->str)
		{
			DEBUG_TRACE("path is [%s]", path->str);

			if (g_strcmp0(label, EMAIL_SYS) == 0)
					mp_ug_email_attatch_file(path->str, layout_data->ad);
			else if(g_strcmp0(label, MESSAGE_SYS) == 0)
					mp_ug_message_attatch_file(path->str, layout_data->ad);
			else if(g_strcmp0(label, BLUETOOTH_SYS) == 0)
				mp_ug_bt_attatch_file(path->str, i, layout_data->ad);
#ifdef MP_FEATURE_WIFI_SHARE
			else if(g_strcmp0(label, WIFI_SYS) == 0)
				mp_ug_wifi_attatch_file(path->str, i, layout_data->ad);
#endif
			g_string_free(path, TRUE);
		}
		else
		{
			ERROR_TRACE("path is NULL");
		}
	}

	mp_ctxpopup_destroy(popup_data->popup);
}

void
mp_menu_share_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	char *path = NULL;	//do not free
	struct appdata *ad = NULL;
	mp_plst_item *item = NULL;

	MP_CHECK(data);
	CtxPopup_Data *popup_data = data;
	void *user_data = popup_data->user_data;
	MP_CHECK(user_data);
	const char *label = popup_data->label;

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");

	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		mp_media_info_h handle = NULL;
		handle = user_data;
		ad = layout_data->ad;
		path = mp_util_get_path_by_handle(layout_data, handle);
		MP_CHECK(path);

		mp_debug("path =%s", path);
	}
	else
	{
		ad = user_data;
		MP_CHECK(ad);

		item = mp_playlist_mgr_get_current(ad->playlist_mgr);
		MP_CHECK(item);

		path = item->uri;
	}

	if (label)
	{
		DEBUG_TRACE("%s selected", label);

		if (g_strcmp0(label, BLUETOOTH_SYS) == 0)
		{
			mp_ug_bt_attatch_file(path, 1, ad);
		}
		else if (g_strcmp0(label, EMAIL_SYS) == 0)
		{
			mp_ug_email_attatch_file(path, ad);
		}
		else if (g_strcmp0(label, MESSAGE_SYS) == 0)
			mp_ug_message_attatch_file(path, ad);
#ifdef MP_FEATURE_WIFI_SHARE
		else if (g_strcmp0(label, WIFI_SYS) == 0)
			mp_ug_wifi_attatch_file(path, 1, ad);
#endif
	}

	mp_ctxpopup_destroy(popup_data->popup);
}

static int
_mp_menu_set_caller_rington(char *path)
{
	int ret = -1;
	bool is_drm = FALSE;
	char *prev_ring_tone_path = NULL;

	//drm check..
	prev_ring_tone_path = vconf_get_str(VCONFKEY_SETAPPL_CALL_RINGTONE_PATH_STR);
	if (prev_ring_tone_path && mp_drm_file_right(prev_ring_tone_path))
	{
		mp_drm_request_setas_ringtone(prev_ring_tone_path, SETAS_REQUEST_UNREGISTER);
		free(prev_ring_tone_path);
	}

	if (mp_drm_file_right(path))
	{
		if (!mp_drm_request_setas_ringtone(path, SETAS_REQUEST_CHECK_STATUS))
			return -1;
		is_drm = true;
	}

	ret = system_settings_set_value_string(SYSTEM_SETTINGS_KEY_INCOMING_CALL_RINGTONE, path);
	if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
		mp_error("system_settings_set_value_string()... [0x%x]", ret);
		return -1;
	}

	if (is_drm)
		mp_drm_request_setas_ringtone(path, SETAS_REQUEST_REGISTER);

	return ret;

}

void
mp_menu_set_as_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	int ret = 0;
	char *path = NULL;	//do not free
	struct appdata *ad = NULL;
	mp_plst_item *item = NULL;

	MP_CHECK(data);
	CtxPopup_Data *popup_data = data;
	void *user_data = popup_data->user_data;
	MP_CHECK(user_data);
	const char *label = popup_data->label;

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");

	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		mp_media_info_h handle = NULL;
		handle = user_data;
		ad = layout_data->ad;
		path = mp_util_get_path_by_handle(layout_data, handle);
	}
	else
	{
		ad = user_data;
		MP_CHECK(ad);

		item = mp_playlist_mgr_get_current(ad->playlist_mgr);
		MP_CHECK(item);

		path = item->uri;
	}

	if (label)
	{
		DEBUG_TRACE("%s selected", label);
		if (g_strcmp0(label, GET_STR(CALLER_RINGTONE)) == 0)
		{
			mp_ug_contact_user_sel(path, ad);
		}
		else if (g_strcmp0(label, GET_STR(CALL_RINGTONE)) == 0)
		{
			char *popup_txt = NULL;

			DEBUG_TRACE("path =%s", path);

			ret = _mp_menu_set_caller_rington(path);

			if (!ret)
				popup_txt = GET_SYS_STR("IDS_COM_POP_SUCCESS");
			else
				popup_txt = GET_SYS_STR("IDS_COM_POP_FAILED");

			mp_widget_text_popup(ad, popup_txt);

		}
	}

	mp_ctxpopup_destroy(popup_data->popup);

	return;
}

static void
_mp_menu_ctx_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DEBUG_TRACE("");
	struct appdata *ad = data;
	MP_CHECK(ad);

	ad->ctx_popup = NULL;
}

void
mp_menu_share_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");

	if (data == NULL)
		return;
	struct appdata *ad = NULL;

	Evas_Object *share_popup = NULL;

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");

	char *file_name = NULL;
	Evas_Object *genlist = NULL;

	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");
		ad = layout_data->ad;

		mp_media_info_h handle = NULL;
		handle = data;
		ad = layout_data->ad;
		file_name = mp_util_get_path_by_handle(layout_data, handle);

		if (mp_drm_check_foward_lock(file_name))
		{
			mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_SHARE_DRM_FILE"));
			return;
		}

		share_popup = mp_ctxpopup_create(obj, MP_CTXPOPUP_PV_SHARE, data, ad);
		MP_CHECK(share_popup);
		evas_object_data_set(share_popup, "layout_data", layout_data);

	}
	else
	{
		ad = data;
		mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");
		mp_plst_item *item = mp_playlist_mgr_get_current(ad->playlist_mgr);

		MP_CHECK(item);

		mp_debug("excuete by list view %s", item->uri);

		if (mp_drm_check_foward_lock(item->uri))
		{
			mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_SHARE_DRM_FILE"));
			return;
		}

		share_popup = mp_ctxpopup_create(obj, MP_POPUP_PV_SHARE, data, ad);
		MP_CHECK(share_popup);
	}

	ad->ctx_popup = share_popup;
	evas_object_event_callback_add(ad->ctx_popup, EVAS_CALLBACK_DEL, _mp_menu_ctx_del_cb, ad);

	return;

}

void
mp_menu_set_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");

	if (data == NULL)
		return;
	struct appdata *ad = NULL;

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");

	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		ad = layout_data->ad;
	}
	else
		ad = data;

	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");

	Evas_Object *popup;

	if (layout_data)
		popup = mp_ctxpopup_create(obj, MP_CTXPOPUP_PV_SET_AS, data, ad);
	else
		popup = mp_ctxpopup_create(obj, MP_CTXPOPUP_PV_SET_AS_INCLUDE_ADD_TO_HOME, data, ad);

	if (layout_data)
	{
		evas_object_data_set(popup, "layout_data", layout_data);
	}

	ad->ctx_popup = popup;
	evas_object_event_callback_add(ad->ctx_popup, EVAS_CALLBACK_DEL, _mp_menu_ctx_del_cb, ad);

	return;

}

#ifndef MP_SOUND_PLAYER
void
mp_menu_delete_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();

	struct appdata *ad = NULL;
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");


	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	mp_media_info_h handle = NULL;
	handle = data;

	ad = layout_data->ad;

	Evas_Object *popup = mp_popup_create(ad->win_main, MP_POPUP_NORMAL, NULL, handle, _mp_menu_excute_delete_cb, ad);
	ad->popup_delete = popup;
	evas_object_data_set(popup, "layout_data", layout_data);

	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_object_text_set(popup, GET_SYS_STR("IDS_COM_POP_DELETE_Q"));

	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_BODY_DELETE"), MP_POPUP_YES);
	mp_popup_button_set(popup, MP_POPUP_BTN_2, GET_SYS_STR("IDS_COM_SK_CANCEL"), MP_POPUP_NO);

	evas_object_show(popup);
}

void
mp_menu_add_to_playlist_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();

	int ret = 0;

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_plst_item *item = NULL;

	char *fid = NULL;

	struct appdata *ad = NULL;

	mp_media_info_h handle = NULL;

	Evas_Object *popup = NULL;

	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		ad = layout_data->ad;
	}
	else
		ad = data;

	MP_CHECK_EXCEP(ad);

	popup = mp_genlist_popup_create(obj, MP_POPUP_ADD_TO_PLST, ad, ad);
	MP_CHECK(popup);

	Evas_Object *genlist = evas_object_data_get(popup, "genlist");
	MP_CHECK(genlist);

	if (layout_data)
	{
		evas_object_data_set(genlist, "layout_data", layout_data);

		handle = data;

		if (layout_data->category == MP_LAYOUT_TRACK_LIST)
		{
			fid = g_strdup(mp_util_get_fid_by_handle(layout_data, handle));
			evas_object_data_set(genlist, MP_MENU_FID, (void *)fid);	//set request fid
		}
		else
		{
			evas_object_data_set(genlist, MP_MENU_GROUP_ITEM_HANDLER, handle);	//set group item handler
		}
	}
	else
	{
		item = mp_playlist_mgr_get_current(ad->playlist_mgr);
		MP_CHECK_EXCEP(item);
		fid = g_strdup(item->uid);
		evas_object_data_set(genlist, MP_MENU_FID, (void *)fid);	//set request fid
		evas_object_data_set(genlist, "ad", ad);
	}

	int i = 0, count = -1, err = -1;

	mp_genlist_popup_item_append(popup, GET_STR("IDS_MUSIC_OPT_CREATE_PLAYLIST"), NULL,
				     _mp_menu_add_playlist_create_select_cb, ad);

	err = mp_media_info_group_list_count(MP_GROUP_BY_PLAYLIST, NULL, NULL, &count);

	if ((err != 0) || (count < 0))
	{
		ERROR_TRACE("Error in mp_media_info_group_list_count (%d)\n", err);
		return;
	}

	if (count)
	{
		mp_media_list_h playlists = NULL;	//must be free

		ret = mp_media_info_group_list_create(&playlists, MP_GROUP_BY_PLAYLIST, NULL, NULL, 0, count);
		mp_retm_if(ret != 0, "Fail to get playlist");
		evas_object_data_set(popup, MP_MENU_POPUP_PLAY_LIST_HANDLER, (void *)playlists);	//set request handle id for support group item

		for (i = 0; i < count; i++)
		{
			/* it should be released in a proper place */
			mp_media_info_h plst = NULL;
			char *name = NULL;
			plst = mp_media_info_group_list_nth_item(playlists, i);
			mp_retm_if(!plst, "Fail to get item");

			ret = mp_media_info_group_get_main_info(plst, &name);
			mp_retm_if(ret != 0, "Fail to get value");

			mp_genlist_popup_item_append(popup, name, NULL, _mp_menu_add_playlist_select_cb, (void *)plst);
		}

	}

	evas_object_show(popup);

	return;

      mp_exception:
	mp_evas_object_del(popup);
	return;
}
#endif

