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

#include <syspopup_caller.h>
#include <sound_manager.h>

#include "music.h"
#include "mp-ta.h"
#include "mp-player-control.h"
#include "mp-player-mgr.h"
#include "mp-player-drm.h"
#include "mp-play-view.h"
#include "mp-item.h"
#include "mp-playlist-mgr.h"
#include "mp-widget.h"
#include "mp-app.h"
#include "mp-streaming-mgr.h"
#include "mp-util.h"
#include "mp-player-debug.h"
#include "mp-minicontroller.h"
#include "mp-play.h"
#include "mp-setting-ctrl.h"
#ifdef MP_FEATURE_AVRCP_13
#include "mp-avrcp.h"
#endif

#define PAUSE_OFF_TIMEOUT			(2 * 60)	// sec



#ifndef MP_SOUND_PLAYER
#include "mp-common.h"
#include "mp-library.h"
#endif






//this function should be called in player ready state.
bool
mp_play_current_file(void *data)
{
	startfunc;
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);
	MP_CHECK_FALSE(ad->player_state == PLAY_STATE_READY);

DEBUG_TRACE("%s:1\n", __func__);	//Minjin	


	mp_play_view_update_progressbar(ad);

	mp_plst_item * current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK_FALSE(current_item);

	if(!ad->paused_by_user)
	{
		if (!mp_player_mgr_play(ad))
		{
			mp_play_destory(ad);
			return FALSE;
		}
	}
	else
	{
		DEBUG_TRACE("stay in pause state..");
		return false;
	}

	if(ad->is_focus_out)
	{
		if(!ad->win_minicon)
			mp_minicontroller_create(ad);
		else
			mp_minicontroller_show(ad);
	}
	if (ad->b_minicontroller_show)
		mp_minicontroller_update(ad);

	if (current_item->track_type ==  MP_TRACK_URI && current_item->uid)
	{
		if (!mp_item_update_db(current_item->uid))
		{
			WARN_TRACE("Error when update db");
		}
	}

#ifndef MP_SOUND_PLAYER
	mp_library_now_playing_set(ad);
	mp_setting_save_now_playing(ad);
#endif

	IF_FREE(ad->latest_playing_key_id);
	ad->latest_playing_key_id = g_strdup(current_item->uid);

	vconf_set_int(MP_VCONFKEY_PLAYING_PID, getpid());


	return TRUE;
}

bool
mp_play_new_file(void *data, bool check_drm)
{
	startfunc;
	struct appdata *ad = data;
	mp_retvm_if(ad == NULL, FALSE, "appdata is NULL");

DEBUG_TRACE("%s:+++\n", __func__);	//Minjin	

	mp_plst_item * current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK_FALSE(current_item);

	if (mp_util_check_uri_available(current_item->uri))
		return mp_streaming_mgr_play_new_streaming(ad);
	else
		return mp_play_new_file_real(ad, check_drm);
}

bool
mp_play_new_file_real(void *data, bool check_drm)
{
	startfunc;
	struct appdata *ad = data;
	mp_retvm_if(ad == NULL, FALSE, "appdata is NULL");

	if (!mp_player_control_ready_new_file(ad, check_drm))
	{
		return FALSE;
	}
	return TRUE;
}

bool
mp_play_item_play(void *data, char *fid)
{
	startfunc;

	MP_CHECK_FALSE(data);
	MP_CHECK_FALSE(fid);

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	if (mp_playlist_mgr_get_current(ad->playlist_mgr))
	{
		if (g_strcmp0(ad->latest_playing_key_id, fid))	//playing a new file
		{
			mp_debug("current key = %s it->key id = %s\n", ad->latest_playing_key_id, fid);
			mp_play_destory(ad);

			if (!mp_play_new_file(ad, TRUE))
			{
				ERROR_TRACE("Fail to play new file");
				return FALSE;
			}
		}
		else
		{
			mp_debug("playing the same file");
		}
	}
	else
	{
		if (!mp_play_new_file(ad, TRUE))
		{
			ERROR_TRACE("Fail to play new file");
			return FALSE;
		}
	}


	endfunc;

	return TRUE;
}


void
mp_play_prev_file(void *data)
{
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	mp_plst_item *item = mp_playlist_mgr_get_prev(ad->playlist_mgr);
	if (item)
	{
		if (ad->playing_view)
			ad->playing_view->flick_direction = 2;
		ad->freeze_indicator_icon = TRUE;

		mp_playlist_mgr_set_current(ad->playlist_mgr, item);

		if (ad->playing_view)
			mp_play_view_play_item(data, item, true, false);
		else
			mp_play_item_play(ad, item->uid);
	}
	else
	{
		mp_error("mp_play_list_get_prev_item return false");
#ifdef MP_SOUND_PLAYER
		if (ad->is_focus_out)
			mp_app_exit(ad);
		else
#endif
		{
			mp_widget_text_popup(data, GET_SYS_STR("IDS_COM_POP_FILE_NOT_FOUND"));
			mp_play_stop_and_updateview(data, FALSE);
		}
	}
}

void
mp_play_next_file(void *data, bool forced)
{
	struct appdata *ad = data;
	mp_plst_item *item = NULL;

	mp_retm_if(ad == NULL, "appdata is NULL");
	MP_CHECK(ad->playlist_mgr);

	int repeat = mp_playlist_mgr_get_repeat(ad->playlist_mgr);

	if(!forced)
	{
		if(repeat == MP_PLST_REPEAT_ONE
			||(repeat == MP_PLST_REPEAT_ALL && mp_playlist_mgr_count(ad->playlist_mgr) == 1))
		{
			DEBUG_TRACE("play same track");
			ad->freeze_indicator_icon = true;
			mp_player_mgr_stop(ad);
			mp_player_mgr_play(ad);
			return;
		}
	}
	IF_FREE(ad->latest_playing_key_id);

	item = mp_playlist_mgr_get_next(ad->playlist_mgr, forced);
	if (item)
	{
		mp_playlist_mgr_set_current(ad->playlist_mgr, item);

		if (ad->playing_view)
			ad->playing_view->flick_direction = 1;
		ad->freeze_indicator_icon = TRUE;
		if (ad->playing_view)
			mp_play_view_play_item(data, item, true, true);
		else
			mp_play_item_play(ad, item->uid);
	}
	else
	{
		WARN_TRACE("mp_play_list_get_next_item return false");
		mp_player_mgr_stop(ad);
		mp_player_mgr_destroy(ad);
		ad->music_pos = 0;
		mp_play_view_update_progressbar(ad);

#ifdef MP_SOUND_PLAYER
		if (ad->is_focus_out)
			mp_app_exit(ad);
		else
#endif
		{
			DEBUG_TRACE("End of playlist");
		}
	}

}

static void
_mp_play_progressbar_done_cb(void *data)
{
	startfunc;
	struct appdata *ad = data;

	mp_play_view_progress_timer_thaw(ad);
}


void
mp_play_start(void *data)
{
	startfunc;
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

//TODO: Minjin
double pos = 0;
vconf_get_dbl("memory/private/org.tizen.music-player/pos", &pos);
ad->music_pos = pos;
DEBUG_TRACE("%s:+++:ad->music_pos=%f\n", __func__, ad->music_pos);	//Minjin
mp_player_mgr_set_position(ad->music_pos * 1000, _mp_play_progressbar_done_cb, ad);
mp_play_view_update_progressbar(ad);


	if (ad->playing_view)
	{
		ad->playing_view->flick_direction = 0;	//reset flick_direction
	}

	mp_plst_item * item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK(item);

	ad->player_state = PLAY_STATE_PLAYING;

	vconf_set_int(VCONFKEY_MUSIC_STATE, VCONFKEY_MUSIC_PLAY);

#ifdef MP_FEATURE_DRM_CONSUMPTION
	mp_drm_start_consumption(item->uri);
#endif
	mp_util_sleep_lock_set(TRUE);

	mp_play_control_play_pause_icon_set(ad, FALSE);;
	if(ad->is_focus_out)
	{
		if(!ad->win_minicon)
			mp_minicontroller_create(ad);
		else
			mp_minicontroller_show(ad);
	}
	if (ad->b_minicontroller_show)
		mp_minicontroller_update(ad);


#ifdef MP_SOUND_PLAYER
	mp_play_view_progress_timer_thaw(ad);
#else
	mp_view_manager_thaw_progress_timer(ad);
#endif

	mp_app_grab_mm_keys(ad);


#ifdef MP_FEATURE_AVRCP_13
	mp_avrcp_noti_player_state(MP_AVRCP_STATE_PLAYING);
#endif


	endfunc;
}

void
mp_play_pause(void *data)
{
	startfunc;
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

//TODO: Minjin
DEBUG_TRACE("%s:+++:ad->music_pos=%f\n", __func__, ad->music_pos);	//Minjin
vconf_set_dbl("memory/private/org.tizen.music-player/pos", ad->music_pos);
	
	ad->player_state = PLAY_STATE_PAUSED;

	if (!ad->paused_by_other_player)
		vconf_set_int(VCONFKEY_MUSIC_STATE, VCONFKEY_MUSIC_PAUSE);

#ifdef MP_FEATURE_DRM_CONSUMPTION
	mp_drm_pause_consumption();
#endif
	mp_util_sleep_lock_set(FALSE);

	mp_play_control_play_pause_icon_set(ad, TRUE);
	if (ad->b_minicontroller_show)
		mp_minicontroller_update_control(ad);


	mp_play_view_update_progressbar(ad);
#ifdef MP_SOUND_PLAYER
	mp_play_view_progress_timer_freeze(ad);
#else
	mp_view_manager_freeze_progress_timer(ad);
#endif


	ad->player_state = PLAY_STATE_PAUSED;
	ad->paused_by_other_player = FALSE;

#ifdef MP_FEATURE_AVRCP_13
	mp_avrcp_noti_player_state(MP_AVRCP_STATE_PAUSED);
#endif

	endfunc;
}

void
mp_play_stop(void *data)
{
	startfunc;
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");


	ad->player_state = PLAY_STATE_READY;
	if (!ad->freeze_indicator_icon)
	{
		if (!mp_util_is_other_player_playing())
			vconf_set_int(VCONFKEY_MUSIC_STATE, VCONFKEY_MUSIC_STOP);


		mp_minicontroller_destroy(ad);
	}
	else
	{
		if (ad->b_minicontroller_show)
			mp_minicontroller_update_control(ad);
	}



#ifdef MP_FEATURE_DRM_CONSUMPTION
	mp_drm_stop_consumption();
	mp_drm_set_consumption(FALSE);
#endif
	mp_util_sleep_lock_set(FALSE);

#ifdef MP_SOUND_PLAYER
	ad->music_pos = 0;
	mp_play_view_update_progressbar(ad);
	mp_play_view_progress_timer_freeze(ad);
#else
	if (mp_view_manager_is_play_view(ad))
	{
		ad->music_pos = 0;
		mp_play_view_update_progressbar(ad);
	}
	else
		mp_view_manager_freeze_progress_timer(ad);
#endif

	mp_play_control_play_pause_icon_set(ad, TRUE);

	mp_lyric_view_destroy(ad);
	mp_lyric_mgr_destory(ad);


#ifdef MP_FEATURE_AVRCP_13
	mp_avrcp_noti_player_state(MP_AVRCP_STATE_STOPPED);
#endif


	endfunc;
}

void
mp_play_resume(void *data)
{
	startfunc;
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	//TODO: Minjin
double pos = 0;
vconf_get_dbl("memory/private/org.tizen.music-player/pos", &pos);
ad->music_pos = pos;
DEBUG_TRACE("%s:+++:ad->music_pos=%f\n", __func__, ad->music_pos);	//Minjin
mp_player_mgr_set_position(ad->music_pos * 1000, _mp_play_progressbar_done_cb, ad);
mp_play_view_update_progressbar(ad);

	ad->player_state = PLAY_STATE_PLAYING;

	vconf_set_int(VCONFKEY_MUSIC_STATE, VCONFKEY_MUSIC_PLAY);

#ifdef MP_FEATURE_DRM_CONSUMPTION
	mp_drm_resume_consumption();
#endif
	mp_util_sleep_lock_set(TRUE);

	mp_play_control_play_pause_icon_set(ad, FALSE);
	if(ad->is_focus_out)
	{
		if(!ad->win_minicon)
			mp_minicontroller_create(ad);
		else
			mp_minicontroller_show(ad);
	}
	if (ad->b_minicontroller_show)
		mp_minicontroller_update_control(ad);


#ifdef MP_SOUND_PLAYER
	mp_play_view_progress_timer_thaw(ad);
#else
	mp_view_manager_thaw_progress_timer(ad);
#endif
	mp_app_grab_mm_keys(ad);


#ifdef MP_FEATURE_AVRCP_13
	mp_avrcp_noti_player_state(MP_AVRCP_STATE_PLAYING);
#endif

	endfunc;
}

bool
mp_play_destory(void *data)
{
	struct appdata *ad = data;
	mp_retvm_if(ad == NULL, FALSE, "appdata is NULL");

	mp_player_mgr_stop(ad);
	mp_player_mgr_unrealize(ad);
	mp_player_mgr_destroy(ad);

	return TRUE;
}

void
mp_play_stop_and_updateview(void *data, bool mmc_removed)
{
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	if (ad->player_state != PLAY_STATE_NONE)
	{
		DEBUG_TRACE("mp_play_stop_and_updateview\n");
		mp_play_destory(ad);
	}

	if (ad->playing_view && ad->playing_view->layout)
	{
		mp_play_view_pop(ad);
	}


#ifndef MP_SOUND_PLAYER
	mp_library_now_playing_hide(ad);
	mp_library_update_view(ad);
#endif
	return;
}
