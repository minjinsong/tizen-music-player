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

#ifndef __MP_STREAMING_MGR_H__
#define __MP_STREAMING_MGR_H__

#include <player.h>

inline void mp_streaming_mgr_buffering_popup_control(struct appdata *ad, bool is_show);

bool mp_streaming_mgr_check_streaming(struct appdata *ad, const char *path);
bool mp_streaming_mgr_set_attribute(struct appdata *ad, player_h player);
bool mp_streaming_mgr_play_new_streaming(struct appdata *ad);

#endif /* __MP_STREAMING_MGR_H__ */
