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

#ifndef __MP_MINICONTROLLER_H__
#define __MP_MINICONTROLLER_H__

#include "mp-ta.h"
#include "music.h"
#include "mp-item.h"
#include "mp-player-control.h"
#include "mp-play-view.h"
#include "mp-common.h"
#include "mp-minicontroller.h"
#include "mp-player-mgr.h"
#include "mp-player-debug.h"

int mp_minicontroller_create(struct appdata *ad);
int mp_minicontroller_hide(struct appdata *ad);
int mp_minicontroller_show(struct appdata *ad);
int mp_minicontroller_destroy(struct appdata *ad);
void mp_minicontroller_update_control(struct appdata *ad);
void mp_minicontroller_update(struct appdata *ad);
void mp_minicontroller_rotate(struct appdata *ad, int angle);
void mp_minicontroller_visible_set(struct appdata *ad, bool visible);

#endif
