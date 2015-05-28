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

#ifndef __MP_LIBRARY_H_
#define __MP_LIBRARY_H_

#include "music.h"
#include "mp-view-manager.h"
#include "mp-setting-ctrl.h"

#define     STR_MP_ALL_TRACKS         	("IDS_MUSIC_TAB4_ALL")
#define     STR_MP_PLAYLISTS			("IDS_MUSIC_BODY_PLAYLISTS")
#define     STR_MP_ALBUMS			("IDS_MUSIC_TAB4_ALBUMS")
#define     STR_MP_ARTISTS			("IDS_MUSIC_TAB4_ARTISTS")
#define     STR_MP_GENRES			("IDS_MUSIC_TAB4_GENRES")
#define     STR_MP_COMPOSERS		("IDS_MUSIC_BUTTON_COMPOSERS")
#define     STR_MP_CONDUCTORS		("IDS_MUSIC_TAB4_CONDUCTORS")
#define     STR_MP_YEARS				("IDS_MUSIC_TAB4_YEARS")
#define     STR_MP_PODCASTS			("IDS_MUSIC_TAB4_PODCASTS")
#define     STR_MP_FOLDERS			("IDS_COM_BODY_FOLDERS")
#define     STR_MP_SQUARE			("IDS_MUSIC_TAB4_MUSIC_SQUARE")
#define     STR_MP_MOST_PLAYED		("IDS_MUSIC_BODY_MOST_PLAYED")
#define     STR_MP_RECENTLY_ADDED	("IDS_MUSIC_BODY_RECENTLY_ADDED")
#define     STR_MP_RECENTLY_PLAYED	("IDS_MUSIC_BODY_RECENTLY_PLAYED")
#define     STR_MP_QUICK_LIST           	("IDS_MUSIC_BODY_FAVOURITES")
#define     STR_MP_ADD_TO_PLAYLIST  ("IDS_MUSIC_BODY_ADD_TO_PLAYLIST")
#define		STR_MP_ALLSHARE_TAB		("Nearby device")

void mp_library_create(struct appdata *ad);
void mp_library_update_view(struct appdata *ad);
void mp_library_now_playing_hide(struct appdata *ad);
void mp_library_now_playing_set(struct appdata *ad);
bool mp_library_load(struct appdata *ad);
void mp_view_manager_clear(struct appdata *ad);

void mp_library_view_change_cb(void *data, Evas_Object * obj, void *event_info);


#endif //__MP_LIBRARY_H_
