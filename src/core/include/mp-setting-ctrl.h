 /*
  * Copyright 2012	   Samsung Electronics Co., Ltd
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

#ifndef __MP_SETTING_CTRL_H_
#define __MP_SETTING_CTRL_H_

#include <vconf-keys.h>
#ifndef MP_SOUND_PLAYER
#include "mp-view-layout.h"
#endif

typedef void (*MpSettingPlaylist_Cb) (int state, void *data);
typedef void (*MpSettingSaChange_Cb) (int state, void *data);
typedef void (*MpSettingAutoOff_Cb)(int min, void *data);
typedef void (*MpSettingPlaySpeed_Cb)(double speed, void *data);

int mp_setting_init(struct appdata *ad);
int mp_setting_deinit(struct appdata *ad);
int mp_setting_set_shuffle_state(int b_val);
int mp_setting_get_shuffle_state(int *b_val);
int mp_setting_set_repeat_state(int val);
int mp_setting_get_repeat_state(int *val);

int mp_setting_playlist_get_state(int *state);
int mp_setting_playlist_set_callback(MpSettingPlaylist_Cb func, void *data);

void mp_setting_save_now_playing(void *ad);
void mp_setting_save_shortcut(char *shortcut_title, char *artist, char *shortcut_description,
			      char *shortcut_image_path);

void mp_setting_update_active_device();

#endif // __MP_SETTING_CTRL_H_
