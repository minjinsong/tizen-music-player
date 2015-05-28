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

#include "mp-define.h"
#include "sp-view-manager.h"
#include "mp-widget.h"

struct _Sp_View_Manager {
	Evas_Object *navi;
	GList *view_history;
};

typedef struct {
	int index;
	Elm_Object_Item *navi_item;
	Sp_View_Type type;
} Sp_View_Data;


Sp_View_Manager*
sp_view_mgr_create(Evas_Object *navi)
{
	startfunc;
	MP_CHECK_NULL(navi);

	Sp_View_Manager *view_mgr = (Sp_View_Manager *)calloc(1, sizeof(Sp_View_Manager));
	mp_assert(view_mgr);

	view_mgr->navi = navi;

	return view_mgr;
}

void
sp_view_mgr_destroy(Sp_View_Manager* view_mgr)
{
	startfunc;
	MP_CHECK(view_mgr);

	if (view_mgr->view_history) {
		GList *current = view_mgr->view_history;
		while(current) {
			IF_FREE(current->data);
			current = current->next;
		}

		g_list_free(view_mgr->view_history);
		view_mgr->view_history = NULL;
	}


	IF_FREE(view_mgr);
}

Evas_Object*
sp_view_mgr_get_naviframe(Sp_View_Manager *view_mgr)
{
	MP_CHECK_NULL(view_mgr);
	return view_mgr->navi;
}

void
sp_view_mgr_push_view_content(Sp_View_Manager *view_mgr, Evas_Object *content, Sp_View_Type type)
{
	startfunc;
	MP_CHECK(view_mgr);
	MP_CHECK(view_mgr->navi);
	MP_CHECK(content);

	Sp_View_Data *view_data = calloc(1, sizeof(Sp_View_Data));
	mp_assert(view_data);

	const char *item_style = NULL;
	Eina_Bool title_visible = EINA_TRUE;
	if (type == SP_VIEW_TYPE_PLAY) {
		item_style = "1line/music";
		title_visible = EINA_FALSE;
	}

	view_data->navi_item = elm_naviframe_item_push(view_mgr->navi, NULL, NULL, NULL, content,  item_style);
	elm_naviframe_item_title_visible_set(view_data->navi_item, title_visible);
	view_data->type = type;
	view_data->index = g_list_length(view_mgr->view_history);

	view_mgr->view_history = g_list_append(view_mgr->view_history, view_data);
}

void
sp_view_mgr_pop_view_content(Sp_View_Manager *view_mgr, bool pop_to_first)
{
	startfunc;
	MP_CHECK(view_mgr);
	MP_CHECK(view_mgr->navi);

	GList *last = g_list_last(view_mgr->view_history);
	MP_CHECK(last);
	Sp_View_Data *view_data = last->data;
	MP_CHECK(view_data);

	if (pop_to_first) {
		Elm_Object_Item *bottom_item = elm_naviframe_bottom_item_get(view_mgr->navi);
		if (bottom_item)
			elm_naviframe_item_pop_to(bottom_item);

		while(view_data && view_data->index > 0) {
			SAFE_FREE(view_data);
			view_mgr->view_history = g_list_delete_link(view_mgr->view_history, last);
			last = g_list_last(view_mgr->view_history);
			if (last)
				view_data = last->data;
		}
	} else {
		elm_naviframe_item_pop(view_mgr->navi);
		IF_FREE(view_data);
		view_mgr->view_history = g_list_delete_link(view_mgr->view_history, last);
	}

	if (g_list_length(view_mgr->view_history) == 0) {
		g_list_free(view_mgr->view_history);
		view_mgr->view_history = NULL;
	}
}

void
sp_view_mgr_pop_view_to(Sp_View_Manager *view_mgr, Sp_View_Type type)
{
	MP_CHECK(view_mgr);
	MP_CHECK(view_mgr->view_history);

	GList *last = g_list_last(view_mgr->view_history);
	MP_CHECK(last);
	Sp_View_Data *view_data = last->data;
	MP_CHECK(view_data);

	while(view_data) {
		if (view_data->type == type)
			break;

		SAFE_FREE(view_data);
		view_mgr->view_history = g_list_delete_link(view_mgr->view_history, last);
		last = g_list_last(view_mgr->view_history);
		if (last)
			view_data = last->data;
	}

	MP_CHECK(view_data);

	if (view_data->navi_item)
		elm_naviframe_item_pop_to(view_data->navi_item);
}

Elm_Object_Item*
sp_view_mgr_get_play_view_navi_item(Sp_View_Manager *view_mgr)
{
	MP_CHECK_NULL(view_mgr);
	MP_CHECK_NULL(view_mgr->view_history);

	GList *current = view_mgr->view_history;
	Sp_View_Data *view_data = NULL;
	while(current) {
		view_data = current->data;
		if (view_data && view_data->type == SP_VIEW_TYPE_PLAY)
			return view_data->navi_item;

		current = current->next;
	}

	return NULL;
}

void
sp_view_mgr_play_view_title_label_set(Sp_View_Manager *view_mgr, const char *title)
{
	MP_CHECK(view_mgr);

	Elm_Object_Item *navi_item = sp_view_mgr_get_play_view_navi_item(view_mgr);
	if (navi_item)
		elm_object_item_text_set(navi_item, title);
}

void
sp_view_mgr_set_title_label(Sp_View_Manager *view_mgr, const char *title)
{
	MP_CHECK(view_mgr);

	Evas_Object *navi = sp_view_mgr_get_naviframe(view_mgr);
	MP_CHECK(navi);

	Elm_Object_Item *navi_item = elm_naviframe_top_item_get(navi);
	if (navi_item)
		elm_object_item_text_set(navi_item, title);
}

void
sp_view_mgr_set_title_visible(Sp_View_Manager *view_mgr, bool flag)
{
	MP_CHECK(view_mgr);

	Evas_Object *navi = sp_view_mgr_get_naviframe(view_mgr);
	MP_CHECK(navi);

	Elm_Object_Item *navi_item = elm_naviframe_top_item_get(navi);
	if (navi_item)
		elm_naviframe_item_title_visible_set(navi_item, flag);
}

void
sp_view_mgr_set_back_button(Sp_View_Manager *view_mgr, Evas_Smart_Cb cb, void *data)
{
	MP_CHECK(view_mgr);

	Evas_Object *navi = view_mgr->navi;
	MP_CHECK(navi);
	Elm_Object_Item *navi_item = elm_naviframe_top_item_get(navi);
	MP_CHECK(navi_item);

	Evas_Object *button = NULL;
	if(cb)
	{
		button = mp_widget_create_button(navi, "naviframe/back_btn/default", NULL, NULL, cb, data);
		elm_object_item_part_content_set(navi_item, ELM_NAVIFRAME_ITEM_PREV_BTN, button);
	}
	else
	{
		elm_object_item_part_content_set(navi_item, ELM_NAVIFRAME_ITEM_PREV_BTN, NULL);
	}
}

