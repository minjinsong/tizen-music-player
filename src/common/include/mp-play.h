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

#ifndef __MP_PLAY_H_
#define __MP_PLAY_H_

#include <Elementary.h>
#include "mp-define.h"
bool mp_play_item_play(void *data, char *fid);
bool mp_play_current_file(void *data);
bool mp_play_new_file(void *data, bool check_drm);
bool mp_play_new_file_real(void *data, bool check_drm);
void mp_play_prev_file(void *data);
void mp_play_next_file(void *data, bool forced);
void mp_play_start(void *data);
void mp_play_pause(void *data);
void mp_play_stop(void *data);
void mp_play_resume(void *data);
bool mp_play_destory(void *data);
#endif /*__DEF_music_player_contro_H_*/
