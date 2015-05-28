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

#include "mp-view-manager.h"
#include "mp-edit-view.h"
#include "mp-playlist-view.h"
#include "mp-track-view.h"
#include "mp-common.h"
#include "mp-menu.h"
#include "mp-play-view.h"
#include "mp-ug-launch.h"
#include "mp-search.h"
#include "mp-widget.h"
#include "mp-library.h"



typedef struct
{
	mp_view_content_t content_type;
	int view_index;
	Evas_Object *view_layout;
	Evas_Object *controlbar;
	Evas_Object *control_buttons[MP_NAVI_CONTROL_BUTTON_MAX];

	Elm_Object_Item *navi_item;
} mp_view_histroy_t;

void
mp_view_manager_set_back_button(Evas_Object * parent, Elm_Object_Item* navi_item, Evas_Smart_Cb cb, void *data)
{
	MP_CHECK(navi_item);
	MP_CHECK(parent);

	Evas_Object *button = NULL;
	if(cb)
	{
		button = mp_widget_create_button(parent,  "naviframe/back_btn/default", NULL, NULL, cb, data);
		elm_object_item_part_content_set(navi_item, ELM_NAVIFRAME_ITEM_PREV_BTN, button);
	}
	else
	{
		elm_object_item_part_content_set(navi_item, ELM_NAVIFRAME_ITEM_PREV_BTN, NULL);
	}
}

Elm_Object_Item *
mp_view_manager_push_view_content(view_data_t * view_data, Evas_Object * content, mp_view_content_t type)
{
	int view_idx = -1;
	mp_view_histroy_t *last_history;
	Evas_Object *top_view = NULL;

	startfunc;

	MP_CHECK_NULL(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	MP_CHECK_NULL(content);
	MP_CHECK_NULL(view_data->navibar);

	struct appdata *ad = view_data->ad;
	MP_CHECK_NULL(ad);

	Elm_Object_Item *navi_item = elm_naviframe_top_item_get(view_data->navibar);
	if (navi_item)
	{
		top_view = elm_object_item_content_get(navi_item);
		MP_CHECK_NULL(top_view);

		mp_view_layout_progress_timer_freeze(top_view);
	}

	view_idx = mp_view_manager_count_view_content(view_data);
	DEBUG_TRACE("view_idx: %d", view_idx);
	last_history = calloc(sizeof(mp_view_histroy_t), 1);

	last_history->content_type = type;
	last_history->view_index = view_idx;
	last_history->view_layout = content;

	if(view_idx == 0)
	{
		last_history->navi_item =
			elm_naviframe_item_push(view_data->navibar, NULL, NULL, NULL, content,  "tabbar");
		elm_naviframe_item_title_visible_set(last_history->navi_item, EINA_FALSE);
	}
	else if(MP_VIEW_CONTENT_PLAY == type)
	{
		elm_naviframe_prev_btn_auto_pushed_set(view_data->navibar, EINA_FALSE);
		last_history->navi_item =
			elm_naviframe_item_push(view_data->navibar, NULL, NULL, NULL, content,  "1line/music");
	}
	else
	{
		last_history->navi_item =
			elm_naviframe_item_push(view_data->navibar, NULL, NULL, NULL, content,  NULL);
	}

	mp_language_mgr_register_object_item(last_history->navi_item, NULL);

	if(MP_VIEW_CONTENT_PLAY == type)
			elm_naviframe_item_title_visible_set(last_history->navi_item, false);

	evas_object_data_set(content, "navi_item", last_history->navi_item);

	if (view_idx)		//Do not set as true when first push. Effect finished callback not called in this case..
		ad->navi_effect_in_progress = TRUE;

	ad->view_history = g_list_append(ad->view_history, last_history);

	endfunc;

	return last_history->navi_item;

}

/* Musc call this api befor del navigationbar. it prevent destroying info ug layout*/
void
mp_view_manager_unswallow_info_ug_layout(struct appdata *ad)
{
	MP_CHECK(ad);
	if(ad->info_ug_base)
	{
		edje_object_part_unswallow(ad->info_ug_base, ad->info_ug_layout);
		evas_object_hide(ad->info_ug_layout);
		mp_ug_send_message(ad, MP_UG_MESSAGE_DEL);
	}
}

void
mp_view_manager_pop_view_content(view_data_t * view_data, bool pop_to_first, bool pop_content)
{
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	struct appdata *ad = view_data->ad;
	MP_CHECK(ad);

	GList *view_item = g_list_last(ad->view_history);
	MP_CHECK(view_item);
	MP_CHECK(view_data->navibar);

	mp_view_histroy_t *last_history = view_item->data;

	mp_view_manager_unswallow_info_ug_layout(ad);

	if (!pop_to_first)
	{
		elm_naviframe_item_pop(view_data->navibar);
		ad->navi_effect_in_progress = TRUE;
		mp_language_mgr_unregister_object_item(last_history->navi_item);
		SAFE_FREE(last_history);
		ad->view_history =
			g_list_delete_link(ad->view_history, view_item);
		view_item = g_list_last(ad->view_history);
		last_history = view_item->data;
	}
	else
	{
		if (last_history->view_index == 0)
		{
			DEBUG_TRACE("");
			return;
		}
		elm_naviframe_item_pop_to(elm_naviframe_bottom_item_get(view_data->navibar));
		while (last_history->view_index > 0)
		{
			mp_language_mgr_unregister_object_item(last_history->navi_item);
			SAFE_FREE(last_history);
			ad->view_history =
				g_list_delete_link(ad->view_history, view_item);
			view_item = g_list_last(ad->view_history);
			MP_CHECK(view_item);
			last_history = view_item->data;
		}
	}

	Elm_Object_Item *navi_item = elm_naviframe_top_item_get(view_data->navibar);
	MP_CHECK(navi_item);
	Evas_Object *top_view = elm_object_item_content_get(navi_item);
	MP_CHECK(top_view);

	mp_view_layout_progress_timer_thaw(top_view);

}

void
mp_view_manager_pop_to_view_content(view_data_t * view_data, mp_view_content_t type)
{
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	struct appdata *ad = view_data->ad;
	MP_CHECK(ad);

	GList *view_item = g_list_last(ad->view_history);
	MP_CHECK(view_item);
	MP_CHECK(view_data->navibar);

	mp_view_histroy_t *last_history = view_item->data;
	bool find_view = false;

	mp_view_manager_unswallow_info_ug_layout(ad);

	while (last_history->view_index > 0)
	{
		DEBUG_TRACE("Content type: %d", last_history->content_type);
		if (last_history->content_type == type)
		{
			find_view = true;
		}
		mp_language_mgr_unregister_object_item(last_history->navi_item);
		SAFE_FREE(last_history);
		ad->view_history =
			g_list_delete_link(ad->view_history, view_item);
		view_item = g_list_last(ad->view_history);
		MP_CHECK(view_item);
		last_history = view_item->data;

		if (find_view)
		{
			elm_naviframe_item_pop_to(last_history->navi_item);
			break;
		}
	}

	Elm_Object_Item *navi_item = elm_naviframe_top_item_get(view_data->navibar);
	MP_CHECK(navi_item);
	Evas_Object *top_view = elm_object_item_content_get(navi_item);
	MP_CHECK(top_view);

	mp_view_layout_progress_timer_thaw(top_view);

}

int
mp_view_manager_count_view_content(view_data_t * view_data)
{
	MP_CHECK_VAL(view_data, -1);
	MP_CHECK_VIEW_DATA(view_data);

	GList *histroy = view_data->ad->view_history;
	mp_retvm_if(!histroy, 0, "last_item not exist");

	return g_list_length(histroy);
}

Evas_Object *
mp_view_manager_get_last_view_layout(struct appdata * ad)
{
	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(ad->naviframe);

	Evas_Object *cur_view = ad->naviframe;

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_NULL(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *last_item = g_list_last(ad->view_history);
	MP_CHECK_NULL(last_item);

	mp_view_histroy_t *last_history = last_item->data;
	MP_CHECK_NULL(last_history);

	return last_history->view_layout;
}

Elm_Object_Item *
mp_view_manager_get_play_view_navi_item(struct appdata *ad)
{
	startfunc;
	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(ad->naviframe);

	mp_view_histroy_t *history;
	Evas_Object *cur_view = ad->naviframe;

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_NULL(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *item = g_list_last(ad->view_history);
	MP_CHECK_NULL(item);

	do
	{
		history = item->data;
		if (history->content_type == MP_VIEW_CONTENT_PLAY)
		{
			return history->navi_item;
		}
		item = g_list_previous(item);
	}
	while (item);

	return NULL;
}

void
mp_view_manager_play_view_title_label_set(struct appdata *ad, const char *title)
{
	startfunc;
	Elm_Object_Item *navi_item;
	MP_CHECK(ad);

	navi_item = mp_view_manager_get_play_view_navi_item(ad);
	if(navi_item) {
		elm_object_item_text_set(navi_item, title);
	}

}

Evas_Object *
mp_view_manager_get_first_view_layout(struct appdata *ad)
{
	MP_CHECK_NULL(ad);

	Evas_Object *cur_view = ad->naviframe;
	MP_CHECK_NULL(cur_view);

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_NULL(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *first_item = g_list_first(ad->view_history);
	MP_CHECK_NULL(first_item);

	mp_view_histroy_t *last_history = first_item->data;
	MP_CHECK_NULL(last_history);

	return last_history->view_layout;
}

Evas_Object *
mp_view_manager_get_edit_view_layout(struct appdata * ad)
{
	mp_view_histroy_t *history = NULL;

	MP_CHECK_NULL(ad);

	Evas_Object *cur_view = ad->naviframe;
	MP_CHECK_NULL(cur_view);

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_NULL(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *item = g_list_last(ad->view_history);
	MP_CHECK_FALSE(item);

	do
	{
		history = item->data;
		if (history->content_type == MP_VIEW_CONTENT_EDIT)
		{
			return history->view_layout;
		}
		item = g_list_previous(item);
	}
	while (item);

	return NULL;
}


bool
mp_view_manager_is_play_view(struct appdata * ad)
{
	MP_CHECK_FALSE(ad);

	Evas_Object *cur_view = ad->naviframe;
	MP_CHECK_FALSE(cur_view);

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_FALSE(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *last_item = g_list_last(ad->view_history);
	MP_CHECK_FALSE(last_item);

	mp_view_histroy_t *last_history = last_item->data;
	MP_CHECK_FALSE(last_history);

	if (last_history->content_type == MP_VIEW_CONTENT_PLAY)
		return TRUE;
	else
		return FALSE;

}

Evas_Object *
mp_view_manager_get_view_layout(struct appdata * ad, mp_view_content_t type)
{
	mp_view_histroy_t *history = NULL;

	MP_CHECK_NULL(ad);

	Evas_Object *cur_view = ad->naviframe;
	MP_CHECK_NULL(cur_view);

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_NULL(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *item = g_list_last(ad->view_history);
	MP_CHECK_FALSE(item);

	do
	{
		history = item->data;
		if (history->content_type == type)
		{
			return history->view_layout;
		}
		item = g_list_previous(item);
	}
	while (item);

	return NULL;
}


void
mp_view_manager_update_list_contents(view_data_t * view_data, bool update_edit_list)
{
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_view_histroy_t *history = NULL;
	struct appdata *ad = view_data->ad;
	MP_CHECK(ad);

	GList *item = g_list_last(ad->view_history);
	MP_CHECK(item);
	do
	{
		history = item->data;
		if (history->content_type == MP_VIEW_CONTENT_LIST)
		{
			mp_layout_data_t *layout_data = evas_object_data_get(history->view_layout, "layout_data");
			if(layout_data)
			{
				if(layout_data->edit_mode && !update_edit_list)
				{
					DEBUG_TRACE("skip update edit view");
					item = g_list_previous(item);
					continue;
				}
				if(layout_data->album_delete_flag)
				{
					DEBUG_TRACE("skip update album view or artist view");
					item = g_list_previous(item);
					continue;
				}
			}
			mp_view_layout_update(history->view_layout);
		}
		item = g_list_previous(item);
	}
	while (item);

}

static void _mp_view_manager_more_btn_move_ctxpopup(Evas_Object *ctxpopup, Evas_Object *btn)
{
	startfunc;

	MP_CHECK(ctxpopup);
	MP_CHECK(btn);

	Evas_Coord x, y, w , h;
	evas_object_geometry_get(btn, &x, &y, &w, &h);
    evas_object_move(ctxpopup, x + w/2, y);
}

static void _mp_view_manager_more_btn_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;

	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_evas_object_del(ad->more_btn_popup);
}

static void _mp_view_manager_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	void *cb_data = NULL;

	struct appdata *ad = view_data->ad;
	MP_CHECK(ad);

	cb_data = evas_object_data_get(obj, "cb_data");

	mp_evas_object_del(ad->more_btn_popup);

	ad->more_btn_popup = elm_ctxpopup_add(ad->naviframe);
	evas_object_smart_callback_add(ad->more_btn_popup, "dismissed", _mp_view_manager_more_btn_dismissed_cb, ad);

	mp_more_button_type_e btn_type = (int)evas_object_data_get(obj, "more_btn_type");
	if (btn_type == MP_MORE_BUTTON_TYPE_TRACK_LIST)
		elm_ctxpopup_item_append(ad->more_btn_popup,
								GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"),
								NULL, mp_edit_view_add_to_plst_cb, cb_data);
	else
	{
		elm_ctxpopup_item_append(ad->more_btn_popup,
								GET_STR("IDS_MUSIC_BODY_CREATE_PLAYLIST"),
								NULL, mp_playlist_view_create_playlist_button_cb, cb_data);
	}

	elm_object_scroll_freeze_push(ad->more_btn_popup);
	_mp_view_manager_more_btn_move_ctxpopup(ad->more_btn_popup, obj);

	evas_object_show(ad->more_btn_popup);
}

void
mp_view_manager_set_title_and_buttons(view_data_t * view_data, char *text_ID, void *data)
{
	startfunc;
	struct appdata *ad = NULL;
	GList *last_node = NULL;
	mp_view_histroy_t *last_history = NULL;
	Evas_Object *navibar = NULL;
	Elm_Object_Item *navi_it = NULL;
	Evas_Object *layout = NULL, *btn = NULL;
	char *title = NULL;
	int i = 0;

	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	ad = view_data->ad;
	MP_CHECK(ad);

	last_node = g_list_last(ad->view_history);
	MP_CHECK(last_node);

	last_history = last_node->data;
	MP_CHECK(last_history);

	navibar = ad->naviframe;
	MP_CHECK(navibar);

	navi_it = last_history->navi_item;
	MP_CHECK(navi_it);

	layout = last_history->view_layout;
	MP_CHECK(layout);

	if(text_ID && strstr(text_ID, "IDS_COM"))
		title = GET_SYS_STR(text_ID);
	else
		title = GET_STR(text_ID);

	if (title)
	{
		DEBUG_TRACE("title: %s", title);
		elm_object_item_text_set(navi_it, title);
		mp_language_mgr_object_item_text_ID_set(navi_it, text_ID);
	}

	//delete all buttons...
	for (i = 0; i < MP_NAVI_CONTROL_BUTTON_MAX; i++)
	{
		if (last_history->control_buttons[i])
		{
			evas_object_del(last_history->control_buttons[i]);
			last_history->control_buttons[i] = NULL;
		}
	}
	elm_object_item_part_content_set(navi_it, "toolbar_button1", NULL);
	elm_object_item_part_content_set(navi_it, "toolbar_button2", NULL);

	//destroy back button
	mp_view_manager_set_back_button(last_history->view_layout, navi_it, NULL, NULL);
	btn = elm_object_item_part_content_get(navi_it, "toolbar_more_btn");
	if(btn)
	{
		evas_object_del(btn);
		elm_object_item_part_content_set(navi_it, "toolbar_more_btn", NULL);
	}

	if (last_history->content_type == MP_VIEW_CONTENT_LIST)
	{
		mp_layout_data_t *layout_data = evas_object_data_get(last_history->view_layout, "layout_data");
		MP_CHECK(layout_data);
		DEBUG_TRACE("layout_data: 0x%x", layout_data);
		MP_CHECK_LAYOUT_DATA(layout_data);

		DEBUG_TRACE("MP_VIEW_CONTENT_LIST. category: %d, rename mode: %d, add tracks : %d ",
			    layout_data->category, layout_data->rename_mode, layout_data->ad->b_add_tracks);
		DEBUG_TRACE("layout_data->playlist_id: %d", layout_data->playlist_id);

		if (layout_data->edit_mode)
		{
			if (layout_data->ad->b_add_tracks)
			{
				DEBUG_TRACE("");

				btn = mp_widget_create_toolbar_btn(layout,
					MP_TOOLBAR_BTN_DEFULTL, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"), mp_track_view_add_to_playlist_cancel_cb, data);
				elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
				last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_ADD_TO_PLAYLIST] = btn;

				mp_view_manager_set_back_button(layout, navi_it, mp_track_view_add_to_playlist_cancel_cb, data);

			}
			else
			{
				if (layout_data->category == MP_LAYOUT_TRACK_LIST)
				{
					DEBUG_TRACE("");

					btn = mp_widget_create_toolbar_btn(layout,
						MP_TOOLBAR_BTN_LEFT, GET_STR("IDS_MUSIC_OPT_DELETE"), mp_edit_view_delete_cb, data);
					elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
					last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_DELETE] = btn;

					btn = mp_widget_create_toolbar_btn(layout,
						MP_TOOLBAR_BTN_RIGHT, GET_STR("IDS_MUSIC_SK_SHARE"), mp_edit_view_share_cb, data);
					elm_object_item_part_content_set(navi_it, "toolbar_button2", btn);
					last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_SHARE] = btn;

					btn = mp_widget_create_toolbar_btn(layout, MP_TOOLBAR_BTN_MORE, NULL, _mp_view_manager_more_btn_cb, view_data);
					evas_object_data_set(btn, "more_btn_type", (void *)MP_MORE_BUTTON_TYPE_TRACK_LIST);
					elm_object_item_part_content_set(navi_it, "toolbar_more_btn", btn);
					evas_object_data_set(btn, "cb_data", data);

					mp_view_manager_set_back_button(layout, navi_it, mp_edit_view_cencel_cb, data);

				}
				else if (layout_data->category == MP_LAYOUT_GROUP_LIST)
				{
					DEBUG_TRACE("");
					btn = mp_widget_create_toolbar_btn(layout,
						MP_TOOLBAR_BTN_DEFULTL, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"), mp_edit_view_add_to_plst_cb, data);
					elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
					last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_ADD_TO_PLAYLIST] = btn;

					mp_view_manager_set_back_button(layout, navi_it, mp_edit_view_cencel_cb, data);


				}
				else
				{
					DEBUG_TRACE("");
					btn = mp_widget_create_toolbar_btn(layout,
						MP_TOOLBAR_BTN_DEFULTL, GET_STR("IDS_MUSIC_OPT_DELETE"), mp_edit_view_delete_cb, data);
					elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
					last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_DELETE] = btn;

					mp_view_manager_set_back_button(layout, navi_it, mp_edit_view_cencel_cb, data);
				}
			}

		}
		else
		{
			if(!layout_data->ad->b_add_tracks)
			{
					mp_view_manager_set_back_button(last_history->view_layout, last_history->navi_item, mp_common_back_button_cb, view_data);
			}

			if (title)
				layout_data->navibar_title = g_strdup(title);

			if (MP_TRACK_BY_ALBUM == layout_data->track_type
			    || MP_TRACK_BY_ARTIST == layout_data->track_type)
				layout_data->navibar_title = g_strdup(layout_data->type_str);

			layout_data->callback_data = data;
			if (layout_data->category != MP_LAYOUT_PLAYLIST_LIST)
			{
				if (layout_data->ad->b_add_tracks)
				{
					DEBUG_TRACE("");
					btn = mp_widget_create_toolbar_btn(layout,
						MP_TOOLBAR_BTN_DEFULTL, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"), mp_track_view_add_to_playlist_done_cb, data);
					elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
					last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_ADD_TO_PLAYLIST] = btn;


					mp_view_manager_set_back_button(layout, navi_it, mp_track_view_add_to_playlist_cancel_cb, data);

				}
				else if (layout_data->track_type == MP_TRACK_BY_PLAYLIST)
				{
					if (!mp_view_manager_get_view_layout(ad, MP_VIEW_CONTENT_SEARCH))
					{
						DEBUG_TRACE("");
						btn = mp_widget_create_toolbar_btn(layout,
							MP_TOOLBAR_BTN_LEFT, GET_SYS_STR("IDS_COM_SK_EDIT"), mp_common_edit_button_cb, data);
						elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
						last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_EDIT] = btn;

						if (layout_data->playlist_id > 0)
						{
							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_RIGHT, GET_SYS_STR("IDS_COM_SK_ADD"), mp_playlist_view_add_button_cb, data);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_CREATE_PLAYLIST] = btn;
						}
						else
						{
							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_RIGHT, GET_SYS_STR("IDS_COM_SK_SEARCH"), mp_common_search_button_cb, data);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_SEARCH] = btn;
						}
						elm_object_item_part_content_set(navi_it, "toolbar_button2", btn);
					}
					else
					{
						DEBUG_TRACE("");
						btn = mp_widget_create_toolbar_btn(layout,
							MP_TOOLBAR_BTN_DEFULTL, GET_SYS_STR("IDS_COM_SK_EDIT"), mp_common_edit_button_cb, data);
						elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
						last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_EDIT] = btn;

					}
				}
				else
				{
					if (layout_data->category == MP_LAYOUT_GROUP_LIST)
					{
						if (!mp_view_manager_get_view_layout(ad, MP_VIEW_CONTENT_SEARCH))
						{
							DEBUG_TRACE("");
							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_LEFT, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"), mp_common_edit_button_cb, data);
							elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_ADD_TO_PLAYLIST] = btn;

							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_RIGHT, GET_SYS_STR("IDS_COM_SK_SEARCH"), mp_common_search_button_cb, data);
							elm_object_item_part_content_set(navi_it, "toolbar_button2", btn);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_SEARCH] = btn;

						}
						else
						{
							DEBUG_TRACE("");
							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_DEFULTL, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"), mp_common_edit_button_cb, data);
							elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_ADD_TO_PLAYLIST] = btn;
						}
					}
					else
					{
						if (!mp_view_manager_get_view_layout(ad, MP_VIEW_CONTENT_SEARCH))
						{
							DEBUG_TRACE("");
							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_LEFT, GET_SYS_STR("IDS_COM_SK_EDIT"), mp_common_edit_button_cb, data);
							elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_EDIT] = btn;

							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_RIGHT, GET_SYS_STR("IDS_COM_SK_SEARCH"), mp_common_search_button_cb, data);
							elm_object_item_part_content_set(navi_it, "toolbar_button2", btn);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_SEARCH] = btn;
						}
						else
						{
							DEBUG_TRACE("");
							btn = mp_widget_create_toolbar_btn(layout,
								MP_TOOLBAR_BTN_DEFULTL, GET_SYS_STR("IDS_COM_SK_EDIT"), mp_common_edit_button_cb, data);
							elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
							last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_EDIT] = btn;
						}
					}
				}
			}
			else
			{
				DEBUG_TRACE("layout_data->item_count=%d", layout_data->item_count);
				btn = mp_widget_create_toolbar_btn(layout,
					MP_TOOLBAR_BTN_LEFT, GET_SYS_STR("IDS_COM_SK_EDIT"), mp_common_edit_button_cb, data);
				elm_object_item_part_content_set(navi_it, "toolbar_button1", btn);
				last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_EDIT] = btn;

				btn = mp_widget_create_toolbar_btn(layout,
					MP_TOOLBAR_BTN_RIGHT, GET_SYS_STR("IDS_COM_SK_SEARCH"), mp_common_search_button_cb, data);
				elm_object_item_part_content_set(navi_it, "toolbar_button2", btn);
				last_history->control_buttons[MP_NAVI_CONTROL_BUTTON_SEARCH] = btn;

				btn = mp_widget_create_toolbar_btn(layout, MP_TOOLBAR_BTN_MORE, NULL, _mp_view_manager_more_btn_cb, view_data);
				evas_object_data_set(btn, "more_btn_type", (void *)MP_MORE_BUTTON_TYPE_DEFAULT);
				evas_object_data_set(btn, "cb_data", data);
				elm_object_item_part_content_set(navi_it, "toolbar_more_btn", btn);
			}

			/* update the first controlba item */
			mp_view_manager_update_first_controlbar_item(layout_data);
		}

	}
	else if (last_history->content_type == MP_VIEW_CONTENT_SEARCH)
	{
		DEBUG_TRACE("MP_VIEW_CONTENT_SEARCH. ");
		mp_view_manager_set_back_button(last_history->view_layout, last_history->navi_item, mp_common_back_button_cb, view_data);
	}
	else if (last_history->content_type == MP_VIEW_CONTENT_PLAY)
	{
		DEBUG_TRACE("MP_VIEW_CONTENT_PLAY. ");
		mp_play_view_set_snd_path_sensitivity(ad);
	}
	else if (last_history->content_type == MP_VIEW_CONTENT_INFO)
	{
		DEBUG_TRACE("MP_VIEW_CONTENT_INFO. ");
		mp_view_manager_set_back_button(last_history->view_layout, last_history->navi_item, mp_play_view_info_back_cb, ad);
	}
	else
	{
		WARN_TRACE("unexpected value: %d", last_history->content_type);
	}

}

void
mp_view_manager_set_now_playing(view_data_t * view_data, bool show)
{
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_view_histroy_t *history = NULL;
	struct appdata *ad = view_data->ad;
	MP_CHECK(ad);

	GList *item = g_list_last(ad->view_history);
	GList *last_item = item;
	MP_CHECK(item);
	do
	{
		history = item->data;
		if (history->content_type == MP_VIEW_CONTENT_LIST)
		{
			if (show)
			{
				mp_view_layout_show_now_playing(history->view_layout);
				if(item != last_item) //freeze timer not to update every view. update top view.
					mp_view_layout_progress_timer_freeze(history->view_layout);
			}
			else
				mp_view_layout_hide_now_playing(history->view_layout);

			mp_view_layout_set_now_playing_info(history->view_layout);
		}
		item = g_list_previous(item);
	}
	while (item);

}

void
mp_view_manager_freeze_progress_timer(struct appdata *ad)
{
	Evas_Object *top_view = mp_view_manager_get_last_view_layout(ad);
	if (top_view)
		mp_view_layout_progress_timer_freeze(top_view);

	mp_play_view_progress_timer_freeze(ad);
}

void
mp_view_manager_thaw_progress_timer(struct appdata *ad)
{
	mp_retm_if(ad->is_lcd_off, "LCD off. not thaw progress timer.. ");

	Evas_Object *top_view = mp_view_manager_get_last_view_layout(ad);
	if (top_view)
		mp_view_layout_progress_timer_thaw(top_view);

	mp_play_view_progress_timer_thaw(ad);
}

void mp_view_manager_update_first_controlbar_item(void *data)
{
	mp_layout_data_t *layout_data = (mp_layout_data_t *)data;
	MP_CHECK(layout_data);
	Evas_Object *edit_item = mp_view_manager_get_controlbar_item(layout_data->ad, MP_NAVI_CONTROL_BUTTON_EDIT);
	if(edit_item)
	{
		if (layout_data->item_count < 1) {
			elm_object_disabled_set(edit_item, true);
		} else {
			elm_object_disabled_set(edit_item, false);
		}
	}
}

Evas_Object *
mp_view_manager_get_controlbar_item(struct appdata * ad, mp_navi_control_button_type type)
{
	MP_CHECK_NULL(ad);

	Evas_Object *cur_view = ad->naviframe;
	MP_CHECK_NULL(cur_view);

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_NULL(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *last_item = g_list_last(ad->view_history);
	MP_CHECK_NULL(last_item);

	mp_view_histroy_t *last_history = last_item->data;
	MP_CHECK_NULL(last_history);

	return last_history->control_buttons[type];
}

Elm_Object_Item *
mp_view_manager_get_navi_item(struct appdata * ad)
{
	MP_CHECK_NULL(ad);

	Evas_Object *cur_view = ad->naviframe;
	MP_CHECK_NULL(cur_view);

	view_data_t *cur_view_data = evas_object_data_get(cur_view, "view_data");
	MP_CHECK_NULL(cur_view_data);
	MP_CHECK_VIEW_DATA(cur_view_data);

	GList *last_item = g_list_last(ad->view_history);
	MP_CHECK_NULL(last_item);

	mp_view_histroy_t *last_history = last_item->data;
	MP_CHECK_NULL(last_history);

	return last_history->navi_item;
}

void
mp_view_manager_pop_info_view(struct appdata *ad)
{
	MP_CHECK(ad);
	MP_CHECK(ad->naviframe);

	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	MP_CHECK(view_data);

	if(ad->info_ug_base)
		mp_view_manager_pop_to_view_content(view_data, MP_VIEW_CONTENT_PLAY);

	MP_CHECK(ad->playing_view);
	evas_object_show(ad->playing_view->layout);
}

void
mp_view_manager_pop_play_view(struct appdata *ad)
{
	MP_CHECK(ad);
	MP_CHECK(ad->naviframe);

	view_data_t *view_data;

	view_data = evas_object_data_get(ad->naviframe, "view_data");
	MP_CHECK(view_data);

	if (ad->playing_view)
		mp_view_manager_pop_to_view_content(view_data, MP_VIEW_CONTENT_PLAY);
}

void
mp_view_manager_clear(struct appdata *ad)
{
	MP_CHECK(ad);

	GList *view_item = ad->view_history;
	while (view_item)
	{
		mp_view_histroy_t *last_history = view_item->data;
		mp_language_mgr_unregister_object_item(last_history->navi_item);
		SAFE_FREE(last_history);
		view_item = g_list_delete_link(view_item, view_item);
	}
	ad->view_history = NULL;
}

void
mp_view_manager_set_controlbar_visible(Elm_Object_Item *navi_item, bool visible)
{
	startfunc;
	MP_CHECK(navi_item);

	DEBUG_TRACE("visible: %d", visible);
	if(visible) {
		elm_object_item_signal_emit(navi_item, "elm,state,toolbar,open", "");
	} else {
		elm_object_item_signal_emit(navi_item, "elm,state,toolbar,close", "");
	}
}


