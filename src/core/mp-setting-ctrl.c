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


#include <string.h>
#include <stdlib.h>
#include <vconf.h>
#include <sound_manager.h>
#include "mp-media-info.h"

#include "music.h"

#include "mp-setting-ctrl.h"
#include "mp-player-debug.h"
#include "mp-file-tag-info.h"
#include "mp-player-mgr.h"
#include "mp-widget.h"
#include "mp-util.h"
#include "mp-vconf-private-keys.h"

#ifdef MP_FEATURE_AVRCP_13
#include "mp-avrcp.h"
#endif

enum _mp_menu_item
{
	MP_MENU_ALBUMS,
	MP_MENU_ARTISTS,
	MP_MENU_GENRES,
	MP_MEMU_COMPOSER,
	MP_MENU_YEARS,
	MP_MENU_FOLDERS,
	MP_MENU_NUMS,
};


typedef struct _mp_setting_t
{
	MpSettingPlaylist_Cb playlist_cb;
	void *playlist_udata;

} mp_setting_t;

static mp_setting_t *g_setting = NULL;


static Eina_Bool _mp_setting_init_idler_cb(void *data);

static void
_mp_setting_playlist_changed_cb(keynode_t * node, void *user_data)
{
	mp_setting_t *sd = NULL;
	int state = 0;

	mp_retm_if(node == NULL, "keymode is NULL");
	mp_retm_if(user_data == NULL, "user_date is NULL");
	sd = (mp_setting_t *) user_data;

	state = vconf_keynode_get_int(node);

	if (sd->playlist_cb)
	{
		sd->playlist_cb(state, sd->playlist_udata);
	}

	return;
}

static int
mp_setting_key_cb_init(void)
{
	int ret = 0;

	mp_retvm_if(g_setting == NULL, -1, "setting data is not initialized, init first!!!!!");


	if (vconf_notify_key_changed(MP_VCONFKEY_PLAYLIST_VAL_INT, _mp_setting_playlist_changed_cb, g_setting) < 0)
	{
		ERROR_TRACE("Fail to register MP_VCONFKEY_PLAYLIST_VAL_INT key callback");
		ret = -1;
	}

	return ret;
}

static void
mp_setting_key_cb_deinit(void)
{
	vconf_ignore_key_changed(MP_VCONFKEY_PLAYLIST_VAL_INT, _mp_setting_playlist_changed_cb);
	return;
}

static Eina_Bool
_mp_setting_init_idler_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	mp_setting_key_cb_init();
	ad->setting_idler = NULL;

	return EINA_FALSE;
}

int
mp_setting_init(struct appdata *ad)
{
	int ret = 0;
DEBUG_TRACE("%s:+++\n", __func__);	//Minjin
	g_setting = malloc(sizeof(mp_setting_t));
	if (!g_setting)
	{
		ERROR_TRACE("Fail to alloc memory");
		return -1;
	}
	memset(g_setting, 0x00, sizeof(mp_setting_t));

	if(!ad->setting_idler)
		ad->setting_idler = ecore_idler_add(_mp_setting_init_idler_cb, ad);

	int shuffle;
	mp_setting_get_shuffle_state(&shuffle);
	mp_playlist_mgr_set_shuffle(ad->playlist_mgr, shuffle);

	return ret;
}


int
mp_setting_deinit(struct appdata *ad)
{
	mp_ecore_idler_del(ad->setting_idler);
	mp_setting_key_cb_deinit();

	if (g_setting)
	{
		free(g_setting);
		g_setting = NULL;
	}

	return 0;
}


int
mp_setting_set_shuffle_state(int b_val)
{
	if (vconf_set_bool(MP_VCONFKEY_MUSIC_SHUFFLE, b_val))
	{
		WARN_TRACE("Fail to set MP_VCONFKEY_MUSIC_SHUFFLE");
		return -1;
	}

	return 0;
}

int
mp_setting_get_shuffle_state(int *b_val)
{
	if (vconf_get_bool(MP_VCONFKEY_MUSIC_SHUFFLE, b_val))
	{
		WARN_TRACE("Fail to get MP_VCONFKEY_MUSIC_SHUFFLE");

		if (vconf_set_bool(MP_VCONFKEY_MUSIC_SHUFFLE, FALSE))
		{
			ERROR_TRACE("Fail to set MP_VCONFKEY_MUSIC_SHUFFLE");
			return -1;
		}
		*b_val = FALSE;
	}
	return 0;
}

int
mp_setting_set_repeat_state(int val)
{
	if (vconf_set_int(MP_VCONFKEY_MUSIC_REPEAT, val))
	{
		ERROR_TRACE("Fail to set MP_VCONFKEY_MUSIC_REPEAT");
		return -1;
	}

	return 0;
}

int
mp_setting_get_repeat_state(int *val)
{
	if (vconf_get_int(MP_VCONFKEY_MUSIC_REPEAT, val))
	{
		WARN_TRACE("Fail to get MP_VCONFKEY_MUSIC_REPEAT");
		if (vconf_set_int(MP_VCONFKEY_MUSIC_REPEAT, MP_SETTING_REP_NON))
		{
			ERROR_TRACE("Fail to set MP_VCONFKEY_MUSIC_REPEAT");
			return -1;
		}
		*val = MP_SETTING_REP_NON;
	}

	return 0;
}


int mp_setting_playlist_get_state(int *state)
{
	int res = vconf_get_int(MP_VCONFKEY_PLAYLIST_VAL_INT, state);
	return res;
}

int mp_setting_playlist_set_callback(MpSettingPlaylist_Cb func, void *data)
{
	mp_retvm_if(g_setting == NULL, -1, "setting data is not initialized, init first!!!!!");

	g_setting->playlist_cb = func;
	g_setting->playlist_udata= data;

	return 0;
}

#ifndef MP_SOUND_PLAYER
void
mp_setting_save_now_playing(void *data)
{
	startfunc;
	struct appdata *ad = data;
	FILE *fp = NULL;
	mp_plst_item *item = NULL;

	MP_CHECK(ad);
	MP_CHECK(ad->playlist_mgr);

	item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK(item);
	MP_CHECK(ad->current_track_info);

	fp = fopen(MP_NOWPLAYING_INI_FILE_NAME, "w");	// make new file.

	if (fp == NULL)
	{
		ERROR_TRACE("Failed to open ini files. : %s", MP_NOWPLAYING_INI_FILE_NAME);
		return;
	}

	fprintf(fp, "%s\n", item->uid);
	fprintf(fp, "%s\n", item->uri);
	fprintf(fp, "%s\n", ad->current_track_info->title);
	fprintf(fp, "%s\n", ad->current_track_info->artist);
	fprintf(fp, "%s\n", ad->current_track_info->album);
	fprintf(fp, "%s\n", ad->current_track_info->thumbnail_path);
	fprintf(fp, "\n");

	fclose(fp);

	endfunc;
}

#define MP_SHORTCUT_COUNT 4

void
mp_setting_save_shortcut(char *shortcut_title, char *artist, char *shortcut_description, char *shortcut_image_path)
{
	startfunc;
	FILE *fp = NULL;
	int ret = 0;

	if (ecore_file_exists(MP_SHORTCUT_INI_FILE_NAME_2))
	{
		ret = rename(MP_SHORTCUT_INI_FILE_NAME_2, MP_SHORTCUT_INI_FILE_NAME_3);
		if (ret != 0) {
			ERROR_TRACE("Failed to rename file:error=%d",ret);
			return;
		}
	}
	if (ecore_file_exists(MP_SHORTCUT_INI_FILE_NAME_1))
	{
		rename(MP_SHORTCUT_INI_FILE_NAME_1, MP_SHORTCUT_INI_FILE_NAME_2);
		if (ret != 0) {
			ERROR_TRACE("Failed to rename file:error=%d",ret);
			return;
		}
	}
	if (ecore_file_exists(MP_SHORTCUT_INI_FILE_NAME_0))
	{
		rename(MP_SHORTCUT_INI_FILE_NAME_0, MP_SHORTCUT_INI_FILE_NAME_1);
		if (ret != 0) {
			ERROR_TRACE("Failed to rename file:error=%d",ret);
			return;
		}
	}

	fp = fopen(MP_SHORTCUT_INI_FILE_NAME_0, "w");	// make new file.

	if (fp == NULL)
	{
		ERROR_TRACE("Failed to open ini files. : %s", MP_SHORTCUT_INI_FILE_NAME_0);
		return;
	}

	fprintf(fp, "[ShortCut]\n");
	fprintf(fp, "title=%s\n", shortcut_title);
	if (artist)
		fprintf(fp, "artist=%s\n", artist);
	fprintf(fp, "desc=%s\n", shortcut_description);
	fprintf(fp, "artwork=%s\n", shortcut_image_path);
	fprintf(fp, "\n");

	fclose(fp);

	endfunc;
}

#endif

void
mp_setting_update_active_device()
{
}
