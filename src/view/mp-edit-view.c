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

#include "mp-edit-view.h"
#include "mp-view-layout.h"
#include "mp-library.h"
#include "mp-popup.h"
#include "mp-util.h"
#include "mp-widget.h"
#include "mp-common.h"
#include "mp-menu.h"
#include "mp-playlist-mgr.h"
#include "mp-playlist-view.h"
#include "mp-ctxpopup.h"

typedef enum {
	MP_EDIT_THREAD_FEEDBACK_UNABLE_TO_ADD_PLST,
	MP_EDIT_THREAD_FEEDBACK_CANCEL_BY_EXCEPTION,
} mp_edit_thread_feedback_e;

void
mp_edit_view_back_button_cb(void *data, Evas_Object * obj, void *event_info)
{
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");

	evas_object_smart_callback_del(obj, "clicked", mp_edit_view_back_button_cb);
	mp_view_manager_pop_view_content(view_data, FALSE, FALSE);
}

static const char *
_mp_edit_view_get_view_title(mp_layout_data_t * layout_data)
{
	const char *title = NULL;
		title = "IDS_COM_BODY_EDIT";

	return title;
}

void
mp_edit_view_share_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");

	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	if (layout_data->checked_count <= 0)
	{
		mp_widget_text_popup(layout_data->ad, GET_STR("IDS_MUSIC_POP_NOTHING_SELECTED"));
		return;
	}

	mp_ctxpopup_create(obj, MP_CTXPOPUP_LIST_SHARE, data, layout_data->ad);

	return;
}

void
mp_edit_view_create_new_cancel_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	view_data_t *view_data = layout_data->view_data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_view_manager_pop_view_content(view_data, FALSE, TRUE);
}

void
mp_edit_view_create_new_done_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();
	mp_playlist_h playlist_handle = NULL;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	view_data_t *view_data = layout_data->view_data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

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

	if(layout_data->edit_playlist_handle)
	{
		mp_media_info_playlist_handle_destroy(layout_data->edit_playlist_handle);
		layout_data->edit_playlist_handle = NULL;
	}

	plst_uid = mp_util_create_playlist(ad, converted_name, &playlist_handle);
	IF_FREE(converted_name);
	if (plst_uid < 0) {
		return;
	}

	mp_view_manager_pop_view_content(view_data, FALSE, TRUE);

	layout_data->edit_playlist_id = plst_uid;
	layout_data->edit_playlist_handle = playlist_handle;
	mp_edit_view_excute_edit(layout_data, MP_EDIT_ADD_TO_PLAYLIST);

	if (view_data->view_type == MP_VIEW_TYPE_PLAYLIST)
		mp_view_manager_update_list_contents(view_data, FALSE);

}

static void
_mp_edit_view_create_playlist_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE_FUNC();

	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK_LAYOUT_DATA(layout_data);
	MP_CHECK(layout_data->ad);

	view_data_t *view_data = layout_data->view_data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	Evas_Object *create_plst_layout = NULL;
	char *new_playlist_name = NULL;

	new_playlist_name = mp_util_get_new_playlist_name();
	create_plst_layout = mp_common_create_editfield_layout(view_data->navibar, view_data->ad, new_playlist_name);
	IF_FREE(new_playlist_name);
	mp_retm_if(create_plst_layout == NULL, "create_plst_layout is NULL");


	Elm_Object_Item *it = mp_view_manager_push_view_content(view_data, create_plst_layout, MP_VIEW_CONTENT_NEW_PLAYLIST_BY_EDIT);
	elm_object_item_text_set(it, GET_STR("IDS_MUSIC_BODY_CREATE_PLAYLIST"));
	mp_language_mgr_register_object_item(it, "IDS_MUSIC_BODY_CREATE_PLAYLIST");

	Evas_Object *btn = mp_widget_create_button(create_plst_layout, "naviframe/toolbar/default", GET_SYS_STR("IDS_COM_OPT_SAVE"), NULL, mp_edit_view_create_new_done_cb, layout_data);
	elm_object_item_part_content_set(it, "title_toolbar_button1", btn);
	btn = mp_widget_create_button(create_plst_layout, "naviframe/back_btn/default", NULL, NULL, mp_playlist_view_create_new_cancel_cb, view_data);
	elm_object_item_part_content_set(it, "title_prev_btn", btn);

	mp_common_add_keypad_state_callback(layout_data->ad->conformant, create_plst_layout, it);

	mp_view_manager_set_back_button(create_plst_layout, it, mp_playlist_view_create_new_cancel_cb, view_data);

	evas_object_show(create_plst_layout);

	mp_popup_destroy(layout_data->ad);
}

static void
_mp_edit_view_add_to_playlist_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	MP_CHECK(data);

	mp_layout_data_t *layout_data = (mp_layout_data_t *) evas_object_data_get(obj, "layout_data");
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	layout_data->edit_playlist_id = (int)data;
	mp_popup_destroy(layout_data->ad);
	mp_edit_view_excute_edit(layout_data, MP_EDIT_ADD_TO_PLAYLIST);
}

static void _mp_edit_view_popup_del_cb(void *data, Evas * e, Evas_Object * eo, void *event_info)
{
	mp_media_list_h list = data;
	mp_media_info_group_list_destroy(list);
}

void
mp_edit_view_add_to_plst_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	MP_CHECK(layout_data->ad);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	bool b_from_more_btn = false;
	if (layout_data->ad->more_btn_popup)
		b_from_more_btn = true;

	mp_evas_object_del(layout_data->ad->more_btn_popup);

	Evas_Object *popup = NULL;

	if (layout_data->checked_count <= 0)
	{
		mp_widget_text_popup(layout_data->ad, GET_STR("IDS_MUSIC_POP_NOTHING_SELECTED"));
		return;
	}

	popup = mp_genlist_popup_create(layout_data->ad->win_main, MP_POPUP_ADD_TO_PLST, data, layout_data->ad);
	evas_object_data_set(popup, "layout_data", layout_data);
	mp_retm_if(!popup, "popup is NULL !!!");

	Evas_Object *genlist = evas_object_data_get(popup, "genlist");
	evas_object_data_set(genlist, "layout_data", layout_data);

	mp_genlist_popup_item_append(popup, GET_STR("IDS_MUSIC_OPT_CREATE_PLAYLIST"), NULL,
				     _mp_edit_view_create_playlist_cb, layout_data);
	int i = 0, count = -1, ret = -1;

	ret = mp_media_info_group_list_count(MP_GROUP_BY_PLAYLIST, NULL, NULL, &count);
	if (ret != 0)
	{
		ERROR_TRACE("Error in mp_media_info_group_list_count (%d)\n", ret);
		return;
	}

	if (count)
	{
		mp_media_list_h list = NULL;

		ret = mp_media_info_group_list_create(&list, MP_GROUP_BY_PLAYLIST, NULL, NULL, 0, count);
		if (ret != 0)
		{
			WARN_TRACE("Fail to get playlist");
			return;
		}
		for (i = 0; i < count; i++)
		{
			char *name = NULL;
			int id;
			mp_media_info_h item = NULL;
			item = mp_media_info_group_list_nth_item(list, i);

			ret = mp_media_info_group_get_main_info(item, &name);
			mp_retm_if(ret != 0, "Fail to get value");
			ret = mp_media_info_group_get_playlist_id(item, &id);
			mp_retm_if(ret != 0, "Fail to get value");

			mp_genlist_popup_item_append(popup, name, NULL, _mp_edit_view_add_to_playlist_cb, (void *)id);
		}

		evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _mp_edit_view_popup_del_cb, list);
	}

	evas_object_show(popup);
}

static Eina_Bool
_mp_edit_view_edit_idler_cb(void *data)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	struct appdata *ad = NULL;
	mp_genlist_item_data_t *item = NULL;
	mp_media_info_h item_handle = NULL;
	int ret = 0;
	int plst_id = -1;
	char *fid = NULL;
	int uid = 0;
	char *file_path;
	bool error_occured = FALSE;

	MP_CHECK_FALSE(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	ad = layout_data->ad;
	MP_CHECK_FALSE(ad);

	if (!layout_data->current_edit_item)
	{
		WARN_TRACE("CHECK here...");
		goto END;
	}

	if(elm_genlist_item_select_mode_get(layout_data->current_edit_item) == ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
	{
		layout_data->current_edit_item = elm_genlist_item_next_get(layout_data->current_edit_item);
		return EINA_TRUE;
	}

	item = elm_object_item_data_get(layout_data->current_edit_item);

	if (!item)
	{
		WARN_TRACE("CHECK here...");
		goto END;
	}

	if (MP_TRACK_BY_ALBUM == layout_data->track_type
	    || MP_TRACK_BY_ARTIST == layout_data->track_type)
		layout_data->category = MP_LAYOUT_TRACK_LIST;

	if (item->checked)
	{
		if (layout_data->edit_operation == MP_EDIT_DELETE)
		{
			switch (layout_data->category)
			{
			case MP_LAYOUT_PLAYLIST_LIST:
				{
					layout_data->current_edit_item =
						elm_genlist_item_next_get(layout_data->current_edit_item);
					item_handle = (item->handle);
					if (!item_handle)
					{
						WARN_TRACE("CHECK here...");
						goto END;
					}

					ret = mp_media_info_group_get_playlist_id(item_handle, &plst_id);
					MP_CHECK_EXCEP(ret == 0);

					ret = mp_media_info_playlist_delete_from_db(plst_id);
					MP_CHECK_EXCEP(ret == 0);

					DEBUG_TRACE("playlist (%d) deleted", plst_id);
					elm_object_item_del(item->it);
					layout_data->item_count--;
					layout_data->checked_count--;

					break;
				}
			case MP_LAYOUT_TRACK_LIST:
				{
					layout_data->current_edit_item =
						elm_genlist_item_next_get(layout_data->current_edit_item);
					item_handle =  (item->handle);
					if (item_handle)
					{
						if (layout_data->playlist_id >= 0)
						{
							int member_id = 0;
							ret = mp_media_info_get_playlist_member_id(item_handle, &member_id);
							MP_CHECK_EXCEP(ret == 0);
							ret = mp_media_info_playlist_remove_media(layout_data->playlist_handle, member_id);
							MP_CHECK_EXCEP(ret == 0);
							DEBUG_TRACE("playlist id = %d, id = %d", plst_id, uid);
						}
						else if (layout_data->track_type == MP_TRACK_BY_ADDED_TIME)
						{
							ret = mp_media_info_set_added_time(item_handle, 0);
							MP_CHECK_EXCEP(ret == 0);
						}
						else if (layout_data->track_type == MP_TRACK_BY_PLAYED_TIME)
						{
							ret = mp_media_info_set_played_time(item_handle, 0);
							MP_CHECK_EXCEP(ret == 0);
						}
						else if (layout_data->track_type == MP_TRACK_BY_FAVORITE)
						{
							ret = mp_media_info_set_favorite(item_handle, false);
							MP_CHECK_EXCEP(ret == 0);
						}
						else if (layout_data->track_type == MP_TRACK_BY_PLAYED_COUNT)
						{
							ret = mp_media_info_set_played_count(item_handle, 0);
							MP_CHECK_EXCEP(ret == 0);
						}
						else
						{
							ret = mp_media_info_get_media_id(item_handle, &fid);
							ret = mp_media_info_get_file_path(item_handle, &file_path);
							MP_CHECK_EXCEP(ret == 0);
							if (mp_util_delete_track(layout_data->ad, fid, file_path, false) != MP_FILE_DELETE_ERR_NONE)
							{
								DEBUG_TRACE("Fail to delete item, fid: %d, path: %s",
									    fid, file_path);
								layout_data->error_count++;
								error_occured = true;
							}
						}
					}
					if (!error_occured)
					{
						if (MP_TRACK_BY_ARTIST == layout_data->track_type) {
							if(!elm_genlist_item_parent_get(elm_genlist_item_next_get(item->it))
								&& !elm_genlist_item_parent_get(elm_genlist_item_prev_get(item->it)))
							{
								elm_object_item_del(elm_genlist_item_parent_get(item->it));
							}
							else
							{
								elm_genlist_item_update(elm_genlist_item_parent_get(item->it));
							}
						} else if (MP_TRACK_BY_ALBUM == layout_data->track_type) {
							/* update group title */
							elm_genlist_item_update(layout_data->album_group);
						}
						elm_object_item_del(item->it);
						layout_data->item_count--;
						layout_data->checked_count--;
					}
					break;
				}
			default:
				WARN_TRACE("unexpected case...");
				break;
			}
			mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);
		}
		else
		{
			WARN_TRACE("Unsupported operation...");
			goto mp_exception;
		}
	}
	else
		layout_data->current_edit_item = elm_genlist_item_next_get(layout_data->current_edit_item);

	if (layout_data->current_edit_item)
	{
		return EINA_TRUE;
	}
	DEBUG_TRACE("no more items");

      END:
      	DEBUG_TRACE("");
	mp_popup_response(ad->popup[MP_POPUP_PROGRESS], MP_POPUP_YES);
	layout_data->edit_idler = NULL;
	return EINA_FALSE;

      mp_exception:
      	DEBUG_TRACE("");
	if (layout_data->group_track_handle)
	{
		mp_media_info_list_destroy(layout_data->group_track_handle);
		layout_data->group_track_handle = NULL;
	}
	mp_popup_response(ad->popup[MP_POPUP_PROGRESS], MP_POPUP_NO);
	layout_data->edit_idler = NULL;
	return EINA_FALSE;


}

static void
_mp_edit_view_progress_popup_response_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	MP_CHECK(obj);
	mp_evas_object_del(obj);

	struct appdata *ad = layout_data->ad;
	MP_CHECK(ad);

	if (layout_data->edit_idler)
	{
		ecore_idler_del(layout_data->edit_idler);
		layout_data->edit_idler = NULL;
	}

	if (layout_data->edit_thread)
	{
		ecore_thread_cancel(layout_data->edit_thread);
		layout_data->edit_thread = NULL;
	}

	if (layout_data->group_track_handle)
	{
		mp_media_info_list_destroy(layout_data->group_track_handle);
		layout_data->group_track_handle = NULL;
	}

	if(mp_view_manager_count_view_content(layout_data->view_data) >= 1)
		mp_view_manager_update_list_contents(layout_data->view_data, TRUE);

	int selected_count = layout_data->selected_count;

	mp_view_layout_set_edit_mode(layout_data, false);
	mp_util_unset_rename(layout_data);

	DEBUG_TRACE("selected_count, %d, error_count: %d", selected_count, layout_data->error_count);

	if (layout_data->edit_operation == MP_EDIT_ADD_TO_PLAYLIST)
	{
		if (selected_count)
			mp_util_post_status_message(layout_data->ad, GET_STR("IDS_MUSIC_POP_ADDED"));
	}
	else
	{
		if ((selected_count == 1) && layout_data->error_count)
		{
			if ((layout_data->category == MP_LAYOUT_TRACK_LIST)
			    && (layout_data->view_data->view_type == MP_VIEW_TYPE_PLAYLIST))
			{
				mp_util_post_status_message(layout_data->ad, GET_SYS_STR("IDS_COM_POP_FAILED"));
			}
			else
			{
				mp_util_post_status_message(layout_data->ad, GET_SYS_STR("IDS_COM_POP_FAILED"));
			}
		}
		else
		{
			if ((layout_data->category == MP_LAYOUT_TRACK_LIST)
			    && (layout_data->view_data->view_type == MP_VIEW_TYPE_PLAYLIST))
			{
				mp_util_post_status_message(layout_data->ad, GET_SYS_STR("IDS_COM_POP_REMOVED"));
			}
			else
			{
				mp_util_post_status_message(layout_data->ad, GET_SYS_STR("IDS_COM_POP_DELETED"));
			}
		}
	}

	layout_data->ad->edit_in_progress = false;

	if (layout_data->edit_operation == MP_EDIT_ADD_TO_PLAYLIST && layout_data->ad->b_add_tracks)
	{
		layout_data->ad->b_add_tracks = FALSE;
		mp_view_layout_reset_select_all(layout_data);
		elm_toolbar_item_selected_set(layout_data->ad->library->ctltab_plist, EINA_TRUE);

		if (selected_count)
			mp_util_post_status_message(ad, GET_STR("IDS_MUSIC_POP_ADDED"));
	}

}

static void
_mp_edit_view_add_to_plst_thread(void *data, Ecore_Thread *thread)
{
	mp_layout_data_t *layout_data = data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	struct appdata *ad = layout_data->ad;
	MP_CHECK(ad);

	mp_genlist_item_data_t *item = NULL;
	mp_media_info_h item_handle = NULL;
	int ret = 0;
	char *fid = NULL;
	char *title = NULL;

	if (MP_TRACK_BY_ALBUM == layout_data->track_type || MP_TRACK_BY_ARTIST == layout_data->track_type)
		layout_data->category = MP_LAYOUT_TRACK_LIST;

	while (layout_data->current_edit_item)
	{
		if (ecore_thread_check(thread)) {	// pending cancellation
			WARN_TRACE("popup cancel clicked");
			goto mp_exception;
		}

		if(elm_genlist_item_select_mode_get(layout_data->current_edit_item) == ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
		{
			layout_data->current_edit_item = elm_genlist_item_next_get(layout_data->current_edit_item);
			continue;
		}

		item = elm_object_item_data_get(layout_data->current_edit_item);
		if (!item)
		{
			WARN_TRACE("CHECK here...");
			ecore_thread_feedback(thread, (void *)MP_EDIT_THREAD_FEEDBACK_CANCEL_BY_EXCEPTION);
			goto mp_exception;
		}

		if (!item->checked)
		{
			layout_data->current_edit_item = elm_genlist_item_next_get(layout_data->current_edit_item);
			continue;
		}

#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
		if (layout_data->playlist_track_count >= MP_PLAYLIST_MAX_ITEM_COUNT)
		{
			DEBUG_TRACE("unable to add more tracks...");
			ecore_thread_feedback(thread, (void *)MP_EDIT_THREAD_FEEDBACK_CANCEL_BY_EXCEPTION);
			goto mp_exception;
		}
#endif
		switch (layout_data->category) {
		case MP_LAYOUT_TRACK_LIST:
		{
			item_handle =  (item->handle);
			if (item_handle)
			{
				ret = mp_media_info_get_media_id(item_handle,  &fid);
				MP_CHECK_EXCEP(ret == 0);

				if (!mp_util_add_to_playlist_by_key(layout_data->edit_playlist_id, fid))
				{
					ecore_thread_feedback(thread, (void *)MP_EDIT_THREAD_FEEDBACK_UNABLE_TO_ADD_PLST);
					ecore_thread_feedback(thread, (void *)MP_EDIT_THREAD_FEEDBACK_CANCEL_BY_EXCEPTION);
					goto mp_exception;
				}
				else
					layout_data->playlist_track_count++;
			}

			layout_data->current_edit_item = elm_genlist_item_next_get(layout_data->current_edit_item);
			break;
		}
		case MP_LAYOUT_GROUP_LIST:
		{
			item_handle =  (item->handle);
			if (item_handle)
			{
				mp_track_type_e item_type = MP_TRACK_ALL;
				int count = 0;
				mp_media_info_h item = NULL;

				if (layout_data->view_data->view_type == MP_VIEW_TYPE_FOLDER)
				{
					ret = mp_media_info_group_get_folder_id(item_handle, &title);
				}
				else
				{
					ret = mp_media_info_group_get_main_info(item_handle, &title);
				}
				MP_CHECK_EXCEP(ret == 0);
				MP_CHECK_EXCEP(title);

				item_type = mp_menu_get_track_type_by_group(layout_data->group_type);
				if (!layout_data->group_track_handle)
				{
					ret = mp_media_info_list_count(item_type, title, NULL, NULL, 0, &count);

					MP_CHECK_EXCEP(ret == 0);
					MP_CHECK_EXCEP(count > 0);
					mp_debug("track_count: %d", count);

					ret = mp_media_info_list_create(&layout_data->group_track_handle, item_type, title, NULL, NULL, 0, 0, count);
					MP_CHECK_EXCEP(ret == 0);

					layout_data->edit_track_index = count - 1;
					layout_data->group_item_delete_error = false;
				}

				item = mp_media_info_list_nth_item(layout_data->group_track_handle, layout_data->edit_track_index);
				ret = mp_media_info_get_media_id(item, &fid);
				MP_CHECK_EXCEP(ret == 0);

				if (!mp_util_add_to_playlist_by_key(layout_data->edit_playlist_id, fid))
				{
					ecore_thread_feedback(thread, (void *)MP_EDIT_THREAD_FEEDBACK_CANCEL_BY_EXCEPTION);
					goto mp_exception;
				}
				else
					layout_data->playlist_track_count++;

				if (layout_data->edit_track_index <= 0)
				{
					DEBUG_TRACE("all tracks in %s added.", title);
					layout_data->current_edit_item = elm_genlist_item_next_get(layout_data->current_edit_item);
					mp_media_info_list_destroy(layout_data->group_track_handle);
					layout_data->group_track_handle = NULL;
				}
				else
					layout_data->edit_track_index--;

			}
			break;
		}
		default:
			WARN_TRACE("unexpected case...");
			break;
		}
	}

mp_exception:
	if (layout_data->group_track_handle) {
		mp_media_info_list_destroy(layout_data->group_track_handle);
		layout_data->group_track_handle = NULL;
	}
}

static void
_mp_edit_view_edit_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
	DEBUG_TRACE("");
	mp_layout_data_t *layout_data = data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	struct appdata *ad = layout_data->ad;
	MP_CHECK(ad);

	mp_edit_thread_feedback_e feedback = (mp_edit_thread_feedback_e)msg_data;
	switch (feedback) {
	case MP_EDIT_THREAD_FEEDBACK_UNABLE_TO_ADD_PLST:
		mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_ADD"));
		break;

	case MP_EDIT_THREAD_FEEDBACK_CANCEL_BY_EXCEPTION:
		mp_popup_response(ad->popup[MP_POPUP_PROGRESS], MP_POPUP_NO);
		break;

	default:
		WARN_TRACE("Not defined feedback .. [%d]", feedback);
	}
}

static void
_mp_edit_view_edit_thread_end_cb(void *data, Ecore_Thread *thread)
{
	WARN_TRACE("thread_end");
	mp_layout_data_t *layout_data = data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	layout_data->edit_thread = NULL;

	struct appdata *ad = layout_data->ad;
	MP_CHECK(ad);

	mp_popup_response(ad->popup[MP_POPUP_PROGRESS], MP_POPUP_YES);
}

static void
_mp_edit_view_edit_thread_cancel_cb(void *data, Ecore_Thread *thread)
{
	WARN_TRACE("thread_cancel");
}

void
mp_edit_view_excute_edit(mp_layout_data_t * layout_data, mp_edit_operation_t edit_operation)
{
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	struct appdata *ad = layout_data->ad;
	MP_CHECK(ad);

	Elm_Object_Item *first_item = elm_genlist_first_item_get(layout_data->genlist);
	MP_CHECK(first_item);
	layout_data->current_edit_item = first_item;

	if (layout_data->edit_idler)
	{
		ecore_idler_del(layout_data->edit_idler);
		layout_data->edit_idler = NULL;
	}

	layout_data->selected_count = layout_data->checked_count;
	layout_data->error_count = 0;
	layout_data->group_track_handle = NULL;
	layout_data->group_item_delete_error = false;
	layout_data->edit_operation = edit_operation;

	if (edit_operation == MP_EDIT_ADD_TO_PLAYLIST)
	{
		mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, NULL, layout_data->edit_playlist_id, &layout_data->playlist_track_count);
		DEBUG_TRACE("number of tracks in playlist: %d", layout_data->playlist_track_count);
#ifdef MP_PLAYLIST_MAX_ITEM_COUNT
		if (layout_data->playlist_track_count >= MP_PLAYLIST_MAX_ITEM_COUNT)
		{
			if (layout_data->ad->b_add_tracks)
			{
				layout_data->ad->b_add_tracks = FALSE;
				mp_view_layout_reset_select_all(layout_data);
				elm_toolbar_item_selected_set(layout_data->ad->library->ctltab_plist, EINA_TRUE);
			}
			char *fmt_str = GET_STR("IDS_MUSIC_POP_UNABLE_TO_ADD_MORE_THAN_PD_MUSIC_FILE");
			char *noti_str = g_strdup_printf(fmt_str, MP_PLAYLIST_MAX_ITEM_COUNT);
			mp_util_post_status_message(ad, noti_str);
			IF_FREE(noti_str);
			return;
		}
#endif
	}


	if (edit_operation == MP_EDIT_ADD_TO_PLAYLIST)
	{
		mp_popup_create(ad->win_main, MP_POPUP_PROGRESS, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"), layout_data,
				_mp_edit_view_progress_popup_response_cb, ad);

		layout_data->edit_thread = ecore_thread_feedback_run(
						_mp_edit_view_add_to_plst_thread,
						_mp_edit_view_edit_thread_notify_cb,
						_mp_edit_view_edit_thread_end_cb,
						_mp_edit_view_edit_thread_cancel_cb,
						(const void *)layout_data,
						EINA_TRUE);

		if (!layout_data->edit_thread) {
			mp_popup_response(ad->popup[MP_POPUP_PROGRESS], MP_POPUP_NO);
		}
	}
	else
	{
		layout_data->edit_idler = ecore_idler_add(_mp_edit_view_edit_idler_cb, layout_data);

		DEBUG_TRACE("layout_data->category=%d=====layout_data->view_data->view_type=%d\n",
			    layout_data->category, layout_data->view_data->view_type);
		if ((layout_data->category == MP_LAYOUT_TRACK_LIST)
		    && (layout_data->view_data->view_type == MP_VIEW_TYPE_PLAYLIST))
		{
			mp_popup_create(ad->win_main, MP_POPUP_PROGRESS, GET_SYS_STR("IDS_COM_SK_REMOVE"), layout_data,
					_mp_edit_view_progress_popup_response_cb, ad);
		}
		else
		{
			mp_popup_create(ad->win_main, MP_POPUP_PROGRESS, GET_SYS_STR("IDS_COM_OPT_DELETE"), layout_data,
					_mp_edit_view_progress_popup_response_cb, ad);
		}
	}
	ad->edit_in_progress = true;

}

static void
_mp_edit_view_excute_delete_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	MP_CHECK(layout_data->ad);
	layout_data->ad->popup_delete = NULL;

	mp_evas_object_del(obj);

	int response = (int)event_info;
	if (response == MP_POPUP_NO)
	{
		return;
	}
	ERROR_TRACE("0x%x", layout_data->layout);
	mp_edit_view_excute_edit(layout_data, MP_EDIT_DELETE);

	endfunc;
	return;
}

void
mp_edit_view_cencel_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	struct appdata *ad = NULL;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	ad = layout_data->ad;

	mp_view_layout_set_edit_mode(layout_data, false);
	mp_util_unset_rename(layout_data); /* Cancel the rename mode */

}

void
mp_edit_view_delete_cb(void *data, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	struct appdata *ad = NULL;
	mp_layout_data_t *layout_data = (mp_layout_data_t *) data;
	MP_CHECK(layout_data);
	MP_CHECK_LAYOUT_DATA(layout_data);
	ERROR_TRACE("0x%x", layout_data->layout);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	if (layout_data->checked_count <= 0)
	{
		mp_widget_text_popup(layout_data->ad, GET_STR("IDS_MUSIC_POP_NOTHING_SELECTED"));
		return;
	}

	ad = layout_data->ad;

	Evas_Object *popup = mp_popup_create(ad->win_main, MP_POPUP_NORMAL, NULL, layout_data, _mp_edit_view_excute_delete_cb, ad);
	evas_object_data_set(popup, "layout_data", layout_data);
	ad->popup_delete = popup;

	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	if ((layout_data->category == MP_LAYOUT_TRACK_LIST)
	    && (layout_data->view_data->view_type == MP_VIEW_TYPE_PLAYLIST))
	{
		elm_object_text_set(popup, GET_STR("IDS_MUSIC_POP_REMOVE_Q"));
	}
	else
	{
		elm_object_text_set(popup, GET_SYS_STR("IDS_COM_POP_DELETE_Q"));
	}

	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_SK_YES"), MP_POPUP_YES);
	mp_popup_button_set(popup, MP_POPUP_BTN_2, GET_SYS_STR("IDS_COM_SK_NO"), MP_POPUP_NO);

	evas_object_show(popup);

}

void
mp_edit_view_genlist_sel_cb(void *data, Evas_Object * obj, void *event_info)
{
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");
	mp_retm_if(!layout_data, "layout_data is NULL !!!!");
	MP_CHECK_LAYOUT_DATA(layout_data);

	mp_retm_if(layout_data->ad->navi_effect_in_progress, "navi effect in progress");

	Elm_Object_Item *gli = (Elm_Object_Item *) event_info;
	elm_genlist_item_selected_set(gli, FALSE);

	if (elm_genlist_item_flip_get(gli))
	{
		return;
	}

	mp_genlist_item_data_t *item = (mp_genlist_item_data_t *) elm_object_item_data_get(gli);
	MP_CHECK(item);
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
}

Evas_Object *
mp_edit_view_create(view_data_t * view_data)
{
	DEBUG_TRACE("");
	MP_CHECK_NULL(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	int reorder = FALSE;

	mp_common_hide_search_ise_context(view_data);

	Evas_Object *top_view = mp_view_manager_get_last_view_layout(view_data->ad);
	mp_retvm_if(!top_view, NULL, "top view must exist...");
	mp_layout_data_t *layout_data = evas_object_data_get(top_view, "layout_data");
	MP_CHECK_LAYOUT_DATA(layout_data);

	if (layout_data->category == MP_LAYOUT_TRACK_LIST && layout_data->track_type == MP_TRACK_BY_PLAYLIST)
		reorder = TRUE;

	mp_view_layout_set_layout_data(top_view, MP_LAYOUT_EDIT_MODE, TRUE, MP_LAYOUT_REORDER_MODE, reorder, -1);

	mp_view_layout_set_edit_mode(layout_data, true);

	const char *title = NULL;
	if(mp_view_manager_count_view_content(view_data) > 1)
		title = _mp_edit_view_get_view_title(layout_data);

	mp_view_manager_set_title_and_buttons(view_data, title, layout_data);
	mp_common_set_toolbar_button_sensitivity(layout_data, 0);

	return NULL;

}

bool
mp_edit_view_get_selected_track_list(void *data, GList **p_list)
{
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);

	Evas_Object *top_view = mp_view_manager_get_last_view_layout(ad);
	MP_CHECK_FALSE(top_view);

	mp_layout_data_t *layout_data = evas_object_data_get(top_view, "layout_data");
	MP_CHECK_FALSE(layout_data);	/* not list layout */

	if (layout_data->category != MP_LAYOUT_TRACK_LIST || !layout_data->edit_mode)
	{
		mp_debug("Not track list (edit mode)");
		return false;
	}

	if (layout_data->checked_count > 0)
	{
		Elm_Object_Item *gl_item = elm_genlist_first_item_get(layout_data->genlist);
		char *path = NULL;
		while (gl_item) {
			mp_genlist_item_data_t *item = elm_object_item_data_get(gl_item);
			if (item && item->checked) {
				mp_media_info_get_file_path(item->handle, &path);
				if (path) {
					*p_list = g_list_append(*p_list, path);
				}
			}

			gl_item = elm_genlist_item_next_get(gl_item);
		}
	}

	return true;
}

