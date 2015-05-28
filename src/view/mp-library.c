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
#include "mp-setting-ctrl.h"
#include "mp-player-debug.h"
#include "mp-common.h"
#include "music.h"
#include "mp-track-view.h"
#include "mp-group-view.h"
#include "mp-playlist-view.h"
#include "mp-play-view.h"
#include "mp-util.h"
#include "mp-widget.h"



static void _mp_library_init(void *data);
static Evas_Object *_mp_library_create_layout_main(Evas_Object * parent);
static Evas_Object *_mp_library_create_tabbar(struct appdata *ad);
static void _mp_library_controlbar_item_append(struct appdata *ad);
static bool _mp_library_check_request_item(struct appdata *ad);

static void
_mp_library_init(void *data)
{
	struct appdata *ad = data;

	ad->library = calloc(1, sizeof(mp_library));
	if (!ad->library)
	{
		ERROR_TRACE("failed to create library object\n");
	}
}

static Evas_Object *
_mp_library_create_layout_main(Evas_Object * parent)
{
	Evas_Object *layout;

	mp_retv_if(parent == NULL, NULL);
	layout = elm_layout_add(parent);
	mp_retvm_if(layout == NULL, NULL, "Failed elm_layout_add.\n");

	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	evas_object_show(layout);

	return layout;
}

static Evas_Object *
_mp_library_create_tabbar(struct appdata *ad)
{
	Evas_Object *obj;

	/* create controlbar */
	TA_S(3, "elm_toolbar_add");
	obj = elm_toolbar_add(ad->naviframe);
	TA_E(3, "elm_toolbar_add");

	elm_toolbar_transverse_expanded_set(obj, EINA_TRUE);
	elm_toolbar_shrink_mode_set(obj, ELM_TOOLBAR_SHRINK_EXPAND);
	elm_toolbar_select_mode_set(obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
	elm_object_style_set(obj, "tabbar");

	mp_retvm_if(obj == NULL, NULL, "Fail to create control bar");
	ad->tabbar = obj;

	TA_S(3, "_mp_library_controlbar_item_append");
	_mp_library_controlbar_item_append(ad);
	TA_E(3, "_mp_library_controlbar_item_append");
	TA_S(3, "evas_object_show");
	evas_object_show(obj);
	TA_E(3, "evas_object_show");

	return obj;
}

// update menu when menu changed while bgm.
static void
_mp_library_menu_changed(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	MP_CHECK(ad->library->ctltab_songs);
	MP_CHECK(ad->tabbar);

	DEBUG_TRACE("loadtype=%d", ad->loadtype);
	if (ad->loadtype == LOAD_TRACK)
	{
		DEBUG_TRACE("Don't need to update menu...");
		return;
	}

	if (ad->setting_ug) {
		mp_debug("setting ug on playing view.. reserve menu change");
		ad->music_setting_change_flag = true;
		return;
	}

	mp_util_set_library_controlbar_items(ad);
}


static bool
_mp_library_append_item_idler_cb( void *data)
{
	startfunc;
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);
	Evas_Object *obj;
	obj = ad->tabbar;

	TA_S(4, "elm_toolbar_item_append-ctltab_genres");
	{
		ad->library->ctltab_genres =
			elm_toolbar_item_append(obj, MP_CTRBAR_ICON_GENRE, GET_STR(STR_MP_GENRES), mp_library_view_change_cb, ad);
		mp_language_mgr_register_object_item( ad->library->ctltab_genres, STR_MP_GENRES);
		DEBUG_TRACE("library->ctltab_genres: 0x%x", ad->library->ctltab_genres);
	}
	TA_E(4, "elm_toolbar_item_append-ctltab_genres");
	TA_S(4, "elm_toolbar_item_append-ctltab_composer");
	{
		ad->library->ctltab_composer =
			elm_toolbar_item_append(obj, MP_CTRBAR_ICON_COMPOSER, GET_STR(STR_MP_COMPOSERS), mp_library_view_change_cb, ad);
		mp_language_mgr_register_object_item( ad->library->ctltab_composer,STR_MP_COMPOSERS);
		DEBUG_TRACE("library->ctltab_composer: 0x%x", ad->library->ctltab_composer);
	}
	TA_E(4, "elm_toolbar_item_append-ctltab_composer");
	TA_S(4, "elm_toolbar_item_append-ctltab_year");
	{
		ad->library->ctltab_year =
			elm_toolbar_item_append(obj, MP_CTRBAR_ICON_YEAR, GET_STR(STR_MP_YEARS), mp_library_view_change_cb, ad);
		mp_language_mgr_register_object_item( ad->library->ctltab_year, STR_MP_YEARS);
		DEBUG_TRACE("library->ctltab_year: 0x%x", ad->library->ctltab_year);
	}
	TA_E(4, "elm_toolbar_item_append-ctltab_year");
	TA_S(4, "elm_toolbar_item_append-ctltab_folder");
	{
		ad->library->ctltab_folder =
			elm_toolbar_item_append(obj, MP_CTRBAR_ICON_FOLDER, GET_SYS_STR(STR_MP_FOLDERS), mp_library_view_change_cb, ad);
		mp_language_mgr_register_object_item(ad->library->ctltab_folder, STR_MP_FOLDERS);
		DEBUG_TRACE("library->ctltab_folder: 0x%x", ad->library->ctltab_folder);
	}
	TA_E(4, "elm_toolbar_item_append-ctltab_folder");


	return EINA_FALSE;
}

static void
_mp_library_controlbar_item_append(struct appdata *ad)
{
	startfunc;
	MP_CHECK(ad);
	MP_CHECK(ad->library);

	Evas_Object *obj;

	obj = ad->tabbar;

	ad->library->first_append = true;
	TA_S(4, "elm_toolbar_item_append-ctltab_songs");
	ad->library->ctltab_songs =
		elm_toolbar_item_append(obj, MP_CTRBAR_ICON_SONGS, GET_STR(STR_MP_ALL_TRACKS), mp_library_view_change_cb, ad);
	TA_E(4, "elm_toolbar_item_append-ctltab_songs");
	mp_language_mgr_register_object_item(ad->library->ctltab_songs, STR_MP_ALL_TRACKS);
	DEBUG_TRACE("library->ctltab_songs: 0x%x", ad->library->ctltab_songs);

	ad->library->first_append = false;
	TA_S(4, "elm_toolbar_item_append-ctltab_plist");
	ad->library->ctltab_plist =
		elm_toolbar_item_append(obj, MP_CTRBAR_ICON_PLAYLIST, GET_STR(STR_MP_PLAYLISTS), mp_library_view_change_cb, ad);
	TA_E(4, "elm_toolbar_item_append-ctltab_plist");
	mp_language_mgr_register_object_item( ad->library->ctltab_plist,STR_MP_PLAYLISTS);
	DEBUG_TRACE("library->ctltab_plist: 0x%x", ad->library->ctltab_plist);

	TA_S(4, "elm_toolbar_item_append-ctltab_album");
	{
		ad->library->ctltab_album =
			elm_toolbar_item_append(obj, MP_CTRBAR_ICON_ALBUM, GET_STR(STR_MP_ALBUMS), mp_library_view_change_cb, ad);
		mp_language_mgr_register_object_item( ad->library->ctltab_album, STR_MP_ALBUMS);
		DEBUG_TRACE("library->ctltab_album: 0x%x", ad->library->ctltab_album);
	}
	TA_E(4, "elm_toolbar_item_append-ctltab_album");
	TA_S(4, "elm_toolbar_item_append-ctltab_artist");
	{
		ad->library->ctltab_artist =
			elm_toolbar_item_append(obj, MP_CTRBAR_ICON_ARTIST, GET_STR(STR_MP_ARTISTS), mp_library_view_change_cb, ad);
		mp_language_mgr_register_object_item( ad->library->ctltab_artist, STR_MP_ARTISTS);
		DEBUG_TRACE("library->ctltab_artist: 0x%x", ad->library->ctltab_artist);
	}
	TA_E(4, "elm_toolbar_item_append-ctltab_artist");

	ecore_idler_add(_mp_library_append_item_idler_cb, ad);

	endfunc;
}

/**
 * load view  use for _mp_library_view_change_cb
 *
 * @param void *data, Evas_Object* navi_bar
 * @return FALSE or TRUE if it success create
 * @author aramie.kim@samsung.com
 */

static bool
_mp_library_load_request_view(void *data, Evas_Object * navi_bar)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	DEBUG_TRACE("ad->request_play_id: %d", ad->request_play_id);
	MP_CHECK_FALSE(ad);

	if (!_mp_library_check_request_item(ad))
		return false;

	if (ad->request_group_name)
	{
		mp_debug("group name = %s", ad->request_group_name);
		mp_group_view_create_by_group_name(navi_bar, ad->request_group_name, ad->request_group_type);
		IF_FREE(ad->request_group_name);
		ad->update_default_view = TRUE;
	}
	else if (ad->request_play_id > 0)
	{
		mp_debug("play list id = %d", ad->request_play_id);
		mp_playlist_view_create_by_id(navi_bar, ad->request_play_id);
		ad->request_play_id = MP_SYS_PLST_NONE;
		ad->update_default_view = TRUE;
	}
	else if (ad->request_play_id == MP_SYS_PLST_MOST_PLAYED)
	{
		mp_playlist_view_create_auto_playlist(ad, STR_MP_MOST_PLAYED);
		ad->request_play_id = MP_SYS_PLST_NONE;
		ad->update_default_view = TRUE;
	}
	else if (ad->request_play_id == MP_SYS_PLST_RECENTELY_ADDED)
	{
		mp_playlist_view_create_auto_playlist(ad, STR_MP_RECENTLY_ADDED);
		ad->request_play_id = MP_SYS_PLST_NONE;
		ad->update_default_view = TRUE;
	}
	else if (ad->request_play_id == MP_SYS_PLST_RECENTELY_PLAYED)
	{
		mp_playlist_view_create_auto_playlist(ad, STR_MP_RECENTLY_PLAYED);
		ad->request_play_id = MP_SYS_PLST_NONE;
		ad->update_default_view = TRUE;
	}
	else
	{
		mp_debug("That is not request mode ");
		return FALSE;
	}

	endfunc;

	return true;
}

static Elm_Object_Item *
_mp_library_get_request_control_item(struct appdata *ad)
{
	Elm_Object_Item *select_request_item = NULL;

	if (ad->loadtype == LOAD_DEFAULT)
	{
		select_request_item = ad->library->ctltab_songs;
	}
	else if (ad->loadtype == LOAD_PLAYLIST)
	{
		select_request_item = ad->library->ctltab_plist;
	}
	else if (ad->loadtype == LOAD_GROUP)
	{
		if (ad->request_group_type == MP_GROUP_BY_ARTIST)
		{
			select_request_item = ad->library->ctltab_artist;
		}
		else if (ad->request_group_type == MP_GROUP_BY_ALBUM)
		{
			select_request_item = ad->library->ctltab_album;
		}
		else if (ad->request_group_type == MP_GROUP_BY_GENRE)
		{
			select_request_item = ad->library->ctltab_genres;
		}
		else if (ad->request_group_type == MP_GROUP_BY_COMPOSER)
		{
			select_request_item = ad->library->ctltab_composer;
		}
		else if (ad->request_group_type == MP_GROUP_BY_YEAR)
		{
			select_request_item = ad->library->ctltab_year;
		}
		else if (ad->request_group_type == MP_GROUP_BY_FOLDER)
		{
			select_request_item = ad->library->ctltab_folder;
		}
		else
		{
			mp_error("can not support that value");
		}
	}

	return select_request_item;
}


static void
_mp_library_show_selected_view(struct appdata *ad, Elm_Object_Item *it)
{
	MP_CHECK(ad);
	MP_CHECK(it);

	Evas_Object *content = NULL;
	view_data_t *view_data = NULL;
	mp_layout_data_t *layout_data = NULL;
	mp_library *library = NULL;

	library = ad->library;
	MP_CHECK(library);

	DEBUG_TRACE("selected view: %s", elm_object_item_text_get(it));

	content = elm_object_part_content_unset(ad->controlbar_layout, "elm.swallow.content");

	if (content)
	{
		DEBUG_TRACE("destory previous view");
		mp_view_manager_unswallow_info_ug_layout(ad);
		evas_object_del(content);
		content = NULL;
	}

	if (it == library->ctltab_album)
	{
		content = mp_group_view_create(ad, MP_VIEW_TYPE_ALBUM);
	}
	else if (it == library->ctltab_artist)
	{
		content = mp_group_view_create(ad, MP_VIEW_TYPE_ARTIST);
	}
	else if (it == library->ctltab_genres)
	{
		content = mp_group_view_create(ad, MP_VIEW_TYPE_GENRE);
	}
	else if (it == library->ctltab_year)
	{
		content = mp_group_view_create(ad, MP_VIEW_TYPE_YEAR);
	}
	else if (it == library->ctltab_composer)
	{
		content = mp_group_view_create(ad, MP_VIEW_TYPE_COMPOSER);
	}
	else if (it == library->ctltab_folder)
	{
		content = mp_group_view_create(ad, MP_VIEW_TYPE_FOLDER);
	}

	else if (it == library->ctltab_plist)
	{
		content = mp_playlist_view_create(ad, MP_VIEW_TYPE_PLAYLIST);
	}

	else
	{
		TA_S(5, "mp_track_view_create");
		content = mp_track_view_create(ad);
		TA_E(5, "mp_track_view_create");
		TA_S(5, "mp_track_view_update_title_button");
		mp_track_view_update_title_button(ad->naviframe);
		TA_E(5, "mp_track_view_update_title_button");
	}

	MP_CHECK(content);

	layout_data = evas_object_data_get(content, "layout_data");
	MP_CHECK(layout_data);

	if (it != library->ctltab_songs)
	{
		ad->b_add_tracks = FALSE;
	}

	elm_object_part_content_set(ad->controlbar_layout, "elm.swallow.content", content);
	TA_S(5, "mp_view_layout_update");
	mp_view_layout_update(content);
	TA_E(5, "mp_view_layout_update");

	view_data = (view_data_t *) evas_object_data_get(ad->naviframe, "view_data");
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	if (!_mp_library_load_request_view(ad, ad->naviframe))
	{
		DEBUG_TRACE("Update default layout...");
		if (view_data->view_type == MP_VIEW_TYPE_PLAYLIST)
		{
			mp_playlist_view_update_navibutton(layout_data);
		}
	}


	evas_object_show(content);
}

void mp_library_view_change_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	Elm_Object_Item *it;
	view_data_t *view_data = mp_util_get_view_data(ad);

	MP_CHECK(ad);
	DEBUG_TRACE_FUNC();

	if (ad->library->first_append && ad->launch_type == MP_LAUNCH_ADD_TO_HOME && ad->loadtype != LOAD_TRACK) {
		WARN_TRACE("add to home.. first view change");
		ad->library->first_append = false;
		return;
	}

	if(mp_view_manager_count_view_content(view_data) > 1)
	{
		DEBUG_TRACE("pop to first view");
		mp_view_manager_pop_view_content(view_data, true, true);
	}
	it = elm_toolbar_selected_item_get(obj);
	DEBUG_TRACE("selected toolbar item: 0x%x", it);
	mp_retm_if(it == NULL, "tab item is NULL");

	_mp_library_show_selected_view(ad, it);

	ad->navi_effect_in_progress = false;
}


static bool
_mp_library_check_request_item(struct appdata *ad)
{
	startfunc;

	char *popup_text = NULL;
	int count = -1;
	int ret = 0;
	MP_CHECK_FALSE(ad);

	if (!ad->request_group_name && ad->request_play_id > 0)
	{
		int i = 0;
		mp_media_list_h list = NULL;

		ret = mp_media_info_group_list_count(MP_GROUP_BY_PLAYLIST, NULL, NULL, &count);
		MP_CHECK_FALSE(ret == 0);

		ret = mp_media_info_group_list_create(&list, MP_GROUP_BY_PLAYLIST, NULL, NULL, 0, count);
		MP_CHECK_FALSE(ret == 0);

		for(i=0; i<count; i++)
		{
			int playlist_id;
			mp_media_info_h media = NULL;
			media = mp_media_info_group_list_nth_item(list, i);
			mp_media_info_group_get_playlist_id(media, &playlist_id);
			if(playlist_id == ad->request_play_id)
			{
				break;
			}
		}

		if (i == count)
		{
			IF_FREE(ad->request_group_name);
			ad->request_play_id = MP_SYS_PLST_NONE;
			mp_error("removed play list");
			popup_text = g_strdup(GET_STR("IDS_MUSIC_BODY_NO_PLAYLISTS"));
		}
	}
	else if (ad->request_group_name)
	{
		mp_track_type_e list_type = MP_TRACK_ALL;

		switch (ad->request_group_type)
		{
		case MP_GROUP_BY_ALBUM:
			DEBUG_TRACE("create album detail view");
			list_type = MP_TRACK_BY_ALBUM;
			break;
		case MP_GROUP_BY_ARTIST:
			DEBUG_TRACE("create artist detail view");
			list_type = MP_TRACK_BY_ARTIST;
			break;
		case MP_GROUP_BY_GENRE:
			DEBUG_TRACE("create genre detail view");
			list_type = MP_TRACK_BY_GENRE;
			break;
		case MP_GROUP_BY_YEAR:
			DEBUG_TRACE("create year detail view");
			list_type = MP_TRACK_BY_YEAR;
			break;
		case MP_GROUP_BY_COMPOSER:
			DEBUG_TRACE("create composer detail view");
			list_type = MP_TRACK_BY_COMPOSER;
			break;
		case MP_GROUP_BY_FOLDER:
			DEBUG_TRACE("create folder detail view");
			list_type = MP_TRACK_BY_FOLDER;
			break;
		default:
			break;
		}

		ret = mp_media_info_list_count(list_type, ad->request_group_name, NULL, NULL, 0, &count);
		mp_debug("group cout %d", count);

		if (!(ret == 0 && count > 0))
		{
			mp_error("removed group");
			char *fmt = GET_STR("IDS_MUSIC_POP_PS_REMOVED");
			popup_text = g_strdup_printf(fmt, ad->request_group_name);
			IF_FREE(ad->request_group_name);
		}

	}
	else if (ad->request_playing_path)
	{
		if (mp_util_check_uri_available(ad->request_playing_path))
		{
			mp_debug("http uri path");

		} else if (!mp_check_file_exist(ad->request_playing_path))
		{
			mp_error("removed file");
			IF_FREE(ad->request_playing_path);
			popup_text = g_strdup_printf(GET_SYS_STR("IDS_COM_POP_FILE_NOT_EXIST"));
		}
	}

	if (popup_text)
	{
		mp_widget_text_popup(ad, popup_text);
		free(popup_text);
		return false;
	}

	return true;
}

static Elm_Object_Item *
_mp_library_get_control_tab_item(mp_library *library, mp_group_type_e group_type)
{
	MP_CHECK_NULL(library);
	Elm_Object_Item *it = NULL;
	if (group_type == MP_GROUP_BY_ARTIST)
	{
		it = library->ctltab_artist;
	}
	else if (group_type == MP_GROUP_BY_ALBUM)
	{
		it = library->ctltab_album;
	}
	else if (group_type == MP_GROUP_BY_GENRE)
	{
		it = library->ctltab_genres;
	}
	else if (group_type == MP_GROUP_BY_COMPOSER)
	{
		it = library->ctltab_composer;
	}
	else if (group_type == MP_GROUP_BY_YEAR)
	{
		it = library->ctltab_year;
	}
	else if (group_type == MP_GROUP_BY_FOLDER)
	{
		it = library->ctltab_folder;
	}
	else
	{
		mp_error("can not support that value");
	}
	return it;
}

/**
 * load library use for add to home case
 *
 * @param struct appdata *ad, char *path, int fid
 * @return FALSE or TRUE if it success create
 * @author aramie.kim@samsung.com
 */

bool
mp_library_load(struct appdata * ad)
{
	startfunc;

	MP_CHECK_FALSE(ad);

	if (!ad->base_layout_main)
	{
		mp_library_create(ad);
	}
	else
	{
		//Select current tab
		Elm_Object_Item *selected_item = NULL;
		Elm_Object_Item *select_request_item = NULL;

		if (!ad->tabbar)
			_mp_library_create_tabbar(ad);
		else
			selected_item = elm_toolbar_selected_item_get(ad->tabbar);

		select_request_item = _mp_library_get_request_control_item(ad);

		if (selected_item && selected_item == select_request_item)
		{
			view_data_t *view_data = NULL;

			if (ad->naviframe)
			{
				//clear all item
				view_data = evas_object_data_get(ad->naviframe, "view_data");
				MP_CHECK_FALSE(view_data);
				MP_CHECK_VIEW_DATA(view_data);
				mp_view_manager_pop_view_content(view_data, TRUE, TRUE);

				_mp_library_load_request_view(ad, ad->naviframe);
			}
			else
				mp_error("invalid cur_view");
		}
		else if (select_request_item)
		{
			elm_toolbar_item_selected_set(select_request_item, EINA_TRUE);
		}
		else
		{
			DEBUG_TRACE("Tab is not exist for selected category...");
			if (ad->loadtype == LOAD_GROUP)
			{
				elm_toolbar_item_selected_set(ad->library->ctltab_songs, EINA_TRUE);
			}
			else
				ERROR_TRACE("It shouldn't be here..");
		}
	}

	endfunc;

	return true;
}


void
mp_library_create(struct appdata *ad)
{
	startfunc;

	Evas_Object *tabbar = NULL;
	view_data_t *view_data = NULL;

	//init
	_mp_library_init(ad);

	//create layout_main
	TA_S(2, "_mp_library_create_layout_main");
	ad->base_layout_main = _mp_library_create_layout_main(ad->conformant);
	TA_E(2, "_mp_library_create_layout_main");
	mp_retm_if(ad->base_layout_main == NULL, "library view layout is not initialized");
	elm_object_content_set(ad->conformant, ad->base_layout_main);

	TA_S(2, "mp_widget_navigation_new");
	ad->naviframe = mp_widget_navigation_new(ad->base_layout_main, ad);
	TA_E(2, "mp_widget_navigation_new");

	view_data = calloc(sizeof(view_data_t), 1);
	MP_SET_VIEW_DATA_MAGIC(view_data);
	view_data->ad = ad;
	view_data->navibar = ad->naviframe;

	evas_object_data_set(ad->naviframe, "view_data", view_data);

	ad->controlbar_layout = elm_layout_add(ad->base_layout_main);
	MP_CHECK(ad->controlbar_layout);
	elm_layout_file_set(ad->controlbar_layout, EDJ_NAME, "music/tabbar/default");

	TA_S(2, "mp_view_manager_push_view_content");
	mp_view_manager_push_view_content(view_data, ad->controlbar_layout, MP_VIEW_CONTENT_LIST);
	TA_E(2, "mp_view_manager_push_view_content");

	elm_object_part_content_set(ad->base_layout_main, "elm.swallow.content", ad->naviframe);

	//create control tab
	TA_S(2, "_mp_library_create_tabbar");
	tabbar = _mp_library_create_tabbar(ad);
	elm_object_part_content_set(ad->controlbar_layout, "elm.swallow.tabbar", tabbar);
	TA_E(2, "_mp_library_create_tabbar");

	if(ad->loadtype == LOAD_TRACK)
	{
		DEBUG_TRACE("Don't need to select control tab..");
		return;
	}

	Elm_Object_Item *reqeust_item = _mp_library_get_request_control_item(ad);

	//select tabbar item
	if (reqeust_item)
	{
		TA_S(2, "elm_toolbar_item_selected_set");
		elm_toolbar_item_selected_set(reqeust_item, EINA_TRUE);
		TA_E(2, "elm_toolbar_item_selected_set");
	}
	else
	{
		Elm_Object_Item *it = _mp_library_get_control_tab_item(ad->library, ad->request_group_type);
		if (it)
			_mp_library_show_selected_view(ad, it);
		else
			elm_toolbar_item_selected_set(ad->library->ctltab_songs, EINA_TRUE);
	}

	endfunc;

}

void
mp_library_update_view(struct appdata *ad)
{
	MP_CHECK(ad);
	MP_CHECK(ad->naviframe);

	view_data_t *cur_view_data = evas_object_data_get(ad->naviframe, "view_data");
	MP_CHECK(cur_view_data);

	MP_CHECK_VIEW_DATA(cur_view_data);
	mp_view_manager_update_list_contents(cur_view_data, FALSE);
}

void
mp_library_now_playing_hide(struct appdata *ad)
{
	if (ad->show_now_playing)
	{
		ad->show_now_playing = FALSE;
		view_data_t *view_data = (view_data_t *) evas_object_data_get(ad->naviframe, "view_data");
		MP_CHECK(view_data);
		MP_CHECK_VIEW_DATA(view_data);

		mp_view_manager_set_now_playing(view_data, FALSE);
	}
}

void
mp_library_now_playing_set(struct appdata *ad)
{
	view_data_t *view_data = (view_data_t *) evas_object_data_get(ad->naviframe, "view_data");
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	if (!ad->show_now_playing)
	{
		ad->show_now_playing = TRUE;
	}

	mp_view_manager_set_now_playing(view_data, TRUE);
}


