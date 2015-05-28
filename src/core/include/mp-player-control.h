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

#ifndef __DEF_music_player_contro_H_
#define __DEF_music_player_contro_H_

#include <Elementary.h>
#include "music.h"

void mp_play_control_play_pause(struct appdata *ad, bool play);
void mp_player_control_stop(struct appdata *ad);
void mp_play_control_ff_cb(void *data, Evas_Object * o, const char *emission, const char *source);
void mp_play_control_rew_cb(void *data, Evas_Object * o, const char *emission, const char *source);
void mp_play_control_volume_down_cb(void *data, Evas_Object * o, const char *emission, const char *source);
void mp_play_control_volume_up_cb(void *data, Evas_Object * o, const char *emission, const char *source);
void mp_play_control_volume_cb(void *data, Evas_Object * o, const char *emission, const char *source);
void mp_play_control_menu_cb(void *data, Evas_Object * o, const char *emission, const char *source);
void mp_play_control_end_of_stream(void *data);
bool mp_player_control_ready_new_file(void *data, bool check_drm);

void mp_play_stop_and_updateview(void *data, bool mmc_removed);
void mp_play_control_play_pause_icon_set(void *data, bool play_enable);
void mp_play_control_shuffle_set(void *data, bool shuffle_enable);
void mp_play_control_repeat_set(void *data, int repeat_state);

#endif /*__DEF_music_player_contro_H_*/
