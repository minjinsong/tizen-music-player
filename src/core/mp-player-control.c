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
#include <player.h>

#include "music.h"
#include "mp-ta.h"
#include "mp-player-control.h"
#include "mp-player-mgr.h"
#include "mp-player-drm.h"
#include "mp-play-view.h"
#include "mp-item.h"
#include "mp-playlist-mgr.h"
#include "mp-play.h"
#include "mp-util.h"
#include "mp-setting-ctrl.h"
#include "mp-player-mgr.h"
#include "mp-app.h"
#include "mp-player-debug.h"
#include "mp-minicontroller.h"
#include "mp-widget.h"
#include "mp-streaming-mgr.h"
#include "mp-ug-launch.h"
#ifdef MP_SOUND_PLAYER
#include "mp-ug-launch.h"
#include "sp-view-manager.h"
#else
#include "mp-common.h"
#include "mp-view-manager.h"
#endif

#ifdef MP_FEATURE_AVRCP_13
#include "mp-avrcp.h"
#endif


#define CTR_EDJ_SIG_SRC "ctrl_edj"
#define CTR_PROG_SIG_SRC "ctrl_prog"

#define LONG_PRESS_INTERVAL             1.0	//sec
#define FF_REW_INTERVAL             0.5		//sec
#define LONG_PRESS_TIME_INCREASE	1.0	//sec

static Eina_Bool _mp_play_mute_popup_cb(void *data);

static void
_mp_play_control_error_timeout(void *data)
{
	startfunc;
	struct appdata *ad = data;
	MP_CHECK(ad);
	mp_plst_item *next = mp_playlist_mgr_get_next(ad->playlist_mgr, false);
	mp_plst_item *current = mp_playlist_mgr_get_current(ad->playlist_mgr);

	if(next == current)
	{
		WARN_TRACE("There is no playable track.. ");
		return;
	}

	mp_playlist_mgr_item_remove_item(ad->playlist_mgr, current);
	if(next)
	{
		mp_playlist_mgr_set_current(ad->playlist_mgr, next);
		mp_play_new_file(ad, true);
	}
	else
		mp_playlist_mgr_set_current(ad->playlist_mgr, mp_playlist_mgr_get_nth(ad->playlist_mgr, 0));

	if(ad->playing_view)
		mp_play_view_refresh(ad);

}

static void
_mp_play_error_handler(struct appdata *ad, const char *msg)
{
	startfunc;
	mp_play_destory(ad);

	if(ad->app_is_foreground && ad->playing_view)
		mp_widget_text_popup_with_cb(ad, msg,
			_mp_play_control_error_timeout);
	else
		_mp_play_control_error_timeout(ad);
}

static void
_mp_play_control_long_press_seek_done_cb(void *data)
{
	struct appdata *ad = data;
	mp_play_view_progress_timer_thaw(ad);
}

static Eina_Bool
_mp_play_control_long_pressed_cb(void *data)
{
	startfunc;

	struct appdata *ad = data;
	mp_retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "appdata is NULL");

	double pos = 0, duration = 0, new_pos = 0;

	if (ad->is_ff)
	{
		ad->ff_rew += LONG_PRESS_TIME_INCREASE;
#ifdef MP_FEATURE_AVRCP_13
		if(!ad->is_Longpress)
			mp_avrcp_noti_player_state(MP_AVRCP_STATE_FF);
#endif
	}
	else
	{
		ad->ff_rew -= LONG_PRESS_TIME_INCREASE;
#ifdef MP_FEATURE_AVRCP_13
		if(!ad->is_Longpress)
			mp_avrcp_noti_player_state(MP_AVRCP_STATE_FF);
#endif
	}

	ad->is_Longpress = true;

	duration = mp_player_mgr_get_duration();
	pos = mp_player_mgr_get_position();

	ad->music_length = duration / 1000;
	ad->music_pos = pos / 1000;

	new_pos = ad->music_pos + ad->ff_rew;

DEBUG_TRACE("new pos=%f", new_pos);

	int req_seek_pos = 0;
	if (new_pos < 0.)
	{
		ad->music_pos = 0;
		req_seek_pos = 0;
	}
	else if (new_pos > ad->music_length)
	{
		ad->music_pos = ad->music_length;
		req_seek_pos = duration;
	}
	else
	{
		ad->music_pos = new_pos;
		req_seek_pos = new_pos * 1000;
	}

DEBUG_TRACE("req_seek_pos=%f", req_seek_pos);

	if (mp_player_mgr_set_position(req_seek_pos, _mp_play_control_long_press_seek_done_cb, ad)) {
		mp_play_view_progress_timer_freeze(ad);
		mp_play_view_update_progressbar(ad);
	}

	ecore_timer_interval_set(ad->longpress_timer, FF_REW_INTERVAL);

	endfunc;

	return ECORE_CALLBACK_RENEW;

}

static void
_mp_play_control_add_longpressed_timer(void *data)
{
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");
	MP_CHECK(!ad->longpress_timer);

	ad->longpress_timer =
		ecore_timer_add(LONG_PRESS_INTERVAL, _mp_play_control_long_pressed_cb, ad);
}

void
_mp_play_control_del_longpressed_timer(void *data)
{
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	ad->ff_rew = 0;
	mp_ecore_timer_del(ad->longpress_timer);
}

static void
_mp_play_control_completed_cb(void *userdata)
{
	startfunc;
	struct appdata *ad = userdata;
	MP_CHECK(ad);

	mp_play_control_end_of_stream(ad);
}

static void
_mp_play_control_interrupted_cb(player_interrupted_code_e code, void *userdata)
{
	startfunc;
	struct appdata *ad = userdata;
	MP_CHECK(ad);

	switch(code)
	{
	case PLAYER_INTERRUPTED_BY_MEDIA:
		DEBUG_TRACE("receive MM_MSG_CODE_INTERRUPTED_BY_OTHER_APP");
		break;
	case PLAYER_INTERRUPTED_BY_CALL:
		DEBUG_TRACE("receive PLAYER_INTERRUPTED_BY_CALL");
		break;
	case PLAYER_INTERRUPTED_BY_EARJACK_UNPLUG:
		DEBUG_TRACE("receive MM_MSG_CODE_INTERRUPTED_BY_EARJACK_UNPLUG");
		break;
	case PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT:
		DEBUG_TRACE("receive MM_MSG_CODE_INTERRUPTED_BY_RESOURCE_CONFLICT");
		break;
	case PLAYER_INTERRUPTED_BY_ALARM:
		DEBUG_TRACE("receive MM_MSG_CODE_INTERRUPTED_BY_ALARM_START");
		break;
	case PLAYER_INTERRUPTED_COMPLETED:
		DEBUG_TRACE("PLAYER_INTERRUPTED_COMPLETED");
		/* ready to resume */
		if (ad->player_state == PLAY_STATE_PAUSED)
			mp_play_control_play_pause(ad, true);
		return;
		break;
	default:
		DEBUG_TRACE("Unhandled code: %d", code);
		break;
	}

	mp_play_pause(ad);
}

static void
_mp_play_control_error_cb(int error_code, void *userdata)
{
	startfunc;
	struct appdata *ad = userdata;
	MP_CHECK(ad);

	ERROR_TRACE("\n\nError from player");

	switch (error_code)
	{
	case PLAYER_ERROR_OUT_OF_MEMORY:
		ERROR_TRACE("PLAYER_ERROR_OUT_OF_MEMORY");
		break;
	case PLAYER_ERROR_INVALID_PARAMETER:
		ERROR_TRACE("PLAYER_ERROR_INVALID_PARAMETER");
		break;
	case PLAYER_ERROR_NOT_SUPPORTED_FILE:	//can receive error msg while playing.
		ERROR_TRACE("receive MM_ERROR_PLAYER_CODEC_NOT_FOUND");
		_mp_play_error_handler(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_UNSUPPORTED_FILETYPE"));
		break;
	case PLAYER_ERROR_CONNECTION_FAILED:
		ERROR_TRACE("MM_ERROR_PLAYER_STREAMING_CONNECTION_FAIL");
		mp_streaming_mgr_buffering_popup_control(ad, FALSE);
		_mp_play_error_handler(ad, GET_SYS_STR("IDS_COM_POP_CONNECTION_FAILED"));
		break;
	default:
		ERROR_TRACE("error_code: %d", error_code);
		_mp_play_error_handler(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_UNSUPPORTED_FILETYPE"));
	}
}

static void
_mp_play_control_buffering_cb(int percent, void *userdata)
{
	startfunc;
	struct appdata *ad = userdata;
	MP_CHECK(ad);

	mp_debug("Buffering : %d%% \n", percent);

	bool is_show_buffering = true;
	if (percent >= 100)
		is_show_buffering = false;

	mp_streaming_mgr_buffering_popup_control(ad, is_show_buffering);
}

static void
_mp_play_control_prepare_cb(void *userdata)
{
	startfunc;
	struct appdata *ad = userdata;
	MP_CHECK(ad);

	mp_play_current_file(ad);
}

bool
mp_player_control_ready_new_file(void *data, bool check_drm)
{
	startfunc;

	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);

	mp_plst_item *item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK_FALSE(item);

	bool is_drm = FALSE;

	//mp_play_control_play_pause_icon_set(ad, TRUE);
	DEBUG_TRACE("current item pathname : [%s]", item->uri);
	if (mp_util_check_uri_available(item->uri))
	{
		mp_debug("http uri path");
	}
	else	if (!mp_check_file_exist(item->uri))
	{
		ERROR_TRACE("There is no such file\n");
		_mp_play_error_handler(ad, GET_SYS_STR("IDS_COM_POP_FILE_NOT_EXIST"));
		return FALSE;
	}

	//DRM File Check
	if (check_drm)
	{
		if (mp_drm_file_right(item->uri))
		{
			DEBUG_TRACE("This is DRM Contents\n");
			is_drm = TRUE;

			if (!mp_drm_check_left_ro(ad, item->uri))	//drm pop-up  raised.
				return FALSE;

			if (!mp_drm_check_forward(ad, item->uri))
				return FALSE;
		}
	}
	else
		is_drm = TRUE;
#ifdef MP_FEATURE_DRM_CONSUMPTION
	if (is_drm)
		mp_drm_set_consumption(TRUE);
#endif

	ad->player_state = PLAY_STATE_NONE;

	mp_player_type_e player_type = MP_PLAYER_TYPE_MMFW;
	void *extra_data = NULL;

	if (!mp_player_mgr_create(ad, item->uri, player_type, extra_data))
	{
		_mp_play_error_handler(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_ERROR_OCCURRED"));
		return FALSE;
	}

	mp_player_mgr_set_started_db(mp_play_start, ad);
	mp_player_mgr_set_completed_cb(_mp_play_control_completed_cb, ad);
	mp_player_mgr_set_interrupted_cb(_mp_play_control_interrupted_cb, ad);
	mp_player_mgr_set_error_cb(_mp_play_control_error_cb, ad);
	mp_player_mgr_set_buffering_cb(_mp_play_control_buffering_cb, ad);
	mp_player_mgr_set_prepare_cb(_mp_play_control_prepare_cb, ad);
	mp_player_mgr_set_paused_cb(mp_play_pause, ad);


	if (!mp_player_mgr_realize(ad))
	{
		_mp_play_error_handler(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_ERROR_OCCURRED"));
		return FALSE;
	}

	return TRUE;
}


void
mp_play_control_play_pause(struct appdata *ad, bool play)
{
	mp_retm_if(ad == NULL, "appdata is NULL");

DEBUG_TRACE("%s:+++:play=%d\n", __func__, play); //Minjin

	DEBUG_TRACE("play [%d], ad->player_state: %d", play, ad->player_state);

	if (play)
	{
		ad->paused_by_user = FALSE;

		if (ad->player_state == PLAY_STATE_PAUSED)
		{
			if(mp_player_mgr_resume(ad))
			{
				vconf_set_int(MP_VCONFKEY_PLAYING_PID, getpid());
				if (ad->player_state == PLAY_STATE_PAUSED)
					mp_play_resume(ad);
				ad->player_state = PLAY_STATE_PLAYING;
			}
		}
		else if (ad->player_state == PLAY_STATE_READY)
		{
			mp_play_current_file(ad);
		}
		else if (ad->player_state == PLAY_STATE_PLAYING)
		{
			DEBUG_TRACE("player_state is already playing. Skip event");
		}
		else if (ad->player_state == PLAY_STATE_PREPARING)
		{
			WARN_TRACE("player_state is preparing. Skip event");
		}
		else
		{
			//silentmode -> go to listview -> click one track -> silent mode play no -> go to playing view -> click play icon
			mp_play_new_file(ad, TRUE);
		}
	}
	else
	{
		if (ad->player_state == PLAY_STATE_PLAYING)
		{
			if(mp_player_mgr_pause(ad))
			{
				ad->paused_by_user = TRUE;
			}
		}
		else if (ad->player_state == PLAY_STATE_PREPARING)
		{
			WARN_TRACE("player_state is prepareing. set paused_by_user!!!");
			ad->paused_by_user = TRUE;
		}
	}

DEBUG_TRACE("%s:---\n", __func__); //Minjin	

}

void mp_player_control_stop(struct appdata *ad)
{
	startfunc;
	mp_player_mgr_stop(ad);
}

void
mp_play_control_ff_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	mp_retm_if(ad == NULL, "appdata is NULL");
	DEBUG_TRACE("mp_play_control_ff_cb [%s]\n", emission);

	ad->is_ff = TRUE;

	if (!strcmp(emission, "ff_btn_down") && !strcmp(source, CTR_EDJ_SIG_SRC))
	{
		if(!ad->seek_off)
			_mp_play_control_add_longpressed_timer(ad);
	}
	else if (!strcmp(emission, "ff_btn_up") && !strcmp(source, CTR_EDJ_SIG_SRC))
	{
		_mp_play_control_del_longpressed_timer(ad);

		if (ad->is_Longpress)
		{
			ad->is_Longpress = false;
#ifdef MP_FEATURE_AVRCP_13
			if(ad->player_state == PLAY_STATE_PLAYING)
				mp_avrcp_noti_player_state(MP_AVRCP_STATE_PLAYING);
			else if(ad->player_state == PLAY_STATE_PAUSED)
				mp_avrcp_noti_player_state(MP_AVRCP_STATE_PAUSED);
			else
				mp_avrcp_noti_player_state(MP_AVRCP_STATE_STOPPED);
#endif
		}
		else
		{
#ifdef ENABLE_RICHINFO
#ifdef MP_SOUND_PLAYER
			if(ad->info_ug_base) {
				mp_play_view_unswallow_info_ug_layout(ad);
				sp_view_mgr_pop_view_to(ad->view_mgr, SP_VIEW_TYPE_PLAY);
			}

			MP_CHECK(ad->playing_view);
			evas_object_show(ad->playing_view->layout);
#else
			mp_view_manager_pop_info_view(ad);
#endif
#endif
			if(ad->playing_view)
				ad->playing_view->flick_direction = 1;

			mp_play_next_file(ad, TRUE);
		}
	}


}

void
mp_play_control_rew_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	DEBUG_TRACE("mp_play_control_rew_cb [%s]\n", emission);

	ad->is_ff = FALSE;

	if (!strcmp(emission, "rew_btn_down"))
	{
		if(!ad->seek_off)
			_mp_play_control_add_longpressed_timer(ad);
	}
	else if (!strcmp(emission, "rew_btn_up"))
	{
		_mp_play_control_del_longpressed_timer(ad);

		if (ad->is_Longpress)
		{
			ad->is_Longpress = false;
#ifdef MP_FEATURE_AVRCP_13
			if(ad->player_state == PLAY_STATE_PLAYING)
				mp_avrcp_noti_player_state(MP_AVRCP_STATE_PLAYING);
			else if(ad->player_state == PLAY_STATE_PAUSED)
				mp_avrcp_noti_player_state(MP_AVRCP_STATE_PAUSED);
			else
				mp_avrcp_noti_player_state(MP_AVRCP_STATE_STOPPED);
#endif
		}
		else
		{
			int pos = mp_player_mgr_get_position();
			if (pos > 3000 ||mp_playlist_mgr_count(ad->playlist_mgr) == 1)
			{
				mp_player_mgr_set_position(0, NULL, NULL);

				if (!ad->paused_by_user && ad->player_state == PLAY_STATE_PAUSED)
				{
					if(mp_player_mgr_resume(ad))
					{
						vconf_set_int(MP_VCONFKEY_PLAYING_PID, getpid());
						if (ad->player_state == PLAY_STATE_PAUSED)
							mp_play_resume(ad);
						ad->player_state = PLAY_STATE_PLAYING;
					}
				}
				else
				{
					ad->music_pos = 0;
					mp_play_view_update_progressbar(ad);
				}
				return;
			}


#ifdef ENABLE_RICHINFO
#ifdef MP_SOUND_PLAYER
			if(ad->info_ug_base) {
				mp_play_view_unswallow_info_ug_layout(ad);
				sp_view_mgr_pop_view_to(ad->view_mgr, SP_VIEW_TYPE_PLAY);
			}

			MP_CHECK(ad->playing_view);
			evas_object_show(ad->playing_view->layout);
#else
			mp_view_manager_pop_info_view(ad);
#endif
#endif
			if(ad->playing_view)
				ad->playing_view->flick_direction = 2;
			mp_play_prev_file(ad);

		}
	}

}

#define MP_LONG_PRESS_TIMEOUT 1.0
#define MUTE_POPUP_INTERVAL_TIME				3.0

static Eina_Bool
_mp_play_control_volume_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	ad->volume_long_pressed = true;
	mp_ecore_timer_del(ad->volume_down_timer);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);
	//previous status is mute
	if (ad->mute_flag)
	{
		edje_object_signal_emit(_EDJ(playing_view->play_icon), "unmute", "volume");
		edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_HIDE, "*");
		ad->mute_flag = false;
	}
	else
	{		//previous status is unmute
		edje_object_signal_emit(_EDJ(playing_view->play_icon), "mute", "volume");
		edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_SHOW, "*");
		ad->mute_flag = true;
		mp_ecore_timer_del(ad->mute_popup_show_timer);
		ad->mute_popup_show_timer =
			ecore_timer_add(MUTE_POPUP_INTERVAL_TIME, _mp_play_mute_popup_cb, (void *)ad);
	}

	return EINA_FALSE;
}

void
mp_play_control_volume_down_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	//initilize flag data
	ad->volume_long_pressed = false;

	mp_ecore_timer_del(ad->volume_down_timer);

	ad->volume_down_timer = ecore_timer_add(MP_LONG_PRESS_TIMEOUT, _mp_play_control_volume_timer_cb, (void *)ad);

	return;
}

void
mp_play_control_volume_up_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_ecore_timer_del(ad->volume_down_timer);
	return;
}

static Eina_Bool
_mp_play_mute_popup_cb(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);
	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	mp_ecore_timer_del(ad->mute_popup_show_timer);

	edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_HIDE, "*");
	endfunc;
	return EINA_FALSE;
}

void
mp_play_control_volume_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_ecore_timer_del(ad->volume_down_timer);

	DEBUG_TRACE("volume_long_pressed=%d,mute_flag=%d", ad->volume_long_pressed, ad->mute_flag);
	if (ad->volume_long_pressed)
	{			//longpress volume button
		ad->volume_long_pressed = false;
	}
	else if (ad->mute_flag)
	{			//previous status is mute
		mp_playing_view *playing_view = ad->playing_view;
		MP_CHECK(playing_view);
		edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_SHOW, "*");
		mp_ecore_timer_del(ad->mute_popup_show_timer);
		ad->mute_popup_show_timer =
			ecore_timer_add(MUTE_POPUP_INTERVAL_TIME, _mp_play_mute_popup_cb, (void *)ad);
	}
	else {
		mp_play_view_volume_widget_show(ad, true);
	}

	return;
}

void
mp_play_control_menu_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;

	DEBUG_TRACE("mp_play_control_menu_cb with[%s]\n", emission);

#if 0	//Minjin
	if (!strcmp(emission, SIGNAL_INFO))
	{
	}
	else if (!strcmp(emission, SIGNAL_MAINALBUM_CLICKED))
	{
	}
	else if (!strcmp(emission, SIGNAL_SHUFFLE))
	{			// TURN OFF SHUFFLE
		if (!mp_setting_set_shuffle_state(FALSE))
		{
			mp_play_control_shuffle_set(ad, FALSE);
		}
		mp_playlist_mgr_set_shuffle(ad->playlist_mgr, FALSE);
#ifdef MP_FEATURE_AVRCP_13
		mp_avrcp_noti_shuffle_mode(MP_AVRCP_SHUFFLE_OFF);
#endif
	}
	else if (!strcmp(emission, SIGNAL_SHUFNON))
	{			// TURN ON SHUFFE

		if (!mp_setting_set_shuffle_state(TRUE))
		{
			mp_play_control_shuffle_set(ad, TRUE);
		}
		mp_playlist_mgr_set_shuffle(ad->playlist_mgr, TRUE);
#ifdef MP_FEATURE_AVRCP_13
		mp_avrcp_noti_shuffle_mode(MP_AVRCP_SHUFFLE_ON);
#endif
	}
	else if (!strcmp(emission, SIGNAL_REPALL))	//off -1 - all - off
	{
		if (!mp_setting_set_repeat_state(MP_SETTING_REP_NON))
			mp_play_control_repeat_set(ad, MP_SETTING_REP_NON);

		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_NONE);
#ifdef MP_FEATURE_AVRCP_13
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_OFF);
#endif
	}
	else if (!strcmp(emission, SIGNAL_REPNON))
	{
		if (!mp_setting_set_repeat_state(MP_SETTING_REP_1))
			mp_play_control_repeat_set(ad, MP_SETTING_REP_1);

		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_ONE);
#ifdef MP_FEATURE_AVRCP_13
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_ONE);
#endif
	}
	else if (!strcmp(emission, SIGNAL_REP1))
	{
		if (!mp_setting_set_repeat_state(MP_SETTING_REP_ALL))
			mp_play_control_repeat_set(ad, MP_SETTING_REP_ALL);

		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_ALL);
#ifdef MP_FEATURE_AVRCP_13
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_ALL);
#endif
	}
#else	
	//TODO: push app-relay 
	//DEBUG_TRACE("%s:+++\n", __func__);
	if (ad->player_state == PLAY_STATE_PLAYING)
	{
		if(mp_player_mgr_pause(ad))
		{
			ad->paused_by_user = TRUE;
		}
	}
	else if (ad->player_state == PLAY_STATE_PREPARING)
	{
		WARN_TRACE("player_state is prepareing. set paused_by_user!!!");
		ad->paused_by_user = TRUE;
	}

	vconf_set_bool("db/private/org.tizen.menu-screen/app_relay", 1);
#endif	
}


void
mp_play_control_end_of_stream(void *data)
{
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	ad->music_pos = ad->music_length;
	mp_play_view_update_progressbar(ad);

	mp_play_next_file(ad, FALSE);
}



void
mp_play_control_play_pause_icon_set(void *data, bool play_enable)
{
	struct appdata *ad = data;
	Evas_Object *buttons = NULL;
	mp_retm_if(ad == NULL, "appdata is NULL");
	mp_retm_if(ad->playing_view == NULL, "playing_view is NULL");
	mp_retm_if(ad->playing_view->play_control == NULL, "play_contol is NULL");

	buttons = evas_object_data_get(ad->playing_view->play_control, "buttons");
	mp_retm_if(buttons == NULL, "button is NULL");

	if (play_enable)
	{
		edje_object_signal_emit(_EDJ(buttons), "play", CTR_PROG_SIG_SRC);
	}
	else
	{
		edje_object_signal_emit(_EDJ(buttons), "pause", CTR_PROG_SIG_SRC);
	}

}

void
mp_play_control_shuffle_set(void *data, bool shuffle_enable)
{
	struct appdata *ad = data;

	mp_retm_if(ad == NULL, "appdata is NULL");
	mp_retm_if(ad->playing_view == NULL, "playing_view is NULL");
	mp_retm_if(ad->playing_view->play_menu == NULL, "play_menu is NULL");

	if (shuffle_enable)
	{
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "shuffle_visible", "shuffle");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "shuffle_non_invisible", "shuffle_non");
	}
	else
	{
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "shuffle_invisible", "shuffle");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "shuffle_non_visible", "shuffle_non");
	}
}


void
mp_play_control_repeat_set(void *data, int repeat_state)
{
	struct appdata *ad = data;

	mp_retm_if(ad == NULL, "appdata is NULL");
	mp_retm_if(ad->playing_view == NULL, "playing_view is NULL");
	mp_retm_if(ad->playing_view->play_menu == NULL, "play_menu is NULL");

	if (repeat_state == MP_PLST_REPEAT_ALL)
	{
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_all_visible", "rep_all");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_1_invisible", "rep_1");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_non_invisible", "rep_non");
		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_ALL);
	}
	else if (repeat_state == MP_PLST_REPEAT_ONE)
	{
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_all_invisible", "rep_all");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_1_visible", "rep_1");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_non_invisible", "rep_non");
		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_ONE);
	}
	else if (repeat_state == MP_PLST_REPEAT_NONE)
	{
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_all_invisible", "rep_all");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_1_invisible", "rep_1");
		edje_object_signal_emit(_EDJ(ad->playing_view->play_menu), "rep_non_visible", "rep_non");
		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_NONE);
	}
	else
		ERROR_TRACE("Error when set repeat\n");

}

