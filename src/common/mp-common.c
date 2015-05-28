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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Ecore_X.h>
#include <vconf.h>

#include "mp-common.h"
#include "mp-player-debug.h"

#include "mp-media-info.h"
#include "mp-view-layout.h"
#include "mp-util.h"
#include "mp-menu.h"
#include "mp-play-view.h"
#include "mp-playlist-view.h"
#include "mp-edit-view.h"
#include "mp-search.h"
#include "mp-view-manager.h"
#include "mp-widget.h"
#include "mp-library.h"
#include "mp-volume.h"



char *
mp_common_track_list_label_get(void *data, Evas_Object * obj, const char *part)
{
	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h track = (mp_media_info_h) (item->handle);
	mp_retvm_if(!track, NULL, "data is null");

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	MP_CHECK_NULL(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	if (layout_data->track_type == MP_TRACK_BY_FOLDER && !g_strcmp0(part, "elm.text.1"))
	{
		char *pathname = NULL;
		mp_media_info_get_file_path(track, &pathname);
		mp_retv_if(!pathname, NULL);
		const char *fileanme = ecore_file_file_get(pathname);

		return g_strdup(fileanme);
	}
	else if (!strcmp(part, "elm.text.1") || !strcmp(part, "elm.slide.text.1"))
	{
		char *title = NULL;

		mp_media_info_get_title(track,  &title);

		mp_retv_if(!title, NULL);
		if(layout_data->filter_str)
		{
			char *markup_name;
			bool res = false;
			markup_name = (char *)mp_util_search_markup_keyword(title, layout_data->filter_str, &res);
			if(res)
				return strdup(markup_name);
		}
		else if (!strcmp(part, "elm.text.1"))
		{
			char *markup = elm_entry_utf8_to_markup(title);
			return markup;
		}
		else
			return g_strdup(title);
	}
	else if (!strcmp(part, "elm.text.2") && layout_data->track_type != MP_TRACK_BY_FOLDER)
	{
		char *artist = NULL;
			mp_media_info_get_artist(track, &artist);
		mp_retv_if(!artist, NULL);
		return g_strdup(artist);
	}
	else if (!strcmp(part, "elm.text.3") ||
		(layout_data->track_type == MP_TRACK_BY_FOLDER && !g_strcmp0(part, "elm.text.2")))
	{
		int duration;
		char time[16] = "";
			mp_media_info_get_duration(track, &duration);

		mp_util_format_duration(time, duration);
		time[15] = '\0';
		return g_strdup(time);
	}
	return NULL;
}

void
mp_common_track_genlist_sel_cb(void *data, Evas_Object * obj, void *event_info)
{
	Elm_Object_Item *gli = (Elm_Object_Item *) event_info;
	Elm_Object_Item *gli2 = NULL;
	elm_genlist_item_selected_set(gli, FALSE);

	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_retm_if(!layout_data, "layout_data is NULL !!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);
	MP_CHECK(layout_data->ad);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	if(layout_data->rename_git)
	{
		mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
		return;
	}

	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) elm_object_item_data_get(gli);
	MP_CHECK(item);

	if (layout_data->ad->b_add_tracks)
	{
		item->checked = !item->checked;

		if (item->checked)
			layout_data->checked_count++;
		else
			layout_data->checked_count--;

		if (layout_data->select_all_layout)
		{
			if (layout_data->item_count == layout_data->checked_count)
				layout_data->select_all_checked = EINA_TRUE;
			else
				layout_data->select_all_checked = EINA_FALSE;

			elm_check_state_pointer_set(layout_data->select_all_checkbox, &layout_data->select_all_checked);
		}

		elm_genlist_item_fields_update(item->it, "elm.edit.icon.1", ELM_GENLIST_ITEM_FIELD_CONTENT);
		mp_util_create_selectioninfo_with_count(layout_data->layout, layout_data->checked_count);
		mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);

		return;
	}

	if (layout_data->edit_mode)
	{
		mp_edit_view_genlist_sel_cb(data, obj, event_info);
		return;
	}
	mp_util_reset_genlist_mode_item(layout_data->genlist);

	/*  If genlist item removed by sweep menu or editing,
	*   layout_data->svc_handle can not be used for creating playlist..
	*   update layout again before start play from list view.
	*/

	mp_plst_item *plst_item = NULL;
	mp_plst_item *current_item = NULL;
	char *prev_item_uid = NULL;
	plst_item = mp_playlist_mgr_get_current(layout_data->ad->playlist_mgr);
	if(plst_item)
		prev_item_uid = g_strdup(plst_item->uid);

	mp_playlist_mgr_clear(layout_data->ad->playlist_mgr);
	gli2 = elm_genlist_first_item_get(layout_data->genlist);
	while(gli2)
	{
		if(elm_genlist_item_select_mode_get(gli2)  != ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
		{
			mp_genlist_item_data_t *item_data = elm_object_item_data_get(gli2);
			if (item_data && item_data->item_type == MP_GENLIST_ITEM_TYPE_NORMAL)
			{
				if(item_data->group_type == MP_GROUP_NONE || item_data->group_type == MP_GROUP_BY_ALLSHARE)
				{
					char *uri = NULL;
					char *uid = NULL;

					mp_track_type track_type = MP_TRACK_URI;
						mp_media_info_get_media_id(item_data->handle, &uid);
						mp_media_info_get_file_path(item_data->handle, &uri);
					plst_item = mp_playlist_mgr_item_append(layout_data->ad->playlist_mgr, uri, uid, track_type);
					if(gli2 == gli && plst_item)
						current_item = plst_item;
				}
			}
		}
		gli2 = elm_genlist_item_next_get(gli2);
	}

	mp_playlist_mgr_set_current(layout_data->ad->playlist_mgr, current_item);

	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, true);
	if(!mp_play_view_load_and_play(layout_data->ad, prev_item_uid, FALSE)) {
		mp_widget_text_popup(layout_data->ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_ERROR_OCCURRED"));
		mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, false);
	}

	IF_FREE(prev_item_uid);

	return;
}

Evas_Object *
mp_common_track_list_icon_get(void *data, Evas_Object * obj, const char *part)
{
	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) data;
	MP_CHECK_NULL(item);
	mp_media_info_h track = item->handle;
	mp_retvm_if(!track, NULL, "data is null");
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_retvm_if(!layout_data, NULL, "list_data is null!!");
	MP_CHECK_LAYOUT_DATA(layout_data);

	if (!strcmp(part, "elm.icon"))
	{
		char *thumbpath = NULL;
		Evas_Object *icon;
		mp_media_info_get_thumbnail_path(track, &thumbpath);
		icon = mp_util_create_thumb_icon(obj, thumbpath, MP_LIST_ICON_SIZE, MP_LIST_ICON_SIZE);
		return icon;
	}

	Evas_Object *button;

	if (!strcmp(part, "elm.slide.swallow.3"))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/multiline");
		elm_object_text_set(button, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_MUSIC_BODY_ADD_TO_PLAYLIST");
		evas_object_smart_callback_add(button, "clicked", mp_menu_add_to_playlist_cb, track);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!strcmp(part, "elm.slide.swallow.1"))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/multiline");
		elm_object_text_set(button, GET_SYS_STR("IDS_COM_BUTTON_SHARE"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_COM_BUTTON_SHARE");
		evas_object_smart_callback_add(button, "clicked", mp_menu_share_cb, track);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!strcmp(part, "elm.slide.swallow.2"))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/multiline");
		elm_object_text_set(button, GET_SYS_STR("IDS_COM_SK_SET"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_COM_SK_SET");
		evas_object_smart_callback_add(button, "clicked", mp_menu_set_cb, track);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!strcmp(part, "elm.slide.swallow.4"))
	{
		button = elm_button_add(obj);
		elm_object_style_set(button, "sweep/delete");
		MP_CHECK_NULL(layout_data->view_data);
		elm_object_text_set(button, GET_SYS_STR("IDS_COM_OPT_DELETE"));
		mp_language_mgr_register_object(button, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_COM_OPT_DELETE");
		evas_object_smart_callback_add(button, "clicked", mp_menu_delete_cb, track);
		evas_object_data_set(button, "layout_data", layout_data);
		return button;
	}
	else if (!g_strcmp0(part, "elm.icon.storage"))
	{
		char *folder = NULL;
		int ret = mp_media_info_get_file_path(track, &folder);
		mp_retvm_if((ret != 0), NULL, "Fail to get value");
		if (folder) {
			const char *icon_path = NULL;
			if (g_strstr_len(folder, strlen(MP_PHONE_ROOT_PATH), MP_PHONE_ROOT_PATH))
				icon_path = MP_ICON_STORAGE_PHONE;
			else if (g_strstr_len(folder, strlen(MP_MMC_ROOT_PATH), MP_MMC_ROOT_PATH))
				icon_path = MP_ICON_STORAGE_MEMORY;
			else
				icon_path = MP_ICON_STORAGE_EXTERNAL;

			Evas_Object *icon = elm_icon_add(obj);
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

	return NULL;
}

static Evas_Object *
_mp_common_gl_icon_get(void *data, Evas_Object * obj, const char *part)
{
	Evas_Object *editfield = NULL;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(obj);

	if (!strcmp(part, "elm.icon"))
	{
		ad->editfiled_new_playlist = editfield =
			mp_widget_create_editfield(obj, MP_PLAYLIST_NAME_SIZE - 1, NULL, ad);
		Evas_Object *entry = mp_widget_editfield_entry_get(editfield);
		elm_entry_entry_set(entry, ad->new_playlist_name);
		elm_entry_cursor_end_set(entry);
		evas_object_show(editfield);

		return editfield;
	}
	return NULL;
}


Evas_Object *
mp_common_create_editfield_layout(Evas_Object * parent, struct appdata * ad, char *text)
{
	startfunc;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *item = NULL;
	static Elm_Genlist_Item_Class itc;
	Evas_Object *layout = NULL;

	MP_CHECK_NULL(parent);
	MP_CHECK_NULL(ad);

	layout = elm_layout_add(parent);
	elm_layout_file_set(layout, EDJ_NAME, "create_playlist");

	IF_FREE(ad->new_playlist_name);
	if (text)
		ad->new_playlist_name = strdup(text);

	itc.version = ELM_GENGRID_ITEM_CLASS_VERSION;
	itc.refcount = 0;
	itc.delete_me = EINA_FALSE;
	itc.item_style = "dialogue/1icon";
	itc.func.text_get = NULL;
	itc.func.content_get = _mp_common_gl_icon_get;
	itc.func.state_get = NULL;
	itc.func.del = NULL;

	genlist = elm_genlist_add(layout);
	item = elm_genlist_item_append(genlist, &itc, ad, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	elm_object_scroll_freeze_push(genlist);

	elm_object_part_content_set(layout, "elm.swallow.content", genlist);
	evas_object_show(layout);

	return layout;
}

void
mp_common_hide_search_ise_context(view_data_t * view_data)
{
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	mp_layout_data_t *layout_data = NULL;

	Evas_Object *last_view_content = mp_view_manager_get_last_view_layout(view_data->ad);
	MP_CHECK(last_view_content);

	layout_data = (mp_layout_data_t *) evas_object_data_get(last_view_content, "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	mp_search_hide_imf_pannel(layout_data->search_bar);
}

static void
_mp_common_search_layout_del_cb(void *data, Evas * e, Evas_Object * eo, void *event_info)
{
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	//view_data->view_mode = MP_VIEW_MODE_DEFAULT;
}


void
mp_common_search_button_cb(void *data, Evas_Object * obj, void *event_info)
{

	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	MP_CHECK(view_data->ad);

	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");

	Evas_Object *layout = mp_view_layout_create(view_data->navibar, view_data, MP_VIEW_MODE_SEARCH);

	Evas_Object *top_view = mp_view_manager_get_last_view_layout(view_data->ad);
	MP_CHECK(top_view);
	mp_layout_data_t *layout_data = evas_object_data_get(top_view, "layout_data");
	MP_CHECK_LAYOUT_DATA(layout_data);

	// list must be updated. label changed callback does not called any more...
	mp_view_layout_update(layout);

	mp_view_manager_push_view_content(view_data, layout, MP_VIEW_CONTENT_SEARCH);

	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, _mp_common_search_layout_del_cb, view_data);

	mp_view_manager_set_title_and_buttons(view_data, "IDS_COM_SK_SEARCH", mp_util_get_layout_data(layout));

}

void
mp_common_edit_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	mp_retm_if(view_data->ad->b_add_tracks, "add track mode..");
	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");
	mp_retm_if(mp_view_manager_get_edit_view_layout(view_data->ad), "edit view already created...");

	Evas_Object *layout = mp_view_manager_get_last_view_layout(view_data->ad);
	mp_layout_data_t *layout_data = evas_object_data_get(layout, "layout_data");
	MP_CHECK(layout_data);

	if (layout_data->item_count < 1)
	{
		mp_widget_text_popup(layout_data->ad, GET_SYS_STR("IDS_COM_BODY_NO_ITEMS"));
		return;
	}
	mp_edit_view_create(view_data);
}

void
mp_common_back_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	bool b_play_all = false;

	struct appdata *ad = view_data->ad;
	MP_CHECK(ad);
	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");


	mp_layout_data_t* layout_data = mp_util_get_layout_data(mp_view_manager_get_last_view_layout(ad));
	if (layout_data && layout_data->search_bar) {
		Evas_Object *en = mp_search_entry_get(layout_data->search_bar);
		Ecore_IMF_Context *imf_context = elm_entry_imf_context_get(en);
		/* hide SIP */
		if (imf_context)
			ecore_imf_context_input_panel_hide(imf_context);
		/* set the entry object not be focused */
		elm_object_focus_allow_set(en, EINA_FALSE);
	}

	if(mp_view_manager_count_view_content(view_data) == 1)
	{
		DEBUG_TRACE("First view. go to background");
		elm_win_lower(ad->win_main);
		return;
	}

	/* get the flag if the playing view is created by playall */
	if (ad->playing_view != NULL)
	{
		b_play_all = ad->playing_view->b_play_all;
		ad->playing_view->b_play_all = false;
	}

	mp_view_manager_pop_view_content(view_data, FALSE, FALSE);
	evas_object_smart_callback_del(obj, "clicked", mp_common_back_button_cb);

	if(mp_util_is_db_updating())
	{
		Evas_Object *top_view = mp_view_manager_get_last_view_layout(ad);
		mp_view_layout_update(top_view);
	}
	else if (layout_data != NULL && layout_data->view_data != NULL)
	{
		if (layout_data->category == MP_LAYOUT_TRACK_LIST && layout_data->view_data->view_type == MP_VIEW_TYPE_PLAYLIST)
		{
			Evas_Object *top_view = mp_view_manager_get_last_view_layout(ad);
			mp_view_layout_update(top_view);
		}
	}
	else if (b_play_all)
	{
		/* when back from play view which created by playall option, the layout_data is NULL */
		Evas_Object *top_view = mp_view_manager_get_last_view_layout(ad);
		mp_view_layout_update(top_view);
	}


}

void
mp_common_item_check_changed_cb(void *data, Evas_Object * obj, void *event_info)
{
	mp_genlist_item_data_t *item_data = (mp_genlist_item_data_t *) data;
	MP_CHECK(item_data);
	mp_layout_data_t *layout_data =
		(mp_layout_data_t *) evas_object_data_get(elm_object_item_widget_get(item_data->it), "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	if(layout_data->rename_git)
	{
		mp_playlist_view_rename_done_cb(layout_data, NULL, NULL);
		item_data->checked = !item_data->checked;
		DEBUG_TRACE("item_data->checked: %d", item_data->checked);
		elm_check_state_pointer_set(obj, &item_data->checked);
		return;
	}

	if (item_data->checked)
		layout_data->checked_count++;
	else
		layout_data->checked_count--;

	// update select all check button
	if (layout_data->select_all_layout)
	{
		if (layout_data->item_count == layout_data->checked_count)
			layout_data->select_all_checked = EINA_TRUE;
		else
			layout_data->select_all_checked = EINA_FALSE;

		elm_check_state_pointer_set(layout_data->select_all_checkbox, &layout_data->select_all_checked);
	}

	mp_util_create_selectioninfo_with_count(layout_data->layout, layout_data->checked_count);
	mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);
}


void
mp_common_set_toolbar_button_sensitivity(mp_layout_data_t * layout_data, int selected_count)
{

	int i;
	int all_track_count = 0;
	Evas_Object *item;
	bool disable = true;

	MP_CHECK(layout_data);

	DEBUG_TRACE("rename_mode=%d, edit_mode=%d, selected_count=%d\n item_count: %d",
	            layout_data->rename_mode, layout_data->edit_mode, selected_count, layout_data->item_count);

	if (layout_data->rename_mode)
	{
		for (i = 0; i < MP_NAVI_CONTROL_BUTTON_MAX; i++) /* Don't disable the cancel button in rename mode */
		{
			item = mp_view_manager_get_controlbar_item(layout_data->ad, i);
			elm_object_disabled_set(item, disable);
		}
	}
	else if (layout_data->edit_mode)
	{
		if(selected_count > 0)
			disable = false;

		for (i = 0; i < MP_NAVI_CONTROL_BUTTON_MAX; i++)	//Cancel button always enabled.
		{
			item = mp_view_manager_get_controlbar_item(layout_data->ad, i);
			elm_object_disabled_set(item, disable);
		}
	}
	else if (layout_data->ad->b_add_tracks)
	{
		item = mp_view_manager_get_controlbar_item(layout_data->ad, MP_NAVI_CONTROL_BUTTON_ADD_TO_PLAYLIST);
		MP_CHECK(item);
		if(selected_count > 0)
			elm_object_disabled_set(item, false);
		else
			elm_object_disabled_set(item, true);
	}
	else
	{
		if(layout_data->item_count > 0)
			disable = false;

		if(layout_data->category != MP_LAYOUT_PLAYLIST_LIST)
		{
			item = mp_view_manager_get_controlbar_item(layout_data->ad, MP_NAVI_CONTROL_BUTTON_EDIT);
			if (item)
				elm_object_disabled_set(item, disable);
		}
		else
		{
			bool disable_search = true;
			mp_media_info_list_count(MP_TRACK_ALL, NULL, NULL, NULL, 0, &all_track_count);
			if(all_track_count > 0)
				disable_search = false;
			item = mp_view_manager_get_controlbar_item(layout_data->ad, MP_NAVI_CONTROL_BUTTON_EDIT);
			if(item)
				elm_object_disabled_set(item, disable);
		}
	}

}

void
mp_common_navigationbar_finish_effect(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("finish effect");
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	ad->navi_effect_in_progress = FALSE;

	if(!elm_naviframe_prev_btn_auto_pushed_get(ad->naviframe))
		elm_naviframe_prev_btn_auto_pushed_set(ad->naviframe, EINA_TRUE);

	if (ad->editfiled_new_playlist) {
		Evas_Object *entry = mp_widget_editfield_entry_get(ad->editfiled_new_playlist);
		elm_object_focus_set(entry, EINA_TRUE);
	}

	mp_layout_data_t * layout_data = mp_util_get_layout_data(mp_view_manager_get_last_view_layout(ad));
	MP_CHECK(layout_data);
	if(layout_data->search_bar)
	{
		Evas_Object *ed = mp_search_entry_get(layout_data->search_bar);
		elm_object_focus_set(ed, EINA_TRUE);
	}
}

static void _show_title_toolbar(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	elm_object_item_signal_emit(data, "elm,state,sip,shown", "");
}

static void _hide_title_toolbar(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	elm_object_item_signal_emit(data, "elm,state,sip,hidden", "");
}

static void
_mp_common_layout_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	MP_CHECK(data);
	evas_object_smart_callback_del(data, "virtualkeypad,state,on", _show_title_toolbar);
	evas_object_smart_callback_del(data, "virtualkeypad,state,off", _hide_title_toolbar);
}

void
mp_common_add_keypad_state_callback(Evas_Object *conformant, Evas_Object *view_layout, Elm_Object_Item *navi_item)
{
	startfunc;
	MP_CHECK(conformant);
	MP_CHECK(view_layout);

	evas_object_smart_callback_add(conformant, "virtualkeypad,state,on", _show_title_toolbar, navi_item);
	evas_object_smart_callback_add(conformant, "virtualkeypad,state,off", _hide_title_toolbar, navi_item);

	evas_object_event_callback_add(view_layout, EVAS_CALLBACK_DEL, _mp_common_layout_del_cb,
				       conformant);
}

