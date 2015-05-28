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

#ifndef __MP_VIEW_LAYOUT_H_
#define __MP_VIEW_LAYOUT_H_

#include "music.h"
#include "mp-define.h"

typedef void (*genlist_cb_t) (void *, Evas_Object *, void *);

typedef enum
{
	MP_LAYOUT_TRACK_LIST,
	MP_LAYOUT_GROUP_LIST,
	MP_LAYOUT_PLAYLIST_LIST,


} mp_list_category_t;

typedef enum
{
	MP_EDIT_DELETE,
	MP_EDIT_ADD_TO_PLAYLIST,
} mp_edit_operation_t;

enum
{
	MP_LAYOUT_CATEGORY_TYPE = 1,	//track list or group list or playlist
	MP_LAYOUT_TRACK_LIST_TYPE,	//mp_track_type_e
	MP_LAYOUT_GROUP_LIST_TYPE,	//mp_group_type_e

	MP_LAYOUT_PLAYLIT_ID,
	MP_LAYOUT_TYPE_STR,	//type_str for db query
	MP_LAYOUT_TYPE_STR2,	//type_str for db query
	MP_LAYOUT_FILTER_STR,
	MP_LAYOUT_GENLIST_ITEMCLASS,	//item class of genlist
	MP_LAYOUT_GENLIST_AUTO_PLAYLIST_ITEMCLASS,	//item class of genlist
	MP_LAYOUT_LIST_CB,
	MP_LAYOUT_EDIT_MODE,
	MP_LAYOUT_REORDER_MODE,
	MP_LAYOUT_PLST_HANDLE_TYPE,
};

typedef struct
{
	genlist_cb_t selected_cb;
	genlist_cb_t auto_playlist_cb;
} mp_genlist_cb_t;

typedef enum
{
	MP_SQUARE_VIEW_GENGRID,
	MP_SQUARE_VIEW_GENLIST
} mp_square_view_type_e;

typedef struct
{
	int magic;
	Evas_Object *navibar;
	mp_view_type_t view_type;
	struct appdata *ad;	//appdata
} view_data_t;

typedef struct
{
	int magic;
	mp_list_category_t category;
	mp_track_type_e track_type;
	mp_group_type_e group_type;

	int playlist_id;
	mp_media_info_h playlist_handle;
	mp_media_list_h playlists;
	char *type_str;
	char *type_str2;
	char *filter_str;

	int edit_mode;
	int reorder;
	bool reordered;
	int add_to_plst;
	int rename_mode;

	int b_loading;

	int item_count;
	int track_count;	//for create playing list from search result.

	mp_media_list_h svc_handle;
	mp_media_list_h track_handle;	//for search
	mp_media_list_h artist_handle;	//for search
	mp_media_list_h album_handle;	//for search
	mp_media_list_h default_playlists;

	Evas_Object *layout;
	Evas_Object *genlist;
	Evas_Object *search_bar;
	Evas_Object *now_playing;
	Evas_Object *box;	//content of confomant, genlist or sentinal should be contained in it.
	Evas_Object *index_fast;
	Evas_Object *sentinel;
	Evas_Object *isf_entry;

	Ecore_IMF_Context *imf_context;

	Elm_Genlist_Item_Class *itc;
	Elm_Genlist_Item_Class auto_playlist_item_class;
	Elm_Object_Item *selected_it;	//Selected genlist item;

	struct appdata *ad;

	mp_genlist_cb_t cb_func;

	Ecore_Idler *search_idler_handle;

	view_data_t *view_data;

	char *old_name;
	Elm_Object_Item *rename_git;

	mp_view_mode_t view_mode;	// to classify edit & search view.
	Elm_Object_Item *search_group_git;	//group item for search list.

	Evas_Object *now_playing_icon;
	Evas_Object *now_playing_progress;
	Ecore_Timer *progress_timer;

	// append item in idler callback.
	char *fast_scrooll_index;
	Ecore_Idler *load_item_idler;
	Ecore_Idler *block_size_idler;

	// genlist edit mode
	Evas_Object *select_all_layout;
	Evas_Object *select_all_checkbox;
	Eina_Bool select_all_checked;
	int checked_count;

	//mp_plst_type_t plst_handle_type;

	// support cancel while processing edit function.
	Ecore_Idler *edit_idler;
	Elm_Object_Item *current_edit_item;
	int selected_count;	//use to store checked count when editing strared.
	int error_count;
	mp_media_list_h group_track_handle;	//for deleting group items
	int edit_track_index;	//for deleting group items
	bool group_item_delete_error;	//for deleting group items
	mp_edit_operation_t edit_operation;
	int playlist_track_count;	//number of tracks in playlist..
	int edit_playlist_id;	//playlist_id  tracks to be added.
	mp_playlist_h edit_playlist_handle;

	char *navibar_title;
	void *callback_data;
	int default_playlist_count;

	bool album_delete_flag; //flag for delete album item

	Elm_Object_Item *album_group;	//group item for album track view.

	Ecore_Thread *edit_thread;
} mp_layout_data_t;

typedef enum
{
	MP_GENLIST_ITEM_TYPE_NORMAL	= 0,
	MP_GENLIST_ITEM_TYPE_GROUP_TITLE,
} mp_genlist_item_type;

typedef struct
{
	int index;
	mp_genlist_item_type item_type;

	Elm_Object_Item *it;	// Genlist Item pointer
	Eina_Bool checked;	// Check status
	mp_group_type_e group_type;	//use this to classify ablum or artist when group item seleted in search view.
	mp_media_info_h handle;
	bool unregister_lang_mgr;

	char *group_title_text_ID;
} mp_genlist_item_data_t;

#define MP_LAYOUT_DATA_MAGIC	0x810522bb
#define MP_SET_LAYOUT_DATA_MAGIC(layout_data)	((mp_layout_data_t *)layout_data)->magic = MP_LAYOUT_DATA_MAGIC
#define MP_CHECK_LAYOUT_DATA(layout_data)	\
do {                                                  \
	if (((mp_layout_data_t *)layout_data)->magic != MP_LAYOUT_DATA_MAGIC) {        \
		ERROR_TRACE("\n###########      ERROR   CHECK  #############\nPARAM is not layout data\n###########      ERROR   CHECK  #############\n"); \
		mp_assert(FALSE);}            \
} while (0)

Evas_Object *mp_view_layout_create(Evas_Object * parent, view_data_t * view_data, mp_view_mode_t view_mode);
void mp_view_layout_destroy(Evas_Object * view_layout);
void mp_view_layout_update(Evas_Object * view_layout);
int mp_view_layout_get_count(Evas_Object * view_layout);
void mp_view_layout_set_layout_data(Evas_Object * view_layout, ...);
void mp_view_layout_clear(Evas_Object * view_layout);

void mp_view_layout_show_now_playing(Evas_Object * view_layout);
void mp_view_layout_hide_now_playing(Evas_Object * view_layout);
void mp_view_layout_set_now_playing_info(Evas_Object * view_layout);
void mp_view_layout_reset_select_all(mp_layout_data_t * layout_data);

void mp_view_layout_progress_timer_thaw(Evas_Object * view_layout);
void mp_view_layout_progress_timer_freeze(Evas_Object * view_layout);
void mp_view_layout_set_edit_mode(mp_layout_data_t * layout_data, bool edit_mode);
void mp_view_layout_search_changed_cb(void *data, Evas_Object * obj, void *event_info);

#endif
