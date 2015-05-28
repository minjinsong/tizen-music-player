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
#include "ug-mp-library-view.h"
#include "ug-mp-track-list.h"
#include "ug-mp-group-list.h"
#include "ug-mp-common.h"

typedef struct{
	struct ug_data *ugd;

	Evas_Object *toolbar;
	Evas_Object *list;

}lib_view_data_t;

enum{
	TAB_ALL,
	TAB_PLAYLIST,
	TAB_ALBUM,
	TAB_ARTIST,
	TAB_MAX,
};

static Elm_Object_Item *g_tab_item[TAB_MAX];
static Evas_Object *g_ly;
static Elm_Object_Item *g_navi_it;

static void _all_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	struct ug_data *ugd = data;
	Evas_Object *sub_view;

	MP_CHECK(ugd);

	sub_view = elm_object_part_content_unset(g_ly, "elm.swallow.content");
	evas_object_del(sub_view);

	sub_view = ug_mp_track_list_create(g_ly, ugd, g_navi_it);
	ug_mp_track_list_update(sub_view);

	elm_object_part_content_set(g_ly, "elm.swallow.content", sub_view);
	evas_object_show(sub_view);
}

static void _playlist_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	struct ug_data *ugd = data;
	Evas_Object *sub_view;

	MP_CHECK(ugd);

	sub_view = elm_object_part_content_unset(g_ly, "elm.swallow.content");
	evas_object_del(sub_view);

	sub_view = ug_mp_group_list_create(g_ly, ugd);
	ug_mp_group_list_set_data(sub_view, MP_GROUP_BY_PLAYLIST, NULL);
	ug_mp_group_list_update(sub_view);

	elm_object_part_content_set(g_ly, "elm.swallow.content", sub_view);
	evas_object_show(sub_view);
}

static void _artist_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	struct ug_data *ugd = data;
	Evas_Object *sub_view;

	MP_CHECK(ugd);

	sub_view = elm_object_part_content_unset(g_ly, "elm.swallow.content");
	evas_object_del(sub_view);

	sub_view = ug_mp_group_list_create(g_ly, ugd);
	ug_mp_group_list_set_data(sub_view, MP_GROUP_BY_ARTIST, NULL);
	ug_mp_group_list_update(sub_view);

	elm_object_part_content_set(g_ly, "elm.swallow.content", sub_view);
	evas_object_show(sub_view);
}

static void _album_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	struct ug_data *ugd = data;
	Evas_Object *sub_view;

	MP_CHECK(ugd);

	sub_view = elm_object_part_content_unset(g_ly, "elm.swallow.content");
	evas_object_del(sub_view);

	sub_view = ug_mp_group_list_create(g_ly, ugd);
	ug_mp_group_list_set_data(sub_view, MP_GROUP_BY_ALBUM, NULL);
	ug_mp_group_list_update(sub_view);

	elm_object_part_content_set(g_ly, "elm.swallow.content", sub_view);
	evas_object_show(sub_view);
}


static Evas_Object *_create_tabbar(Evas_Object *parent, struct ug_data *ugd)
{
	Evas_Object *obj;

	/* create toolbar */
	obj = elm_toolbar_add(parent);
	if(obj == NULL) return NULL;
	elm_toolbar_shrink_mode_set(obj, ELM_TOOLBAR_SHRINK_EXPAND);
	elm_toolbar_reorder_mode_set(obj, EINA_FALSE);
	elm_toolbar_transverse_expanded_set(obj, EINA_TRUE);

	elm_object_signal_emit(parent, "elm,state,default,tabbar", "elm");
	elm_object_style_set(obj, "tabbar");

	g_tab_item[TAB_ALL] = elm_toolbar_item_append(obj, NULL, GET_STR(UG_MP_TEXT_ALL), _all_cb, ugd);
	g_tab_item[TAB_PLAYLIST] = elm_toolbar_item_append(obj, NULL, GET_STR(UG_MP_TEXT_PLAYLISTS), _playlist_cb, ugd);
	g_tab_item[TAB_ALBUM] = elm_toolbar_item_append(obj, NULL, GET_STR(UG_MP_TEXT_ALBUMS), _album_cb, ugd);
	g_tab_item[TAB_ARTIST] = elm_toolbar_item_append(obj, NULL, GET_STR(UG_MP_TEXT_ARTISTS), _artist_cb, ugd);

	elm_toolbar_select_mode_set(obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
	elm_toolbar_item_selected_set(g_tab_item[TAB_ALL], EINA_TRUE);

	return obj;
}

void
ug_mp_library_view_create(struct ug_data *ugd)
{
	startfunc;

	Evas_Object *tabbar;

	{
		g_ly = elm_layout_add(ugd->navi_bar);
		elm_layout_theme_set(g_ly, "layout", "tabbar", "default");

		Evas_Object *btn = elm_button_add(ugd->navi_bar);
	        elm_object_style_set(btn, "naviframe/back_btn/default");

		evas_object_smart_callback_add(btn, "clicked", ug_mp_quit_cb, ugd);

		g_navi_it = elm_naviframe_item_push(ugd->navi_bar, NULL, btn, NULL, g_ly, NULL);
		elm_naviframe_item_title_visible_set(g_navi_it, EINA_FALSE);

		tabbar = _create_tabbar(g_ly, ugd);
		elm_object_part_content_set(g_ly, "elm.swallow.tabbar", tabbar);

	}
}

