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


#ifndef __MP_DEFINE_H_
#define __MP_DEFINE_H_

#include <Elementary.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <glib.h>
#include <vconf.h>
#include "mp-media-info.h"
#include <Ecore_IMF.h>
#include <Ecore_X.h>
#include <Edje.h>
#include <errno.h>
#include <libintl.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <sys/times.h>

#include "mp-player-debug.h"
#include "mp-ta.h"
#include "mp-vconf-private-keys.h"
#include "mp-images.h"
#include "mp-common-defs.h"

#ifndef MP_SQUARE_FEATURE
#define MP_SQUARE_FEATURE
#endif

#ifdef MP_SOUND_PLAYER
 #undef MP_SQUARE_FEATURE
#endif

#ifndef bool
#define bool Eina_Bool
#endif

#define GET_SYS_STR(str) dgettext("sys_string", str)

#define DOMAIN_NAME "music-player"
#define LOCALE_DIR LOCALEDIR
#define GET_STR(str) dgettext(DOMAIN_NAME, str)

#ifdef __ARM__
#define ENABLE_RICHINFO
#else
#define __I386__		//define for support i386
#endif

#ifndef PACKAGE
#define PACKAGE "music-player"
#endif

#define DATA_DIR	DATA_PREFIX"/data"

#ifndef MP_INI_DIR
#define MP_INI_DIR DATA_DIR
#endif

#define PKGNAME_FOR_SHORTCUT	PKG_NAME

#define EDJ_PATH EDJDIR
#define EDJ_NAME EDJ_PATH"/mp-library.edj"
#define MINICON_EDJ_NAME EDJ_PATH"/mp-minicontroller.edj"
#define IMAGE_EDJ_NAME EDJ_PATH"/mp-images.edj"
#define GRP_MAIN "main"

#define THEME_NAME	EDJ_PATH"/mp-custom-winset-style.edj"

#define TITLE_H 90
#define START_Y_POSITION	94

#define MP_PHONE_ROOT_PATH        "/opt/usr/media"
#define MP_MMC_ROOT_PATH		"/opt/storage/sdcard"

#define MP_THUMB_DOWNLOAD_TEMP_DIR	DATA_DIR"/.allshare/thumb"

#ifdef PATH_MAX
#	define MAX_NAM_LEN   PATH_MAX
#else
#	define MAX_NAM_LEN   4096
#endif


#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif

#define SIGNAL_MAIN_MUTE_SHOW					"signal.main.mute.show"
#define SIGNAL_MAIN_MUTE_HIDE					"signal.main.mute.hide"

#define CHECK(x) if(!x)	ERROR_TRACE("RETURN NULL!!\n", x);
#define SAFE_FREE(x)       if(x) {free(x); x = NULL;}

#define MAX_STR_LEN				MAX_NAM_LEN
#define MAX_URL_LEN				MAX_NAM_LEN

#define PLAY_VIEW_EDJ_NAME 			EDJ_PATH"/music.edj"

#define SIGNAL_PAUSE						"pause_clicked"
#define SIGNAL_PLAY						"play_clicked"
#define SIGNAL_VOLUME					"play_volume_clicked"
#define SIGNAL_ALBUMART_CLICKED			"album_art_clicked"
#define SIGNAL_ALBUMART_UP				"album_art_up"
#define SIGNAL_ALBUMART_DOWN			"album_art_down"
#define SIGNAL_ALBUMART_MOVE			"album_art_move"
#define SIGNAL_INFO						"play_info_clicked"
#define SIGNAL_SRS						"srs_clicked"
#define SIGNAL_SRS_DIM					"srs_dim_clicked"
#define SIGNAL_SHUFFLE					"shuffle_clicked"
#define SIGNAL_SHUFNON					"shuffle_non_clicked"
#define SIGNAL_REPALL					"rep_all_clicked"
#define SIGNAL_REPNON					"rep_non_clicked"
#define SIGNAL_REP1						"rep_1_clicked"
#define SIGNAL_AUTOREP					"auto_rep_clicked"
#define SIGNAL_MAINALBUM_CLICKED		"main_albumart_clicked"
#define SIGNAL_FAVORITE_CLICKED			"favorite_clicked"
#define SIGNAL_NOW_PLAYING_CLICKED	"now_playing_clicked"

//2 EVAS_OBJECT_TYPE define
#define MP_FAST_SCROLLER_TYPE 					"mp_data_fast_scroller_type"

//2 FEATURE define
#define MP_FEATURE_SUPPORT_ID3_TAG

#define MP_FEATURE_ADD_TO_HOME
#define MP_FEATURE_S_BEAM
#define MP_FEATURE_MOTION
#define MP_FEATURE_NFC_SHARE
#define MP_FEATURE_SVOICE
#ifndef MP_SOUND_PLAYER	/* music player only*/
#define MP_FEATURE_ASF_ALLSHARE
#define MP_FEATURE_LIVE_BOX
#define MP_FEATURE_SPLIT_VIEW
#define MP_FEATURE_CONTEXT_ENGINE
#endif
#ifdef MP_SOUND_PLAYER
#undef MP_3D_FEATURE
#define MP_FEATURE_EXIT_ON_BACK
#endif
#define MP_FEATURE_WIFI_SHARE
#define MP_FEATURE_INNER_SETTINGS
#define MP_FEATURE_SOUND_ALIVE
//#define MP_FEATURE_DRM_CONSUMPTION
#define MP_FEATURE_AVRCP_13
#define MP_FEATURE_DESKTOP_MODE

#define MP_FUNC_ALLSHARE_PLAYLIST			"music-player:allshare_"

#define MP_POPUP_YES	1
#define MP_POPUP_NO	0
#define MP_POPUP_TIMEOUT	(2.0)

#define MP_STR_UNKNOWN	"Unknown"
#define MP_YEAR_S		"%03u0s"

typedef int SLP_Bool;
typedef void (*MpHttpOpenRspCb) (gpointer user_data);
typedef void (*MpGetShazamSigCb) (char *signature, int size, void *data);

#define TIME_FORMAT_LEN	15
#define PLAY_TIME_ARGS(t) \
        (((int)(t)) / 60) % 60, \
        ((int)(t)) % 60
#define PLAY_TIME_FORMAT "02u:%02u"

#define MUSIC_TIME_ARGS(t) \
        ((int)(t)) / (3600), \
        (((int)(t)) / 60) % 60, \
        ((int)(t)) % 60
#define MUSIC_TIME_FORMAT "02u:%02u:%02u"

#undef FREE
#define FREE(ptr) free(ptr); ptr = NULL;

#undef IF_FREE
#define IF_FREE(ptr) if (ptr) {free(ptr); ptr = NULL;}

#undef IF_G_FREE
#define IF_G_FREE(p) ({g_free(p);p=NULL;})

#define mp_evas_object_del(object) do { \
	if(object) { \
		evas_object_del(object); \
		object = NULL; \
	} \
} while (0)

#define mp_elm_genlist_del(list) do { \
	if(list) { \
		elm_genlist_clear(list);\
		evas_object_del(list); \
		list = NULL; \
	} \
} while (0)

#define mp_ecore_timer_del(timer) do { \
	if(timer) { \
		ecore_timer_del(timer);\
		timer = NULL; \
	} \
} while (0)

#define mp_ecore_idler_del(idler) do { \
	if(idler) { \
		ecore_idler_del(idler);\
		idler = NULL; \
	} \
} while (0)


#define SAFE_STRCPY(dest, src) \
	do{if(!dest||!src)break;\
		strncpy (dest , src, sizeof(dest)-1);\
		dest[sizeof(dest)-1] = 0;	}while(0)

#define mp_evas_object_response_set(obj, response) do { \
	if (obj) { \
		evas_object_data_set((obj), "response", (void *)(response)); \
	} \
} while (0)

#define mp_evas_object_response_get(obj) (int)evas_object_data_get((obj), "response")


typedef enum
{
	MP_SCREEN_MODE_PORTRAIT = 0,
	MP_SCREEN_MODE_LANDSCAPE,
} mp_screen_mode;


typedef enum
{
	MP_SND_PATH_BT,
	MP_SND_PATH_EARPHONE,
	MP_SND_PATH_SPEAKER,
	MP_SND_PATH_MAX,
} mp_snd_path;

typedef enum
{
	MP_VIEW_MODE_DEFAULT,
	MP_VIEW_MODE_EDIT,
	MP_VIEW_MODE_SEARCH,
} mp_view_mode_t;

typedef enum
{
	MP_VIEW_TYPE_SONGS,
	MP_VIEW_TYPE_PLAYLIST,
	MP_VIEW_TYPE_ALBUM,
	MP_VIEW_TYPE_GENRE,
	MP_VIEW_TYPE_ARTIST,
	MP_VIEW_TYPE_YEAR,
	MP_VIEW_TYPE_COMPOSER,
	MP_VIEW_TYPE_FOLDER,
	MP_VIEW_TYPE_ALLSHARE,
	MP_VIEW_TYPE_PLAYVIEW,


	MP_VIEW_TYPE_MAX,
} mp_view_type_t;

#define MP_GENLIST_CHECK_FOREACH_SAFE(first, current, next, data) \
	for (current = first,                                      \
		next = elm_genlist_item_next_get(current),                    \
		data = elm_object_item_data_get(current);                  \
		current;                                             \
		current = next,                                    \
		next = elm_genlist_item_next_get(current),                    \
		data = elm_object_item_data_get(current))

#define 	MP_PLAYLIST_MAX_ITEM_COUNT 1000
#define 	MP_NOW_PLAYING_ICON_SIZE 48 * elm_config_scale_get()
#define 	MP_LIST_ICON_SIZE 70 * elm_config_scale_get()
#define 	MP_ALBUM_LIST_ICON_SIZE 48 * elm_config_scale_get()
#define 	MP_PLAY_VIEW_ARTWORK_SIZE 480 * elm_config_scale_get()

#define MP_NOWPLAYING_INI_FILE_NAME	MP_INI_DIR"/now_playing.ini"
#define MP_SHORTCUT_INI_FILE_NAME_0		MP_INI_DIR"/shortcut_0.ini"
#define MP_SHORTCUT_INI_FILE_NAME_1		MP_INI_DIR"/shortcut_1.ini"
#define MP_SHORTCUT_INI_FILE_NAME_2		MP_INI_DIR"/shortcut_2.ini"
#define MP_SHORTCUT_INI_FILE_NAME_3		MP_INI_DIR"/shortcut_3.ini"

#define SINGLE_BYTE_MAX 0x7F

typedef enum
{
	MP_UG_MESSAGE_BACK,
	MP_UG_MESSAGE_DEL,
	MP_UG_MESSAGE_LOAD,
}mp_ug_message_t;

#define MP_POPUP_GENLIST_ITEM_H 112
#define MP_POPUP_GENLIST_ITEM_H_MAX 408
#define MP_POPUP_GENLIST_ITEM_W 610

#define ELM_NAVIFRAME_ITEM_CONTENT				"default"
#define ELM_NAVIFRAME_ITEM_ICON					"icon"
#define ELM_NAVIFRAME_ITEM_OPTIONHEADER			"optionheader"
#define ELM_NAVIFRAME_ITEM_TITLE_LABEL			"title"
#define ELM_NAVIFRAME_ITEM_PREV_BTN				"prev_btn"
#define ELM_NAVIFRAME_ITEM_TITLE_LEFT_BTN		"title_left_btn"
#define ELM_NAVIFRAME_ITEM_TITLE_RIGHT_BTN		"title_right_btn"
#define ELM_NAVIFRAME_ITEM_TITLE_MORE_BTN		"title_more_btn"
#define ELM_NAVIFRAME_ITEM_CONTROLBAR			"controlbar"
#define ELM_NAVIFRAME_ITEM_SIGNAL_OPTIONHEADER_CLOSE		"elm,state,optionheader,close", ""
#define ELM_NAVIFRAME_ITEM_SIGNAL_OPTIONHEADER_OPEN			"elm,state,optionheader,open", ""

#define MP_PLAYLIST_NAME_SIZE			101
#define MP_METADATA_LEN_MAX	193

#endif /* __MP_DEFINE_H_ */
