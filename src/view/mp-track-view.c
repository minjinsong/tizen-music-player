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
#include "mp-widget.h"
#include "mp-util.h"

void
mp_track_view_add_to_playlist_done_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");

	mp_layout_data_t *layout_data = NULL;
	layout_data = evas_object_data_get(mp_view_manager_get_first_view_layout(view_data->ad), "layout_data");
	if (layout_data)
	{
		MP_CHECK_LAYOUT_DATA(layout_data);
		layout_data->edit_playlist_id = view_data->ad->new_playlist_id;
		mp_edit_view_excute_edit(layout_data, MP_EDIT_ADD_TO_PLAYLIST);
	}
	else
		DEBUG_TRACE("layout_data is NULL !!!");
}

void
mp_track_view_add_to_playlist_cancel_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	MP_CHECK(view_data->ad);

	mp_retm_if(view_data->ad->navi_effect_in_progress, "navi effect in progress");

	view_data->ad->b_add_tracks = FALSE;

	Evas_Object *view_layout = mp_view_manager_get_last_view_layout(view_data->ad);
	MP_CHECK(view_layout);

	mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
	mp_view_layout_reset_select_all(layout_data);

	elm_toolbar_item_selected_set(view_data->ad->library->ctltab_plist, EINA_TRUE);
}

Evas_Object *
mp_track_view_create(struct appdata *ad)
{
	DEBUG_TRACE("");

	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	view_data->view_type = MP_VIEW_TYPE_SONGS;
	TA_S(6, "mp_track_view_create");
	Evas_Object *view_layout = mp_view_layout_create(ad->tabbar, view_data, MP_VIEW_MODE_DEFAULT);
	TA_E(6, "mp_track_view_create");

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();

	itc->item_style = "3text.1icon.1";
	itc->decorate_item_style = "mode/slide4";
	itc->func.text_get = mp_common_track_list_label_get;
	itc->func.content_get = mp_common_track_list_icon_get;

	mp_genlist_cb_t genlist_cbs;
	memset(&genlist_cbs, 0, sizeof(mp_genlist_cb_t));
	genlist_cbs.selected_cb = mp_common_track_genlist_sel_cb;

	mp_view_layout_set_layout_data(view_layout,
				       MP_LAYOUT_TRACK_LIST_TYPE, MP_TRACK_ALL,
				       MP_LAYOUT_LIST_CB, &genlist_cbs, MP_LAYOUT_GENLIST_ITEMCLASS, itc, -1);

	mp_layout_data_t *layout_data = NULL;
	layout_data = evas_object_data_get(view_layout, "layout_data");
	MP_CHECK_NULL(layout_data);

	evas_object_data_set(ad->controlbar_layout, "layout_data", layout_data);

	return view_layout;
}

void
mp_track_view_destroy(Evas_Object * track_view)
{
	DEBUG_TRACE("");
	evas_object_del(track_view);
}

void
mp_track_view_refresh(Evas_Object * track_view)
{
	DEBUG_TRACE("");
	view_data_t *view_data = (view_data_t *) evas_object_data_get(track_view, "view_data");
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);
	mp_view_manager_update_list_contents(view_data, TRUE);
}

void
mp_track_view_update_title_button(Evas_Object * track_view)
{
	DEBUG_TRACE("");
	Evas_Object *top_view = NULL;
	Elm_Object_Item *navi_it = NULL;
	view_data_t *view_data = (view_data_t *) evas_object_data_get(track_view, "view_data");
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	navi_it = elm_naviframe_top_item_get(view_data->navibar);
	MP_CHECK(navi_it);
	top_view = elm_object_item_content_get(navi_it);
	MP_CHECK(top_view);

	if (view_data->ad->b_add_tracks)
	{
		mp_view_manager_set_title_and_buttons(view_data, NULL, view_data);
		Evas_Object *view_layout = mp_view_manager_get_last_view_layout(view_data->ad);
		if (view_layout) {
			mp_layout_data_t *layout_data = evas_object_data_get(view_layout, "layout_data");
			if (layout_data)
				mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);
		}
	}
	else
	{
		mp_view_manager_set_title_and_buttons(view_data, NULL, view_data);
	}

}
