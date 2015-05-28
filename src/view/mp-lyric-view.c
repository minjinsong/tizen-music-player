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


#include "mp-lyric-view.h"
#include "mp-lyric-mgr.h"
#include "mp-common.h"
#include "mp-player-mgr.h"
#include "mp-play-view.h"
#include "mp-widget.h"

#include <metadata_extractor.h>

#define MP_LRC_STR_NO_LYRIC ("IDS_MUSIC_SK3_NO_LYRICS")
#define MP_LRC_FOLDER_PATH "/opt/media/Sounds and music/Lyric/"
#define MP_LRC_SIFFIX_NAME ".lrc"
#define MP_LRC_FILE_PATH_LEN (int)255 /* The max length of file path */

#define MP_LRC_LINE_COLOR_DEFAULT "#7e7d7d" /* 126:125:125 */
#define MP_LRC_LINE_COLOR_PLAYING "#f9f9f9" /* 249:249:249 */
#define MP_LRC_FONT_CURRENT_SIZE (int)32 /* The size of current playing line */
#define MP_LRC_FONT_NORMAL_SIZE (int)32 /* The size of normal line */

#define HD_MAIN_W 720
#define HD_MAIN_H 1280

/* Portrait mode */
#define MP_LRC_VIEW_X_SCALE 0/HD_MAIN_W /* The X coordinate of the view */
#define MP_LRC_VIEW_Y_SCALE 454/HD_MAIN_H /* The Y coordinate of the view */
#define MP_LRC_VIEW_H_SCALE 560/HD_MAIN_H /* The hight of lyric view */
#define MP_LRC_VIEW_W_SCALE 592/HD_MAIN_W /* The wigth of lyric view */

/* Landscape mode */
#define MP_LRC_VIEW_H_SCALE_LS 104/HD_MAIN_W
#define MP_LRC_VIEW_W_SCALE_LS 400/HD_MAIN_W
#define MP_LRC_VIEW_Y_SCALE_LS 186/HD_MAIN_W

#define MP_LRC_HEAD_H_SCALE 15/HD_MAIN_H
#define MP_LRC_HEAD_H_SCALE_LS 10/HD_MAIN_W

static void* _mp_lyric_view_get_lyric(struct appdata *ad, int *sync_num);
#ifndef MP_FEATURE_SUPPORT_ID3_TAG
static char* _mp_lyric_view_get_current_music_name(struct appdata *ad);
static char* _mp_lyric_view_get_lyric_path(struct appdata *ad);
#endif
static void _mp_lyric_view_set_line_color(struct appdata *ad, Evas_Object *obj, int index, const char *color);
static const Eina_List* _mp_lyric_view_get_line_list(struct appdata *ad);
static Evas_Object* _mp_lyric_view_get_line(struct appdata *ad, int index);
static void _mp_lyric_view_set_current_line(struct appdata *ad, int index);
static void _mp_lyric_view_drag_start(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mp_lyric_view_drag_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mp_lyric_view_drag_stop(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mp_lyric_view_load_lyric(struct appdata *ad);
static Evas_Object* _mp_lyric_view_create_layout(struct appdata *ad);
static void _mp_lyric_view_init_data(struct appdata *ad);

static void*
_mp_lyric_view_get_lyric(struct appdata *ad, int *sync_num)
{
	void *lrc_data = NULL;
	Eina_List *synclrc_list = NULL;
	char *unsynclrc_buffer = NULL;

	metadata_extractor_h handle = NULL;
	int mmf_error = -1;

	/* Get current playing music */
	mp_plst_item *current_item = NULL;
	current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);;
	MP_CHECK_NULL(current_item);

	const char *musicPath = current_item->uri;
	MP_CHECK_NULL(musicPath);

	mmf_error = metadata_extractor_create(&handle);
	MP_CHECK_NULL(mmf_error == METADATA_EXTRACTOR_ERROR_NONE);

	mmf_error = metadata_extractor_set_path(handle, musicPath);
	if (mmf_error == METADATA_EXTRACTOR_ERROR_NONE)
	{
		int sync_len = 0;
		char *unsynclyrics = NULL;
		int unsync_len = 0;

		char *value = NULL;
		mmf_error = metadata_extractor_get_metadata(handle, METADATA_SYNCLYRICS_NUM, &value);
		if (mmf_error == METADATA_EXTRACTOR_ERROR_NONE && value) {
			sync_len = atoi(value);
		}
		SAFE_FREE(value);

		mmf_error = metadata_extractor_get_metadata(handle, METADATA_UNSYNCLYRICS, &unsynclyrics);
		if (mmf_error == METADATA_EXTRACTOR_ERROR_NONE && unsynclyrics) {
			unsync_len = strlen(unsynclyrics);
		} else {
			DEBUG_TRACE("fail to metadata_extractor_get_metadata() %x", mmf_error);
		}
		/* unsynclyrics must be freed */

		if (sync_len || unsync_len)
		{
			DEBUG_TRACE("sync_len=%d", sync_len);
			*sync_num = sync_len;

			if(sync_len > 0)
			{
				int ret = 0;
				int idx = 0;
				unsigned long time_info = 0;
				char *lyrics_info = NULL;

				for(idx = 0; idx < sync_len ; idx++)
				{
					/*Get one time and lyrics info.*/
					ret = metadata_extractor_get_synclyrics(handle, idx, &time_info, &lyrics_info);
					if(ret == METADATA_EXTRACTOR_ERROR_NONE && lyrics_info != NULL && strlen(lyrics_info) > 0)
					{
						mp_lrc_node_t *new_node = malloc(sizeof(mp_lrc_node_t));
						if (new_node == NULL) {
							if (handle) {
								metadata_extractor_destroy(handle);
								handle = NULL;
							}
							return NULL;
						}
						new_node->time = time_info;
						new_node->lyric = g_strdup(lyrics_info);

						synclrc_list = eina_list_append(synclrc_list, (gpointer)new_node);

						DEBUG_TRACE("[%2d][%6d][%s]", idx, time_info, lyrics_info);
					}
					else
					{
						ERROR_TRACE("Error when get lyrics");
						*sync_num = 0;
						mp_lrc_node_t *node = NULL;
						Eina_List *next = NULL;
						EINA_LIST_FOREACH(synclrc_list, next, node)
						{
							if (node)
							{
								if (node->lyric != NULL)
									free(node->lyric);
								free(node);
							}
						}
						eina_list_free(synclrc_list);
						synclrc_list = NULL;
						break;
					}
				}
			}
			else
			{
				DEBUG_TRACE("unsynclyrics[%d] : %s", unsync_len, unsynclyrics);
				if (unsynclyrics != NULL)
				{
					unsynclrc_buffer = g_strdup(unsynclyrics);
				}
			}

			SAFE_FREE(unsynclyrics);
		}
		else
		{
			DEBUG_TRACE("No lyric infomation");
		}
	}

	if (handle) {
		metadata_extractor_destroy(handle);
		handle = NULL;
	}

	if (*sync_num > 0)
	{
		lrc_data = synclrc_list;
	}
	else
	{
		lrc_data = unsynclrc_buffer;
	}

	return lrc_data;
}

#ifndef MP_FEATURE_SUPPORT_ID3_TAG
static char*
_mp_lyric_view_get_current_music_name(struct appdata *ad)
{
	startfunc;

	MP_CHECK_NULL(ad);

	/* Get current playing music */
	mp_plst_item *current_item = NULL;
	current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);;
	MP_CHECK_NULL(current_item);

	const char *musicPath = current_item->uri;
	MP_CHECK_NULL(musicPath);

	char musicName[MP_LRC_FILE_PATH_LEN] = {'0'};
	int len, start, end;
	const char *p;

	/* Get music name by path */
	start = end = len = strlen(musicPath);
	p = musicPath + len;

	while(*p-- != '.')
	{
		start--;
		end--;
	}

	while(*p-- != '/')
	{
		start--;
	}

	DEBUG_TRACE("len=%d, start=%d, end=%d\n", len, start, end);
	memcpy(musicName, &musicPath[start], end-start);
	DEBUG_TRACE("musicName=%s\n", musicName);

	endfunc;
	return g_strdup(musicName);
}

static char*
_mp_lyric_view_get_lyric_path(struct appdata *ad)
{
	startfunc;

	MP_CHECK_NULL(ad);

	char *musicName = _mp_lyric_view_get_current_music_name(ad);
	MP_CHECK_NULL(musicName);

	char buf[MP_LRC_LINE_BUF_LEN] = {'0'};
	/* Get the lyric path */
	g_strlcpy(buf, MP_LRC_FOLDER_PATH, (gsize)sizeof(buf));
	g_strlcat(buf, musicName, (gsize)sizeof(buf));
	g_strlcat(buf, MP_LRC_SIFFIX_NAME, (gsize)sizeof(buf));

	free(musicName);

	if ( !ecore_file_exists(buf) )
	{
		return NULL;
	}

	if ( ecore_file_is_dir(buf))
	{
		return NULL;
	}

	endfunc;

	return g_strdup(buf);
}
#endif
static void
_mp_lyric_view_set_line_color(struct appdata *ad, Evas_Object *obj, int index, const char *color)
{
	startfunc;

	MP_CHECK(obj);
	MP_CHECK(color);
	MP_CHECK(index >= 0);
	MP_CHECK(ad->lyric_mgr);

	Eina_List *list = ad->lyric_mgr->synclrc_list;
	int count = eina_list_count(list);

	if (count <= 0)
		list = ad->lyric_mgr->unsynclrc_list;

	mp_lrc_node_t *lrc_node = (mp_lrc_node_t*)eina_list_nth(list, index);
	MP_CHECK(lrc_node);
	char *text_old = lrc_node->lyric;
	MP_CHECK(text_old);

	char *text_new = (char*)malloc(sizeof(char)*(strlen(text_old)+50));
	MP_CHECK(text_new);
	strcpy(text_new, "<color=");
	strcat(text_new, color);
	strcat(text_new, ">");
	strcat(text_new, text_old);
	strcat(text_new, "</color>");

	edje_object_part_text_set(_EDJ(obj), "txt", text_new);

	free(text_new);

	endfunc;
}

static const Eina_List*
_mp_lyric_view_get_line_list(struct appdata *ad)
{
	startfunc;
	const Eina_List *list = NULL;

	MP_CHECK_NULL(ad->lyric_view);
	mp_lyric_view_t *lyric_view = ad->lyric_view;

	Evas_Object *scroller =  lyric_view->scroller;
	Evas_Object *content = elm_object_content_get(scroller);
	list = elm_box_children_get(content);

	endfunc;

	return list;
}

static Evas_Object*
_mp_lyric_view_get_line(struct appdata *ad, int index)
{
	startfunc;

	MP_CHECK_NULL(index >= 0);

	const Eina_List *list = _mp_lyric_view_get_line_list(ad);
	Evas_Object *line = eina_list_nth(list, index+1);

	endfunc;

	return line;
}

static void
_mp_lyric_view_set_current_line(struct appdata *ad, int index)
{
	startfunc;

	MP_CHECK(ad->lyric_view);
	MP_CHECK(index >= 0);
	MP_CHECK(ad->playing_view);

	DEBUG_TRACE("index=%d\n", index);
	Evas_Object *line = _mp_lyric_view_get_line(ad, index);
	MP_CHECK(line);

	/* No change */
	if (ad->lyric_view->cur_line == line)
	{
		return;
	}

	/* Update current line */
	ad->lyric_view->prev_line = ad->lyric_view->cur_line;
	ad->lyric_view->cur_line = line;

	ad->lyric_view->prev_line_index = ad->lyric_view->cur_line_index;
	ad->lyric_view->cur_line_index = index;

	int x0, y0, w0, h0;
	Evas_Object *head = _mp_lyric_view_get_line(ad, 0);

	evas_object_geometry_get(head, &x0, &y0, &w0, &h0);
	int head_gap = y0 + h0;

	DEBUG_TRACE("===Get current line====i =%d, x=%d, y=%d, w=%d, h=%d\n",index, x0, y0, w0, h0);
	int x, y, w, h;
	evas_object_geometry_get(ad->lyric_view->cur_line, &x, &y, &w, &h);
	DEBUG_TRACE("===Get current line====i =%d, x=%d, y=%d, w=%d, h=%d\n",index, x, y, w, h);

	int new_y = 0;
	if (ad->playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{
		int scale_h_ls = ad->lyric_view->win_w*MP_LRC_VIEW_H_SCALE_LS;
		int scale_w_ls = ad->lyric_view->win_h*MP_LRC_VIEW_W_SCALE_LS;
		int scale_head_h_ls = ad->lyric_view->win_w*MP_LRC_HEAD_H_SCALE_LS;

		if ((y-y0) > (scale_h_ls/2 - scale_head_h_ls))
		{
			new_y = y-head_gap+(h)-(scale_h_ls/2 - scale_head_h_ls);
		}
		else
		{
			new_y = y-head_gap+(h/2)-(y-y0);
		}

		elm_scroller_region_bring_in(ad->lyric_view->scroller, x, new_y, scale_w_ls, scale_h_ls);
	}
	else
	{
		int scale_h = ad->lyric_view->win_h*MP_LRC_VIEW_H_SCALE;
		int scale_w = ad->lyric_view->win_w*MP_LRC_VIEW_W_SCALE;
		int scale_head_h = ad->lyric_view->win_h*MP_LRC_HEAD_H_SCALE;

		if ((y-y0) > (scale_h/2 - scale_head_h))
		{
			new_y = y-head_gap+(h)-(scale_h/2 - scale_head_h);
		}
		else
		{
			new_y = y-head_gap+(h/2)-(y-y0);
		}

		elm_scroller_region_bring_in(ad->lyric_view->scroller, x, new_y, scale_w, scale_h);
	}

	_mp_lyric_view_set_line_color(ad, ad->lyric_view->cur_line, ad->lyric_view->cur_line_index, MP_LRC_LINE_COLOR_PLAYING);

	if (ad->lyric_view->prev_line != NULL)
	{
		_mp_lyric_view_set_line_color(ad, ad->lyric_view->prev_line, ad->lyric_view->prev_line_index, MP_LRC_LINE_COLOR_DEFAULT);
	}

	evas_object_show(ad->lyric_view->scroller);

	endfunc;
}

static void
_mp_lyric_view_drag_start(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	startfunc;

	struct appdata *ad = data;

	MP_CHECK(data);
	MP_CHECK(ad->lyric_view);

	ad->lyric_view->b_drag = EINA_TRUE;

	if(ad->player_state == PLAY_STATE_PLAYING)
	{
		//mp_player_mgr_pause(ad);
		//ad->player_state = PLAY_STATE_PLAYING;
	}

	endfunc;
}

static void
_mp_lyric_view_drag_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	startfunc;

	struct appdata *ad = data;

	MP_CHECK(ad);
	MP_CHECK(ad->lyric_view);

	endfunc;
}

static void
_mp_lyric_view_drag_stop(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	startfunc;

	struct appdata *ad = data;

	MP_CHECK(data);
	MP_CHECK(ad->lyric_view);

	ad->lyric_view->b_drag = EINA_FALSE;

	if(ad->player_state == PLAY_STATE_PLAYING)
	{
		//mp_player_mgr_resume(ad);
	}

	endfunc;
}

static void
_mp_lyric_view_load_lyric(struct appdata *ad)
{
	startfunc;

	MP_CHECK(ad);
	MP_CHECK(ad->playing_view);
	MP_CHECK(ad->lyric_view);
	MP_CHECK(ad->lyric_mgr);
	MP_CHECK(ad->lyric_view->scroller);
	MP_CHECK(ad->lyric_view->box);

	/* Create lyric line */
	int index = 0;
	Eina_List *list = ad->lyric_mgr->synclrc_list;
	int count = eina_list_count(list);

	if (count <= 0)
	{
		list = ad->lyric_mgr->unsynclrc_list;
		count = eina_list_count(list);
	}

	for (index = 0; index < count; index++)
	{
		mp_lrc_node_t *lrc_node = (mp_lrc_node_t*)eina_list_nth(list, index);
		MP_CHECK(lrc_node);
		DEBUG_TRACE("line%d=%s\n", index, lrc_node->lyric);
		Evas_Object* line = mp_common_load_edj(ad->playing_view->layout, PLAY_VIEW_EDJ_NAME, "mp_lrc_line");
		edje_object_part_text_set(_EDJ(line), "txt", lrc_node->lyric);

		evas_object_show(line);
		evas_object_data_set(line, "time", &lrc_node->time);
		elm_box_pack_end(ad->lyric_view->box, line);
	}

	/* Add gap for head and tail */
	if (ad->playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{
	    Evas_Object* head = mp_common_load_edj(ad->playing_view->layout, PLAY_VIEW_EDJ_NAME, "mp_lrc_head_landscape");
	    elm_box_pack_start(ad->lyric_view->box, head);
	}
	else
	{
	    Evas_Object* head = mp_common_load_edj(ad->playing_view->layout, PLAY_VIEW_EDJ_NAME, "mp_lrc_head");
	    elm_box_pack_start(ad->lyric_view->box, head);
	}

	endfunc;
}

static Evas_Object*
_mp_lyric_view_create_layout(struct appdata *ad)
{
	startfunc;

	Evas_Object *layout = NULL;

	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(ad->lyric_view);

	/* Create layout */
	MP_CHECK_NULL(ad->playing_view);
	if (ad->playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{
		layout = mp_common_load_edj(ad->playing_view->layout, PLAY_VIEW_EDJ_NAME, "mp_lrc_layout_landscape");
	}
	else
	{
		layout = mp_common_load_edj(ad->playing_view->layout, PLAY_VIEW_EDJ_NAME, "mp_lrc_layout");
	}
	evas_object_show(layout);
	ad->lyric_view->layout = layout;

	/* Create scroller */
	Evas_Object *scroller = elm_scroller_add(ad->playing_view->layout);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller,ELM_SCROLLER_POLICY_OFF,ELM_SCROLLER_POLICY_ON);
	evas_object_show(scroller);
	ad->lyric_view->scroller = scroller;

	/* Create box */
	Evas_Object *box = elm_box_add(ad->playing_view->layout);
	evas_object_show(box);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	ad->lyric_view->box = box;

	/* Set layout content */
	elm_object_content_set(scroller, box);
	elm_object_part_content_set(layout, "lyric", scroller);
	elm_object_part_content_set(ad->playing_view->layout, "lrc", layout);

	/* Set event callback */
	//evas_object_smart_callback_add(ad->lyric_view->scroller, "scroll", _mp_lyric_view_scroll_cb, ad);
	evas_object_event_callback_add(ad->lyric_view->scroller, EVAS_CALLBACK_MOUSE_DOWN, _mp_lyric_view_drag_start, ad);
	evas_object_event_callback_add(ad->lyric_view->scroller, EVAS_CALLBACK_MOUSE_MOVE, _mp_lyric_view_drag_move, ad);
	evas_object_event_callback_add(ad->lyric_view->scroller, EVAS_CALLBACK_MOUSE_UP, _mp_lyric_view_drag_stop, ad);

	endfunc;

	return layout;
}

static void
_mp_lyric_view_init_data(struct appdata *ad)
{
	startfunc;

	MP_CHECK(ad);

	if (ad->lyric_view != NULL)
	{
		mp_lyric_view_destroy(ad);
	}

	/* Create view data */
	ad->lyric_view = (mp_lyric_view_t*)malloc(sizeof(mp_lyric_view_t));
	MP_CHECK(ad->lyric_view);
	memset(ad->lyric_view, 0, sizeof(mp_lyric_view_t));

	/* Create lyric layout */
	ad->lyric_view->layout = _mp_lyric_view_create_layout(ad);
	elm_box_clear(ad->lyric_view->box);

	/* Get window size */
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &ad->lyric_view->win_w, &ad->lyric_view->win_h);

	endfunc;
}

void
mp_lyric_view_refresh(struct appdata *ad)
{
	MP_CHECK(ad);
	if(!ad->lyric_view)
		return;

	if (ad->lyric_view->b_drag)
		return;

	if (ad->lyric_mgr != NULL)
	{
		Eina_List *list = ad->lyric_mgr->synclrc_list;
		int count = eina_list_count(list);
		int pos = mp_player_mgr_get_position();
		int index = 0;

		/* Get the current playing line */
		for (index = 0; index < count-1; index++)
		{
			mp_lrc_node_t *node1 = (mp_lrc_node_t*)eina_list_nth(list, index);
			mp_lrc_node_t *node2 = (mp_lrc_node_t*)eina_list_nth(list, index+1);
			MP_CHECK(node1);
			MP_CHECK(node2);

			if ((pos >= node1->time) && (pos < node2->time))
			{
				_mp_lyric_view_set_current_line(ad, index);
				break;
			}
		}
	}

	endfunc;
}

void
mp_lyric_view_hide(struct appdata *ad)
{
	startfunc;

	MP_CHECK(ad);
	MP_CHECK(ad->lyric_view);

	if (ad->lyric_view->layout != NULL)
	{
	        evas_object_hide(ad->lyric_view->layout);
	        Evas_Object *bg = (Evas_Object *)edje_object_part_object_get(_EDJ(ad->lyric_view->layout), "bg");
	        evas_object_hide(bg);
	}

	if (ad->lyric_view->box != NULL)
	{
		evas_object_hide(ad->lyric_view->box);
	}

	if (ad->lyric_view->scroller != NULL)
	{
		evas_object_hide(ad->lyric_view->scroller);
	}

	MP_CHECK(ad->playing_view);
	edje_object_signal_emit(_EDJ(ad->playing_view->layout), "lrc_invisible", "lrc");

	endfunc;
}

void
mp_lyric_view_show(struct appdata *ad)
{
	startfunc;

	MP_CHECK(ad);

	/* The first time show lyric */
	if (ad->lyric_view == NULL)
	{
	        mp_lyric_view_create(ad);
	}

	MP_CHECK(ad->lyric_view);

	/* Show directly if the current playing music is the same as last one, otherwise create again */
#ifdef MP_FEATURE_SUPPORT_ID3_TAG
	/* Get current music path */
	mp_plst_item *current_item = NULL;
	current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);;
	MP_CHECK(current_item);
	MP_CHECK(current_item->uri);

	if (ad->lyric_view->music_path == NULL ||
		((ad->lyric_view->lyric_buffer == NULL) && (ad->lyric_view->synclrc_list == NULL)))
	{
		mp_lyric_view_create(ad);
	}
	else if (strcmp(ad->lyric_view->music_path, current_item->uri))
	{
		mp_lyric_view_create(ad);
	}
#else
	char *lyric_path = _mp_lyric_view_get_lyric_path(ad);
	MP_CHECK(lyric_path);

	if (ad->lyric_view->lyric_path != NULL)
	{
	        if (strcmp(lyric_path, ad->lyric_view->lyric_path))
		{
			mp_lyric_view_create(ad);
		}
	}
	IF_FREE(lyric_path);
#endif

	MP_CHECK(ad->lyric_view);

	if (ad->lyric_view->layout != NULL)
	{
		evas_object_show(ad->lyric_view->layout);
		Evas_Object *bg = (Evas_Object *)edje_object_part_object_get(_EDJ(ad->lyric_view->layout), "bg");
		evas_object_show(bg);
	}

	if (ad->lyric_view->box != NULL)
	{
		evas_object_show(ad->lyric_view->box);
	}

	if (ad->lyric_view->scroller != NULL)
	{
		evas_object_show(ad->lyric_view->scroller);
	}

	MP_CHECK(ad->playing_view);
	edje_object_signal_emit(_EDJ(ad->playing_view->layout), "lrc_visible", "lrc");

	endfunc;
}

void
mp_lyric_view_destroy(struct appdata *ad)
{
	startfunc;

	MP_CHECK(ad);
	MP_CHECK(ad->lyric_view);

	if (ad->lyric_mgr != NULL)
	{
		//mp_lyric_mgr_destory(ad);
	}

        if (ad->lyric_view->layout != NULL)
	{
		evas_object_del(ad->lyric_view->layout);
	}

	if (ad->lyric_view->box != NULL)
	{
		elm_box_clear(ad->lyric_view->box);
		evas_object_del(ad->lyric_view->box);
	}

	if (ad->lyric_view->scroller != NULL)
	{
		evas_object_del(ad->lyric_view->scroller);
	}

#ifdef MP_FEATURE_SUPPORT_ID3_TAG
	if (ad->lyric_view->lyric_buffer != NULL)
	{
	        free(ad->lyric_view->lyric_buffer);
	}
#else
	if (ad->lyric_view->lyric_path != NULL)
	{
	        free(ad->lyric_view->lyric_path);
	}
#endif
	if (ad->lyric_view->music_path != NULL)
	{
		free(ad->lyric_view->music_path);
	}

	free(ad->lyric_view);
	ad->lyric_view = NULL;

	MP_CHECK(ad->playing_view);
	edje_object_signal_emit(_EDJ(ad->playing_view->layout), "lrc_destroy", "lrc");

	endfunc;
}

void
mp_lyric_view_create(struct appdata *ad)
{
	startfunc;

	MP_CHECK(ad);

	/* Get current music path */
	mp_plst_item *current_item = NULL;
	current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);;
	MP_CHECK(current_item);
	MP_CHECK(current_item->uri);

	if (ad->lyric_view != NULL)
	{
	        mp_lyric_view_destroy(ad);
	}

#ifdef MP_FEATURE_SUPPORT_ID3_TAG
	int sync_num = 0;
	void *lrc_data = _mp_lyric_view_get_lyric(ad, &sync_num);

	/* No lyric */
	if (lrc_data == NULL)
	{
		//_mp_lyric_view_show_nolyric(ad);
		return;
	}
#else
	char* lyricPath = _mp_lyric_view_get_lyric_path(ad);

	/* No lyric */
	if (lyricPath == NULL)
	{
		//_mp_lyric_view_show_nolyric(ad);
		return;
	}

	DEBUG_TRACE("lyricPath=%s\n",lyricPath);
#endif

	/* Init view data */
	_mp_lyric_view_init_data(ad);
	MP_CHECK(ad->lyric_view);

	/* Parse lyric file */
#ifdef MP_FEATURE_SUPPORT_ID3_TAG
	if (sync_num > 0)
	{
		ad->lyric_view->synclrc_list = lrc_data;
		mp_lyric_mgr_create(ad, lrc_data, MP_LYRIC_SOURCE_LIST);
	}
	else
	{
		ad->lyric_view->lyric_buffer = lrc_data;
		mp_lyric_mgr_create(ad, lrc_data, MP_LYRIC_SOURCE_BUFFER);
	}
#else
	ad->lyric_view->lyric_path = lyricPath;
	mp_lyric_mgr_create(ad, lyricPath, MP_LYRIC_SOURCE_FILE);
#endif
	SAFE_FREE(ad->lyric_view->music_path);
	ad->lyric_view->music_path = strdup(current_item->uri);

	/* Load lyric view */
	_mp_lyric_view_load_lyric(ad);

	endfunc;
}
