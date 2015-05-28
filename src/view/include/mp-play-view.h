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

#ifndef __MP_PLAY_VIEW_H_
#define __MP_PLAY_VIEW_H_

#include "music.h"
#ifndef MP_SOUND_PLAYER
#include "mp-view-layout.h"
#endif

typedef enum
{
	MP_PLAYING_VIEW_TOP_LEFT,
	MP_PLAYING_VIEW_TOP_CENTER,
	MP_PLAYING_VIEW_TOP_RIGHT,
	MP_PLAYING_VIEW_BOTTOM_LEFT,
	MP_PLAYING_VIEW_BOTTOM_CENTER,
	MP_PLAYING_VIEW_BOTTOM_RIGHT,
} mp_playing_view_bg_capture_mode;

typedef struct
{
	//EDJ Layout
	Evas_Object *layout;
	Evas_Object *content;
	Evas_Object *play_view;
	Evas_Object *play_control;
	Evas_Object *play_menu;	//information, srs, shuffle, repeat
	Evas_Object *play_info;
	Evas_Object *play_progressbar;
	Evas_Object *play_ctrl;	//progress bar and index, track name
	Evas_Object *land_naviframe;
	Elm_Object_Item* play_head;
	Evas_Object *play_title;

	Evas_Object *albumart_img;
	Evas_Object *albumart_bg;
	int flick_direction;	//1 for right, 2 for left, other for invalid
	Evas_Object *play_view_bg;
	Evas_Object *play_options;
	Evas_Object *bg_albumart_bg;
	Evas_Object *dmr_button;
	Evas_Object *snd_button;
	mp_playing_view_bg_capture_mode mode;
	Evas_Object *play_view_bg_next;
	Evas_Object *play_view_next;
	char music_play_time_text[16];
	int x;
	int y;
	int favour_longpress;
	Ecore_Timer *favourite_timer;
	Ecore_Timer *progressbar_timer;

	Ecore_Timer *show_ctrl_timer;

	bool transition_state;	//if user start transiton start, set that value if finish transiton reset it

	mp_screen_mode play_view_screen_mode;
	Evas_Object *play_icon;

	bool b_play_all; /* for update playlist view when back from playview which created by playall */
	Evas_Object *volume_widget;
	Ecore_Timer *volume_widget_timer;
} mp_playing_view;

bool mp_play_view_set_screen_mode(void *data, int mode);
bool mp_play_view_stop_transit(struct appdata *ad);

bool mp_play_view_pop(void *data);
bool mp_play_view_load(void *data);

#ifdef MP_SOUND_PLAYER
void mp_play_view_unswallow_info_ug_layout(struct appdata *ad);
#else
bool mp_play_view_load_by_path(struct appdata *ad, char *path);
bool mp_play_view_load_and_play(struct appdata *ad, char *prev_item_uid, bool effect_value);
#endif
bool mp_play_view_refresh(void *data);	// use for refresh playing view

void mp_play_view_volume_widget_show(void *data, bool volume_clicked);
bool mp_play_view_create(void *data);
bool mp_play_view_destory(void *data);

bool mp_play_view_play_item(void *data, mp_plst_item *it, bool effect_value, bool move_left);

void mp_play_view_update_progressbar(void *data);
void mp_play_view_progress_timer_freeze(void *data);
void mp_play_view_progress_timer_thaw(void *data);
void mp_play_view_set_snd_path_sensitivity(void *data);
void mp_play_view_info_cb(void *data, Evas_Object * obj, void *event_info);
void mp_play_view_back_clicked_cb(void *data, Evas_Object * obj, void *event_info);

bool mp_play_view_load_by_voice_ui(struct appdata *ad, const char *request_title);
void mp_play_view_info_back_cb(void *data, Evas_Object * obj, void *event_info);
void mp_play_view_show_default_info(struct appdata *ad);
void mp_play_view_update_snd_path(struct appdata *ad);
void mp_play_view_update_dmr_icon_state(struct appdata *ad);
bool mp_play_view_set_menu_state(void *data, bool show, bool animation);
void mp_play_view_set_snd_path_cb(void *data, Evas_Object *obj, void *event_info);


#endif
