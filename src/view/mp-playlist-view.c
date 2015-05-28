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
#include "mp-edit-view.h"
#include "mp-playlist-view.h"
#include "mp-util.h"
#include "mp-widget.h"
#include "mp-widget.h"
#include "mp-menu.h"
#include "mp-playlist-mgr.h"
#include "mp-play.h"
#include "mp-play-view.h"

static void _mp_playlist_view_push_item_content(view_data_t * view_data, Evas_Object * content, char *title);
static void _mp_playlist_view_playlist_list_select_cb(void *data, Evas_Object * obj, void *event_info);

static Elm_Genlist_Item_Class *
_mp_playlist_view_get_track_sweep_itc()
{
	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	MP_CHECK_NULL(itc);

	itc->item_style = "3text.1icon.1";
	itc->decorate_item_style = "mode/slide4";
	itc->func.text_get = mp_common_track_list_label_get;
	itc->func.content_get = mp_common_track_list_icon_get;

	return itc;
}

void
mp_playlist_view_create_auto_playlist(struct appdata *ad, char *type)
{
	MP_CHECK(ad);

	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	Evas_Object *view_layout = mp_view_layout_create(ad->naviframe, view_data, MP_VIEW_MODE_DEFAULT);

	mp_genlist_cb_t genlist_cbs;
	memset(&genlist_cbs, 0, sizeof(mp_genlist_cb_t));
	genlist_cbs.selected_cb = mp_common_track_genlist_sel_cb;

	if (!strcmp((STR_MP_MOST_PLAYED), type))
	{
		mp_view_layout_set_layout_data(view_layout,
					       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_TRACK_LIST,
					       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_BY_PLAYED_COUNT,
					       MP_LAYOUT_GENLIST_ITEMCLASS, _mp_playlist_view_get_track_sweep_itc(),
					       MP_LAYOUT_LIST_CB, &genlist_cbs, -1);
	}
	else if (!strcmp((STR_MP_RECENTLY_ADDED), type))
	{
		mp_view_layout_set_layout_data(view_layout,
					       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_TRACK_LIST,
					       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_BY_ADDED_TIME,
					       MP_LAYOUT_GENLIST_ITEMCLASS, _mp_playlist_view_get_track_sweep_itc(),
					       MP_LAYOUT_LIST_CB, &genlist_cbs, -1);
	}
	else if (!strcmp((STR_MP_RECENTLY_PLAYED), type))
	{
		mp_view_layout_set_layout_data(view_layout,
					       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_TRACK_LIST,
					       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_BY_PLAYED_TIME,
					       MP_LAYOUT_GENLIST_ITEMCLASS, _mp_playlist_view_get_track_sweep_itc(),
					       MP_LAYOUT_LIST_CB, &genlist_cbs, -1);
	}
	else if (!strcmp((STR_MP_QUICK_LIST), type))
	{
		mp_view_layout_set_layout_data(view_layout,
					       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_TRACK_LIST,
					       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_BY_FAVORITE,
					       MP_LAYOUT_GENLIST_ITEMCLASS, _mp_playlist_view_get_track_sweep_itc(),
					       MP_LAYOUT_LIST_CB, &genlist_cbs, -1);
	}
	else
	{
		WARN_TRACE("Invalid type: %s", type);
	}

	_mp_playlist_view_push_item_content(view_data, view_layout, type);
	mp_view_layout_update(view_layout);
}

static void
_mp_playlist_view_auto_playlist_list_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	int ret = 0;
	int index = (int)data;
	char *name = NULL;
	mp_media_info_h media;

	Elm_Object_Item *gli = (Elm_Object_Item *) event_info;
	elm_genlist_item_selected_set(gli, FALSE);

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_retm_if(!layout_data, "layout_data is NULL !!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);

	if(layout_data->rename_git)
	{
		mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
		return;
	}

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	if (layout_data->edit_mode)
	{
		return;
	}

	view_data_t *view_data = evas_object_data_get(layout_data->ad->naviframe, "view_data");
	mp_retm_if(!view_data, "view_data is null");
	MP_CHECK_VIEW_DATA(view_data);

	mp_retm_if(mp_view_manager_count_view_content(view_data) != 1, "detail view_layout might be destroying....");
	media = mp_media_info_group_list_nth_item(layout_data->default_playlists, index);
	ret = mp_media_info_group_get_main_info(media, &name);
	mp_retm_if(ret != 0, "Fail to get value");
	mp_retm_if(name == NULL, "Fail to get value");

	DEBUG_TRACE("playlist name: %s", name);

	mp_playlist_view_create_auto_playlist(layout_data->ad, name);

}

bool
mp_playlist_view_create_by_id(Evas_Object * obj, int p_id)
{
	startfunc;
	char *name = NULL;
	mp_media_list_h list = NULL;
	int count = 0;
	int res = 0;
	int i = 0;
	mp_media_info_h media = NULL;
	mp_genlist_cb_t genlist_cbs;
	Evas_Object *view_layout = NULL;
	view_data_t *view_data = evas_object_data_get(obj, "view_data");
	mp_retvm_if(!view_data, FALSE, "view_data is null");
	MP_CHECK_VIEW_DATA(view_data);
	MP_CHECK_FALSE(p_id >= 0);

	view_layout = mp_view_layout_create(obj, view_data, MP_VIEW_MODE_DEFAULT);

	memset(&genlist_cbs, 0, sizeof(mp_genlist_cb_t));
	genlist_cbs.selected_cb = mp_common_track_genlist_sel_cb;

	res = mp_media_info_group_list_count(MP_GROUP_BY_PLAYLIST, NULL, NULL, &count);
	MP_CHECK_FALSE(res == 0);
	MP_CHECK_FALSE(count > 0);

	res = mp_media_info_group_list_create(&list, MP_GROUP_BY_PLAYLIST, NULL, NULL, 0, count);
	MP_CHECK_FALSE(res == 0);

	for(i=0; i<count; i++)
	{
		int playlist_id;
		media = mp_media_info_group_list_nth_item(list, i);
		mp_media_info_group_get_playlist_id(media, &playlist_id);
		if(playlist_id == p_id)
		{
			mp_media_info_group_get_main_info(media, &name);
			DEBUG_TRACE("Find playlist. name: %s", name);
			break;
		}
	}

	if ( i < count)
	{
		mp_view_layout_set_layout_data(view_layout,
					       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_TRACK_LIST,
					       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_BY_PLAYLIST,
					       MP_LAYOUT_PLAYLIT_ID, p_id,
					       MP_LAYOUT_LIST_CB, &genlist_cbs,
					       MP_LAYOUT_GENLIST_ITEMCLASS, _mp_playlist_view_get_track_sweep_itc(), -1);
	}
	else
		WARN_TRACE("invalid playlist!!");

	_mp_playlist_view_push_item_content(view_data, view_layout, name);
	mp_view_layout_update(view_layout);

	mp_media_info_group_list_destroy(list);

	endfunc;

	return true;
}

static void
_mp_playlist_view_playlist_list_select_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	int ret = 0;
	char *name = NULL;
	int p_id = 0;
	mp_genlist_item_data_t *item_data = NULL;

	Elm_Object_Item *gli = (Elm_Object_Item *) event_info;
	elm_genlist_item_selected_set(gli, FALSE);

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_retm_if(!layout_data, "layout_data is NULL !!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);

	if (elm_genlist_item_flip_get(gli))
	{
		return;
	}

	if(layout_data->rename_git)
	{
		mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
		return;
	}

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	if (layout_data->edit_mode)
	{
		mp_edit_view_genlist_sel_cb(data, obj, event_info);
		return;
	}

	view_data_t *view_data = evas_object_data_get(layout_data->ad->naviframe, "view_data");
	mp_retm_if(!view_data, "view_data is null");
	MP_CHECK_VIEW_DATA(view_data);

	mp_retm_if(layout_data->rename_mode, "rename mode.. ignore selection...");
	mp_retm_if(mp_view_manager_count_view_content(view_data) != 1, "detail view_layout might be destroying....");

	item_data = elm_object_item_data_get(gli);
	MP_CHECK(item_data);

	ret = mp_media_info_group_get_playlist_id(item_data->handle, &p_id);
	mp_retm_if(ret != 0, "Fail to get value");

	ret = mp_media_info_group_get_main_info(item_data->handle, &name);
	mp_retm_if(ret != 0, "Fail to get value");
	mp_retm_if(name == NULL, "Fail to get value");

	DEBUG_TRACE("playlist name: %s, playlist id: %d", name, p_id);

	Evas_Object *view_layout = mp_view_layout_create(view_data->navibar, view_data, MP_VIEW_MODE_DEFAULT);

	mp_genlist_cb_t genlist_cbs;
	memset(&genlist_cbs, 0, sizeof(mp_genlist_cb_t));
	genlist_cbs.selected_cb = mp_common_track_genlist_sel_cb;

	if (p_id >= 0)
	{
		mp_view_layout_set_layout_data(view_layout,
					       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_TRACK_LIST,
					       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_BY_PLAYLIST,
					       MP_LAYOUT_PLAYLIT_ID, p_id,
					       MP_LAYOUT_LIST_CB, &genlist_cbs,
					       MP_LAYOUT_GENLIST_ITEMCLASS, _mp_playlist_view_get_track_sweep_itc(), -1);
	}
	else
		WARN_TRACE("invalid playlist!!");

	mp_util_reset_genlist_mode_item(layout_data->genlist);

	_mp_playlist_view_push_item_content(view_data, view_layout, name);
	mp_view_layout_update(view_layout);

}

void
mp_playlist_view_add_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	struct appdata *ad = view_data->ad;
	ad->b_add_tracks = TRUE;

	Elm_Object_Item *top_item = elm_naviframe_top_item_get(view_data->navibar);
	Evas_Object *top_view = elm_object_item_content_get(top_item);
	mp_retm_if(!top_view, "top view must exist...");
	mp_layout_data_t *layout_data = evas_object_data_get(top_view, "layout_data");
	MP_CHECK_LAYOUT_DATA(layout_data);

	ad->new_playlist_id = layout_data->playlist_id;
	elm_toolbar_item_selected_set(ad->library->ctltab_songs, EINA_TRUE);
}

static void
_mp_playlist_view_push_item_content(view_data_t * view_data, Evas_Object * content, char *title)
{
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_view_manager_push_view_content(view_data, content, MP_VIEW_CONTENT_LIST);
	mp_view_manager_set_title_and_buttons(view_data, title, view_data);
}

void
mp_playlist_view_create_new_cancel_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");

	mp_view_manager_pop_view_content(view_data, FALSE, TRUE);
}

void
mp_playlist_view_create_new_done_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");

	struct appdata *ad = view_data->ad;
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

	ad->new_playlist_id = plst_uid;
	ad->b_add_tracks = TRUE;
	elm_toolbar_item_selected_set(ad->library->ctltab_songs, EINA_TRUE);

}

void
mp_playlist_view_create_playlist_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	char *new_playlist_name = NULL;
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	MP_CHECK(view_data->ad);
	mp_evas_object_del(view_data->ad->more_btn_popup);

	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");

	Evas_Object *create_plst_layout = NULL;

	new_playlist_name = mp_util_get_new_playlist_name();
	create_plst_layout = mp_common_create_editfield_layout(view_data->navibar, view_data->ad, new_playlist_name);
	IF_FREE(new_playlist_name);
	mp_retm_if(create_plst_layout == NULL, "create_plst_layout is NULL");

	Elm_Object_Item *it = mp_view_manager_push_view_content(view_data, create_plst_layout, MP_VIEW_CONTENT_NEW_PLAYLIST);
	elm_object_item_text_set(it, GET_STR("IDS_MUSIC_BODY_CREATE_PLAYLIST"));
	mp_language_mgr_register_object_item(it, "IDS_MUSIC_BODY_CREATE_PLAYLIST");

	Evas_Object *btn = mp_widget_create_button(create_plst_layout, "naviframe/toolbar/default", GET_SYS_STR("IDS_COM_OPT_SAVE"), NULL, mp_playlist_view_create_new_done_cb, view_data);
	elm_object_item_part_content_set(it, "toolbar_button1", btn);
	btn = mp_widget_create_button(create_plst_layout, "naviframe/back_btn/default", NULL, NULL, mp_playlist_view_create_new_cancel_cb, view_data);
	elm_object_item_part_content_set(it, "title_prev_btn", btn);

	mp_common_add_keypad_state_callback(view_data->ad->conformant, create_plst_layout, it);

	mp_view_manager_set_back_button(create_plst_layout, it, mp_playlist_view_create_new_cancel_cb, view_data);

	evas_object_show(create_plst_layout);

}

static char *
_mp_playlist_view_label_get(void *data, Evas_Object * obj, const char *part)
{
	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h plst_item = (item->handle);
	MP_CHECK_NULL(plst_item);

	int ret = 0;
	if (!strcmp(part, "elm.text.1") || !strcmp(part, "elm.slide.text.1"))
	{

		char *name = NULL;
		ret = mp_media_info_group_get_main_info(plst_item, &name);
		mp_retvm_if(ret != 0, NULL, "Fail to get value");
		mp_retvm_if(name == NULL, NULL, "Fail to get value");

		if (!strcmp(part, "elm.text.1"))
			return elm_entry_utf8_to_markup(GET_STR(name));
		else
			return g_strdup(GET_STR(name));
	}
	else if(!strcmp(part, "elm.text.2"))
	{
		int count = -1;
		int plst_id = -1;
		ret = mp_media_info_group_get_playlist_id(plst_item, &plst_id);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		MP_CHECK_NULL(plst_id >= 0);

		ret = mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, plst_id, &count);
		mp_retvm_if(ret != 0, NULL, "Fail to get count");
		mp_retvm_if(count < 0, NULL, "Fail to get count");
		return g_strdup_printf("(%d)", count);
	}

	return NULL;
}

void
mp_playlist_view_rename_done_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	char *text = NULL;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;

	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	/* save */

	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) elm_object_item_data_get(layout_data->rename_git);
	MP_CHECK(item);
	mp_media_info_h plst = (item->handle);
	MP_CHECK(plst);

	bool rename_success = FALSE;
	int ret = 0;

	text = mp_util_isf_get_edited_str(layout_data->ad->editfiled_entry, TRUE);

	if (!mp_util_is_playlist_name_valid((char *)text))
		mp_widget_text_popup(layout_data->ad, GET_STR("IDS_MUSIC_POP_UNABLE_RENAME_PLAYLIST"));
	else
	{
		bool exist = false;
		ret = mp_media_info_playlist_is_exist(text, &exist);
		if (ret != 0)
		{
			ERROR_TRACE("Fail to get playlist count by name: %d", ret);
			mp_widget_text_popup(layout_data->ad, GET_STR("IDS_MUSIC_POP_UNABLE_CREATE_PLAYLIST"));
		}
		else
		{
			if (exist) {
				char *origin_name = NULL;
				mp_media_info_group_get_main_info(plst, &origin_name);
				if (origin_name && !g_strcmp0(origin_name, text)) {
					mp_debug("Not edited.. rename OK");
					rename_success = TRUE;
				} else {
					char *msg = g_strdup_printf("Playlist name %s is exist", text);
					mp_widget_text_popup(layout_data->ad, msg);
					SAFE_FREE(msg);
				}
			} else {
				ret = mp_media_info_playlist_rename(plst, text);
				if (ret == 0) {
					mp_debug("mp_media_info_playlist_rename().. OK");
					rename_success = TRUE;
				}
			}
		}
	}
	IF_FREE(text);

	if (rename_success) {
		mp_debug("playlist rename success");
		if (layout_data->edit_mode)
			mp_view_manager_update_list_contents(layout_data->view_data, FALSE);
	}

	elm_genlist_item_update(layout_data->rename_git);

	mp_util_unset_rename(layout_data);

	mp_view_manager_set_controlbar_visible(mp_view_manager_get_navi_item(layout_data->ad), true);

	return;
}

void
mp_playlist_view_rename_cancel_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;

	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");
	mp_util_unset_rename(layout_data);
}

static void
_mp_playlist_rename_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	MP_CHECK(data);
	mp_layout_data_t *layout_data =
		(mp_layout_data_t *) evas_object_data_get(elm_object_item_widget_get(data), "layout_data");
	MP_CHECK(layout_data);

	if (layout_data->rename_mode)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		if (layout_data->rename_git)
		{
			elm_genlist_item_flip_set(layout_data->rename_git, EINA_FALSE);
			elm_genlist_item_select_mode_set(layout_data->rename_git, ELM_OBJECT_SELECT_MODE_DEFAULT);
		}
	}

	layout_data->rename_git = data;
	layout_data->rename_mode = true;

	elm_genlist_item_decorate_mode_set(layout_data->rename_git, "slide", EINA_FALSE);
	elm_genlist_item_flip_set(layout_data->rename_git, EINA_TRUE);
	elm_genlist_item_select_mode_set(layout_data->rename_git, ELM_OBJECT_SELECT_MODE_NONE);
	mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);
}


static bool
_mp_playlist_view_get_playlist_data_by_name(void *data, char *name)
{
	DEBUG_TRACE_FUNC();

	MP_CHECK_FALSE(data);
	MP_CHECK_FALSE(name);

	int ret = 0;
	char *playlist_name = name;
	mp_layout_data_t *layout_data = data;

	mp_media_list_h svc_handle = NULL;
	int count = -1;
	int playlist_id = -1;

	if (!strcmp((STR_MP_MOST_PLAYED), playlist_name))
	{
		playlist_id = -1;

		mp_media_info_list_count(MP_TRACK_BY_PLAYED_COUNT, NULL, NULL, NULL, 0, &count);
		DEBUG_TRACE("music count=%d\n", count);

		ret = mp_media_info_list_create(&svc_handle, MP_TRACK_BY_PLAYED_COUNT, NULL, NULL, NULL, 0, 0, count);
		if (ret != 0)
		{
			DEBUG_TRACE("fail to get list item: %d", ret);
			ret = mp_media_info_list_destroy(svc_handle);
			svc_handle = NULL;
		}
	}
	else if (!strcmp((STR_MP_RECENTLY_ADDED), playlist_name))
	{
		playlist_id = -1;
		mp_media_info_list_count(MP_TRACK_BY_ADDED_TIME, NULL, NULL, NULL, 0, &count);
		DEBUG_TRACE("music count=%d\n", count);

		/* get music item data */
		ret = mp_media_info_list_create(&svc_handle, MP_TRACK_BY_ADDED_TIME, NULL, NULL, NULL, 0, 0, count);
		if (ret != 0)
		{
			DEBUG_TRACE("fail to get list item: %d", ret);
			ret = mp_media_info_list_destroy(svc_handle);
			svc_handle = NULL;
		}
	}
	else if (!strcmp((STR_MP_RECENTLY_PLAYED), playlist_name))
	{
		playlist_id = -1;

		mp_media_info_list_count(MP_TRACK_BY_PLAYED_TIME, NULL, NULL, NULL, 0, &count);
		DEBUG_TRACE("music count=%d\n", count);

		/* get music item data */
		ret = mp_media_info_list_create(&svc_handle, MP_TRACK_BY_PLAYED_TIME, NULL, NULL, NULL, 0, 0, count);
		if (ret != 0)
		{
			DEBUG_TRACE("fail to get list item: %d", ret);
			ret = mp_media_info_list_destroy(svc_handle);
			svc_handle = NULL;
		}
	}
	else if (!strcmp((STR_MP_QUICK_LIST), playlist_name))
	{
		mp_media_info_list_count(MP_TRACK_BY_FAVORITE, NULL, NULL, NULL, 0, &count);
		DEBUG_TRACE("music count=%d\n", count);

		/* get music item data */
		ret = mp_media_info_list_create(&svc_handle, MP_TRACK_BY_FAVORITE, NULL, NULL, NULL, 0, 0, count);
		if (ret != 0)
		{
			DEBUG_TRACE("fail to get list item: %d", ret);
			ret = mp_media_info_list_destroy(svc_handle);
			svc_handle = NULL;
		}
	}
	else
	{
		ret = mp_media_info_playlist_get_id_by_name(playlist_name, &playlist_id);
		mp_retvm_if(ret != 0, FALSE, "ret: %d, playlist_name: %s", ret, playlist_name);
		mp_retvm_if(playlist_id < 0, FALSE, "id is not valid.. %d", playlist_id);

		ret = mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, playlist_id, &count);
		mp_retvm_if(ret != 0, FALSE, "ret: %d", ret);

		if (count <= 0)
		{
			DEBUG_TRACE("Recently played tracks were removed...");
			return NULL;
		}

		ret = mp_media_info_list_create(&svc_handle, MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, playlist_id, 0, count);
		if (ret != 0)
		{
			WARN_TRACE("fail to get list item: %d", ret);
		}
	}

	layout_data->playlist_id = playlist_id;
	layout_data->item_count = count;
	layout_data->svc_handle = svc_handle;

	return TRUE;
}

static void
_mp_playlist_view_playall_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();

	struct appdata *ad = NULL;
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");

	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	ad = layout_data->ad;

	mp_media_info_h handle = data;
	MP_CHECK(handle);

	int playlist_id = -1;
	char *playlist_name = NULL;
	int size = -1;
	int ret = 0;

	/* get playlist name */
	ret = mp_media_info_group_get_main_info(handle, &playlist_name);
	ret = mp_media_info_group_get_playlist_id(handle, &playlist_id);
	mp_retm_if(playlist_name == NULL, "Fail to get playlist_name");
	DEBUG_TRACE("playlist_id=%d, size=%d, playlist_name=%s\n", playlist_id, size, playlist_name);

	/* get playlist data by name */
	mp_layout_data_t *layout_data_new = calloc(1, sizeof(mp_layout_data_t));
	MP_CHECK(layout_data_new);
	MP_SET_LAYOUT_DATA_MAGIC(layout_data_new);
	layout_data_new->ad = ad;

	if (!_mp_playlist_view_get_playlist_data_by_name(layout_data_new, playlist_name))
	{
		IF_FREE(layout_data_new->type_str);
		IF_FREE(layout_data_new->filter_str);
		free(layout_data_new);

		return;
	}

	/* create playlist */
	mp_playlist_mgr_clear(ad->playlist_mgr);
	mp_util_append_media_list_item_to_playlist(ad->playlist_mgr, layout_data_new->svc_handle, layout_data_new->item_count, 0, NULL);

	/* play music */
	mp_play_view_load_and_play(ad, NULL, false);

	/* set the flag for update playlist view when back from playview */
	if (ad->playing_view != NULL)
	{
		ad->playing_view->b_play_all = true;
	}

	if (layout_data_new->svc_handle)
	{
		mp_media_info_list_destroy(layout_data_new->svc_handle);
	}

	IF_FREE(layout_data_new->type_str);
	IF_FREE(layout_data_new->filter_str);
	free(layout_data_new);

	endfunc;
}

static Evas_Object *
_mp_playlist_view_icon_get(void *data, Evas_Object * obj, const char *part)
{
	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h plst = (item->handle);
	MP_CHECK_NULL(plst);

	Evas_Object *eo = NULL;
	int ret = 0;
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	int playlist_id = 0;
	char *thumb_path = NULL;
	char *name = NULL;
	mp_retvm_if(!layout_data, NULL, "list data is NULL !!!");
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_media_info_group_get_playlist_id(plst, &playlist_id);
	ret = mp_media_info_group_get_thumbnail_path(plst, &thumb_path);
	mp_retvm_if(ret != 0, NULL, "Fail to get value");
	ret = mp_media_info_group_get_main_info(plst, &name);
	mp_retvm_if(ret != 0, NULL, "Fail to get value");

	const char *slide_rename_part = "elm.slide.swallow.2";
	const char *slide_delete_part = "elm.slide.swallow.3";

	if (!strcmp(part, "elm.icon"))
	{

		if (!playlist_id)
		{
			eo = mp_util_create_thumb_icon(obj, thumb_path, MP_LIST_ICON_SIZE,
							       MP_LIST_ICON_SIZE);
		}
		else
		{
			if (thumb_path == NULL)
			{
				int count = 0;
				mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, playlist_id, &count);
				if (count <= 0)
					eo = mp_util_create_thumb_icon(obj, THUMBNAIL_PLAYLIST_NOITEM,
								       MP_LIST_ICON_SIZE, MP_LIST_ICON_SIZE);
				else
					eo = mp_util_create_thumb_icon(obj, NULL, MP_LIST_ICON_SIZE, MP_LIST_ICON_SIZE);
			}
			else
			{
				eo = mp_util_create_thumb_icon(obj, thumb_path, MP_LIST_ICON_SIZE, MP_LIST_ICON_SIZE);
			}
		}
	}
	else if (!strcmp(part, "elm.swallow.end"))
	{
		if (playlist_id > 0 && layout_data->edit_mode)
		{
			eo = elm_button_add(obj);
			elm_object_style_set(eo, "rename");
			evas_object_smart_callback_add(eo, "clicked", _mp_playlist_rename_button_cb, item->it);
			evas_object_data_set(eo, "layout_data", layout_data);
		}
	}

	Evas_Object *button = NULL;
	if (!strcmp(part, "elm.slide.swallow.1"))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/multiline");
		elm_object_text_set(button, GET_STR("Play all"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "Play all");
		evas_object_smart_callback_add(button, "clicked", _mp_playlist_view_playall_button_cb, plst);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!strcmp(part, slide_rename_part))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/multiline");
		elm_object_text_set(button, GET_SYS_STR("IDS_COM_BODY_RENAME"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_COM_BODY_RENAME");
		evas_object_smart_callback_add(button, "clicked", _mp_playlist_rename_button_cb, item->it);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!strcmp(part, slide_delete_part))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/delete");
		elm_object_text_set(button, GET_SYS_STR("IDS_COM_OPT_DELETE"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_COM_OPT_DELETE");
		evas_object_smart_callback_add(button, "clicked", mp_menu_delete_cb, plst);
		evas_object_data_set(button, "layout_data", layout_data);

		return button;
	}

	Evas_Object *btn = NULL, *check = NULL, *edit_field = NULL, *entry = NULL;

	if (elm_genlist_decorate_mode_get(obj))
	{
		if (!strcmp(part, "elm.edit.icon.1"))
		{
			if (!elm_genlist_item_flip_get(item->it))
			{
				check = elm_check_add(obj);
				elm_check_state_pointer_set(check, &item->checked);
				evas_object_smart_callback_add(check, "changed", mp_common_item_check_changed_cb, item);
				return check;
			}
		}
		else if (!strcmp(part, "elm.edit.icon.2"))
		{
			btn = elm_button_add(obj);
			elm_object_style_set(btn, "rename");
			if (item->it)
			{
				evas_object_propagate_events_set(btn, EINA_FALSE);
				evas_object_smart_callback_add(btn, "clicked", _mp_playlist_rename_button_cb, item->it);
				evas_object_data_set(btn, "layout_data", layout_data);
			}
			return btn;
		}
	}

	if (!strcmp(part, "elm.flip.content"))
	{
		edit_field = mp_widget_create_editfield(obj, MP_PLAYLIST_NAME_SIZE - 1, NULL, layout_data->ad);
		entry = mp_widget_editfield_entry_get(edit_field);
		name = elm_entry_utf8_to_markup(name);
		elm_entry_entry_set(entry, name);
		elm_entry_cursor_end_set(entry);

		layout_data->ad->editfiled_entry = entry;

		evas_object_smart_callback_add(entry, "activated", mp_playlist_view_rename_done_cb, layout_data);

		evas_object_show(entry);
		elm_object_focus_set(entry, EINA_TRUE);

		return edit_field;
	}
	//}

	return eo;
}

static void _mp_playlist_view_playlist_setting_changed_cb(int state, void *data)
{
	startfunc;
	mp_view_layout_update(data);
}

static void _playlist_view_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	startfunc;
	mp_setting_playlist_set_callback(NULL, NULL);
}

Evas_Object *
mp_playlist_view_create(struct appdata *ad, mp_view_type_t view_type)
{
	DEBUG_TRACE("view_type: %d", view_type);
	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	view_data->view_type = view_type;
	Evas_Object *view_layout = mp_view_layout_create(ad->tabbar, view_data, MP_VIEW_MODE_DEFAULT);

	evas_object_event_callback_add(view_layout, EVAS_CALLBACK_DEL, _playlist_view_del_cb, NULL);
	mp_setting_playlist_set_callback(_mp_playlist_view_playlist_setting_changed_cb, view_layout);

	mp_layout_data_t *layout_data = NULL;
	layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK_NULL(layout_data);
	evas_object_data_set(ad->controlbar_layout, "layout_data", layout_data);

	static Elm_Genlist_Item_Class g_playlist_class_sweep = {
		.version = ELM_GENGRID_ITEM_CLASS_VERSION,
		.refcount = 0,
		.delete_me = EINA_FALSE,
		.item_style = "2text.1icon",
		.decorate_item_style = "mode/slide3",
		.func.text_get = _mp_playlist_view_label_get,
		.func.content_get = _mp_playlist_view_icon_get,
	};

	static Elm_Genlist_Item_Class g_playlist_class = {
		.version = ELM_GENGRID_ITEM_CLASS_VERSION,
		.refcount = 0,
		.delete_me = EINA_FALSE,
		.item_style = "2text.1icon",
		.decorate_item_style = "mode/slide",
		.func.text_get = _mp_playlist_view_label_get,
		.func.content_get = _mp_playlist_view_icon_get,
	};


	static mp_genlist_cb_t g_playlist_list_cbs = {
		.selected_cb = _mp_playlist_view_playlist_list_select_cb,
		.auto_playlist_cb = _mp_playlist_view_auto_playlist_list_select_cb
	};

	mp_view_layout_set_layout_data(view_layout,
				       MP_LAYOUT_CATEGORY_TYPE, MP_LAYOUT_PLAYLIST_LIST,
				       MP_LAYOUT_LIST_CB, &g_playlist_list_cbs,
				       MP_LAYOUT_GENLIST_ITEMCLASS, &g_playlist_class_sweep,
				       MP_LAYOUT_GENLIST_AUTO_PLAYLIST_ITEMCLASS, &g_playlist_class, -1);

	mp_view_manager_set_title_and_buttons(view_data, NULL, view_data);

	return view_layout;
}

void
mp_playlist_view_destroy(Evas_Object * playlist_view)
{
	DEBUG_TRACE("");
}

void
mp_playlist_view_refresh(Evas_Object * playlist_view)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) evas_object_data_get(playlist_view, "view_data");
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	mp_view_manager_update_list_contents(view_data, TRUE);
}

void
mp_playlist_view_update_navibutton(mp_layout_data_t * layout_data)
{
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_view_manager_set_title_and_buttons(layout_data->view_data, NULL, layout_data->view_data);
}

bool
mp_playlist_view_reset_rename_mode(struct appdata *ad)
{
	MP_CHECK_FALSE(ad);

	MP_CHECK_FALSE(ad->naviframe);

	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	MP_CHECK_FALSE(view_data);

	mp_layout_data_t *layout_data = NULL;

	Evas_Object *last_view_layout = mp_view_manager_get_last_view_layout(ad);

	if (last_view_layout)
	{
		layout_data = evas_object_data_get(last_view_layout, "layout_data");
	}

	MP_CHECK_FALSE(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_util_unset_rename(layout_data);

	return true;
}
