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

#include <glib.h>
#include "music.h"
#include "mp-player-mgr.h"
#include "mp-player-control.h"
#include "mp-play.h"
#include "mp-player-drm.h"
#include <player.h>

#include "mp-ta.h"
#include "mp-player-debug.h"
#include "mp-widget.h"
#include "mp-play-view.h"
#include "mp-streaming-mgr.h"
#include "mp-util.h"

#define MAX_PATH_LEN			1024

static player_h _player = 0;
static mp_player_type_e _player_type = MP_PLAYER_TYPE_MMFW;

static bool is_seeking = false;
static int g_reserved_seek_pos = -1;

static Seek_Done_Cb g_requesting_cb = NULL;
static void *g_requesting_cb_data = NULL;

static Seek_Done_Cb g_reserved_cb = NULL;
static void *g_reserved_cb_data = NULL;

static Ecore_Pipe *g_player_pipe = NULL;

typedef enum {
	MP_PLAYER_CB_TYPE_STARTED,
	MP_PLAYER_CB_TYPE_COMPLETED,
	MP_PLAYER_CB_TYPE_INTURRUPTED,
	MP_PLAYER_CB_TYPE_ERROR,
	MP_PLAYER_CB_TYPE_BUFFERING,
	MP_PLAYER_CB_TYPE_PREPARE,
	MP_PLAYER_CB_TYPE_PAUSED,
	MP_PLAYER_CB_TYPE_NUM,
} mp_player_cb_type;

typedef struct {
	/* player callbacks */
	mp_player_started_cb started_cb;
	player_completed_cb completed_cb;
	player_interrupted_cb interrupted_cb;
	player_error_cb error_cb;
	player_buffering_cb buffering_cb;
	player_prepared_cb prepare_cb;
	mp_player_paused_cb paused_cb;

	/* callback user data */
	void *user_data[MP_PLAYER_CB_TYPE_NUM];
} mp_player_cbs;

typedef struct {
	mp_player_cb_type cb_type;

	union {
		player_interrupted_code_e interrupted_code;
		int error_code;
		int percent;
	} param;
} mp_player_cb_extra_data;

typedef struct {
	int (*create)(player_h *);
	int (*destroy)(player_h);
	int (*prepare)(player_h);
	int (*prepare_async)(player_h, player_prepared_cb, void *);
	int (*unprepare)(player_h);
	int (*set_uri)(player_h, const char *);
	int (*get_state)(player_h, player_state_e *);
	int (*set_sound_type)(player_h, sound_type_e);
	int (*set_audio_latency_mode)(player_h, audio_latency_mode_e);
	int (*get_audio_latency_mode)(player_h, audio_latency_mode_e*);
	int (*start)(player_h);
	int (*pause)(player_h);
	int (*stop)(player_h);
	int (*set_started_cb)(player_h, mp_player_started_cb, void *);
	int (*set_completed_cb)(player_h, player_completed_cb, void *);
	int (*set_interrupted_cb)(player_h, player_interrupted_cb, void *);
	int (*set_error_cb)(player_h, player_error_cb, void *);
	int (*set_buffering_cb)(player_h, player_buffering_cb, void *);
	int (*set_paused_cb)(player_h, mp_player_paused_cb, void *);
	int (*set_position)(player_h, int, player_seek_completed_cb, void *);
	int (*set_play_rate)(player_h, float);
	int (*get_position)(player_h, int *);
	int (*get_duration)(player_h, int *);
	int (*set_mute)(player_h, bool);
} mp_player_api_s;
static mp_player_api_s g_player_apis;
#define CHECK_MMFW_PLAYER()	((_player_type == MP_PLAYER_TYPE_MMFW) ? true : false)

static mp_player_cbs *g_player_cbs = NULL;

bool
mp_player_mgr_is_active(void)
{
	return _player ? TRUE : FALSE;
}

void mp_player_mgr_set_started_db(mp_player_started_cb callback, void *user_data)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->started_cb = callback;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_STARTED] = user_data;
}

void mp_player_mgr_set_completed_cb(player_completed_cb  callback, void *user_data)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->completed_cb = callback;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_COMPLETED] = user_data;
}

void mp_player_mgr_set_interrupted_cb(player_interrupted_cb  callback, void *user_data)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->interrupted_cb = callback;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_INTURRUPTED] = user_data;
}

void mp_player_mgr_set_error_cb(player_error_cb  callback, void *user_data)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->error_cb = callback;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_ERROR] = user_data;
}

void mp_player_mgr_set_buffering_cb(player_buffering_cb  callback, void *user_data)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->buffering_cb = callback;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_BUFFERING] = user_data;
}

void mp_player_mgr_set_prepare_cb(player_prepared_cb callback, void *user_data)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->prepare_cb = callback;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_PREPARE] = user_data;
}

void mp_player_mgr_unset_completed_cb(void)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->completed_cb = NULL;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_COMPLETED] = NULL;
}

void mp_player_mgr_unset_interrupted_cb(void)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->interrupted_cb = NULL;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_INTURRUPTED] = NULL;
}

void mp_player_mgr_unset_error_cb(void)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->error_cb = NULL;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_ERROR] = NULL;
}

void mp_player_mgr_unset_buffering_cb(void)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->buffering_cb = NULL;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_BUFFERING] = NULL;
}

void mp_player_mgr_set_paused_cb(mp_player_paused_cb callback, void *user_data)
{
	if (!mp_player_mgr_is_active())
		return;

	MP_CHECK(g_player_cbs);

	g_player_cbs->paused_cb= callback;
	g_player_cbs->user_data[MP_PLAYER_CB_TYPE_PAUSED] = user_data;
}


player_state_e
mp_player_mgr_get_state(void)
{
	int ret = -1;
	player_state_e state_now = PLAYER_STATE_NONE;

	if (!_player)
		return state_now;

	ret = g_player_apis.get_state(_player, &state_now);
	return state_now;
}

static void
_mp_player_mgr_callback_pipe_handler(void *data, void *buffer, unsigned int nbyte)
{
	mp_player_cb_extra_data *extra_data = buffer;
	MP_CHECK(extra_data);
	MP_CHECK(g_player_cbs);

	switch (extra_data->cb_type) {
	case MP_PLAYER_CB_TYPE_STARTED:
		if (g_player_cbs->started_cb)
			g_player_cbs->started_cb(g_player_cbs->user_data[MP_PLAYER_CB_TYPE_STARTED]);
		break;

	case MP_PLAYER_CB_TYPE_COMPLETED:
		if (g_player_cbs->completed_cb)
			g_player_cbs->completed_cb(g_player_cbs->user_data[MP_PLAYER_CB_TYPE_COMPLETED]);
		break;

	case MP_PLAYER_CB_TYPE_INTURRUPTED:
		if (g_player_cbs->interrupted_cb)
			g_player_cbs->interrupted_cb(extra_data->param.interrupted_code, g_player_cbs->user_data[MP_PLAYER_CB_TYPE_INTURRUPTED]);
		break;

	case MP_PLAYER_CB_TYPE_ERROR:
		if (g_player_cbs->error_cb)
			g_player_cbs->error_cb(extra_data->param.error_code, g_player_cbs->user_data[MP_PLAYER_CB_TYPE_ERROR]);
		break;

	case MP_PLAYER_CB_TYPE_BUFFERING:
		if (g_player_cbs->buffering_cb)
			g_player_cbs->buffering_cb(extra_data->param.percent ,g_player_cbs->user_data[MP_PLAYER_CB_TYPE_BUFFERING]);
		break;

	case MP_PLAYER_CB_TYPE_PREPARE:
		if (g_player_cbs->prepare_cb)
			g_player_cbs->prepare_cb(g_player_cbs->user_data[MP_PLAYER_CB_TYPE_PREPARE]);
		break;

	case MP_PLAYER_CB_TYPE_PAUSED:
		if (g_player_cbs->paused_cb)
			g_player_cbs->paused_cb(g_player_cbs->user_data[MP_PLAYER_CB_TYPE_PAUSED]);
		break;

	default:
		WARN_TRACE("Not suppoted callback type [%d]", extra_data->cb_type);
	}
}

static void
_mp_player_mgr_started_cb(void *userdata)
{
	MP_CHECK(g_player_pipe);

	mp_player_cb_extra_data extra_data;
	memset(&extra_data, 0, sizeof(mp_player_cb_extra_data));
	extra_data.cb_type = MP_PLAYER_CB_TYPE_STARTED;

	ecore_pipe_write(g_player_pipe, &extra_data, sizeof(mp_player_cb_extra_data));
}

static void
_mp_player_mgr_completed_cb(void *userdata)
{
	MP_CHECK(g_player_pipe);

	mp_player_cb_extra_data extra_data;
	memset(&extra_data, 0, sizeof(mp_player_cb_extra_data));
	extra_data.cb_type = MP_PLAYER_CB_TYPE_COMPLETED;

	ecore_pipe_write(g_player_pipe, &extra_data, sizeof(mp_player_cb_extra_data));
}

static void
_mp_player_mgr_interrupted_cb(player_interrupted_code_e code, void *userdata)
{
	startfunc;
	MP_CHECK(g_player_pipe);

	mp_player_cb_extra_data extra_data;
	memset(&extra_data, 0, sizeof(mp_player_cb_extra_data));
	extra_data.cb_type = MP_PLAYER_CB_TYPE_INTURRUPTED;
	extra_data.param.interrupted_code = code;

	ecore_pipe_write(g_player_pipe, &extra_data, sizeof(mp_player_cb_extra_data));
}


static void
_mp_player_mgr_error_cb(int error_code, void *userdata)
{
	MP_CHECK(g_player_pipe);

	mp_player_cb_extra_data extra_data;
	memset(&extra_data, 0, sizeof(mp_player_cb_extra_data));
	extra_data.cb_type = MP_PLAYER_CB_TYPE_ERROR;
	extra_data.param.error_code = error_code;

	ecore_pipe_write(g_player_pipe, &extra_data, sizeof(mp_player_cb_extra_data));
}

static void
_mp_player_mgr_buffering_cb(int percent, void *userdata)
{
	MP_CHECK(g_player_pipe);

	mp_player_cb_extra_data extra_data;
	memset(&extra_data, 0, sizeof(mp_player_cb_extra_data));
	extra_data.cb_type = MP_PLAYER_CB_TYPE_BUFFERING;
	extra_data.param.percent = percent;

	ecore_pipe_write(g_player_pipe, &extra_data, sizeof(mp_player_cb_extra_data));
}

static void
_mp_player_mgr_prepare_cb(void *userdata)
{
	MP_CHECK(g_player_pipe);

	struct appdata *ad = (struct appdata *)userdata;
	MP_CHECK(ad);
	ad->player_state = PLAY_STATE_READY;

	mp_player_cb_extra_data extra_data;
	memset(&extra_data, 0, sizeof(mp_player_cb_extra_data));
	extra_data.cb_type = MP_PLAYER_CB_TYPE_PREPARE;

	ecore_pipe_write(g_player_pipe, &extra_data, sizeof(mp_player_cb_extra_data));
}

static void
_mp_player_mgr_paused_cb(void *userdata)
{
	MP_CHECK(g_player_pipe);

	mp_player_cb_extra_data extra_data;
	memset(&extra_data, 0, sizeof(mp_player_cb_extra_data));
	extra_data.cb_type = MP_PLAYER_CB_TYPE_PAUSED;

	ecore_pipe_write(g_player_pipe, &extra_data, sizeof(mp_player_cb_extra_data));
}

static void
_mp_player_mgr_change_player(mp_player_type_e player_type)
{
	_player_type = player_type;

	WARN_TRACE("player type = [%d]", _player_type);

	memset(&g_player_apis, 0x0, sizeof(mp_player_api_s));

	{	/* MP_PLAYER_TYPE_MMFW */
		g_player_apis.create = player_create;
		g_player_apis.destroy = player_destroy;
		g_player_apis.prepare = player_prepare;
		g_player_apis.prepare_async = player_prepare_async;
		g_player_apis.unprepare = player_unprepare;
		g_player_apis.set_uri = player_set_uri;
		g_player_apis.get_state = player_get_state;
		g_player_apis.set_sound_type = player_set_sound_type;
		g_player_apis.set_audio_latency_mode = player_set_audio_latency_mode;
		g_player_apis.get_audio_latency_mode = player_get_audio_latency_mode;
		g_player_apis.start = player_start;
		g_player_apis.pause = player_pause;
		g_player_apis.stop = player_stop;
		g_player_apis.set_started_cb = NULL;
		g_player_apis.set_completed_cb = player_set_completed_cb;
		g_player_apis.set_interrupted_cb = player_set_interrupted_cb;
		g_player_apis.set_error_cb = player_set_error_cb;
		g_player_apis.set_buffering_cb = player_set_buffering_cb;
		g_player_apis.set_paused_cb = NULL;
		g_player_apis.set_position = player_set_position;
		g_player_apis.get_position = player_get_position;
		g_player_apis.get_duration = player_get_duration;
		g_player_apis.set_mute = player_set_mute;
		g_player_apis.set_play_rate = player_set_playback_rate;
	}
}

bool
mp_player_mgr_create(void *data, const gchar * path, mp_player_type_e type, void *extra_data)
{
	struct appdata *ad = (struct appdata *)data;
	int path_len = strlen(path);

	int ret = PLAYER_ERROR_NONE;

	DEBUG_TRACE("mp_player_mgr_create with [%s]\n", path);

	if (path_len > 0 && path_len < MAX_PATH_LEN)
	{

		if (mp_player_mgr_is_active())
		{
			WARN_TRACE("Destroy previous player");
			mp_player_mgr_destroy(ad);
		}

		/* change player for playing in DMR */
		_mp_player_mgr_change_player(type);

		if (g_player_apis.create(&_player) != PLAYER_ERROR_NONE)
		{
			ERROR_TRACE("Error when mp_player_mgr_create\n");
			return FALSE;
		}
		/*avsysaudiosink volume table setting */

			ret = g_player_apis.set_uri(_player, path);

		if (ret != PLAYER_ERROR_NONE)
		{
			ERROR_TRACE("fail to set uri");
			mp_player_mgr_destroy(data);
			return FALSE;
		}

		if (g_player_apis.set_sound_type)
			g_player_apis.set_sound_type(_player, SOUND_TYPE_MEDIA);

		if (g_player_apis.set_audio_latency_mode)
			g_player_apis.set_audio_latency_mode(_player, AUDIO_LATENCY_MODE_HIGH);

		if (mp_streaming_mgr_check_streaming(ad, path)) {
			if (!mp_streaming_mgr_set_attribute(ad, _player)) {
				mp_error("streaming set attribute fail");
				mp_player_mgr_destroy(data);
				return FALSE;
			}
		}
	}
	else
	{

		return FALSE;
	}

	is_seeking = false;
	g_reserved_seek_pos = -1;

	if(!g_player_cbs)
	{
		g_player_cbs = calloc(1, sizeof(mp_player_cbs));
		mp_assert(g_player_cbs);
	}

	if (g_player_apis.set_started_cb)
		g_player_apis.set_started_cb(_player, _mp_player_mgr_started_cb, NULL);
	if (g_player_apis.set_completed_cb)
		g_player_apis.set_completed_cb(_player, _mp_player_mgr_completed_cb, NULL);
	if (g_player_apis.set_interrupted_cb)
		g_player_apis.set_interrupted_cb(_player, _mp_player_mgr_interrupted_cb, NULL);
	if (g_player_apis.set_error_cb)
		g_player_apis.set_error_cb(_player, _mp_player_mgr_error_cb, NULL);
	if (g_player_apis.set_buffering_cb)
		g_player_apis.set_buffering_cb(_player, _mp_player_mgr_buffering_cb, NULL);
	if (g_player_apis.set_paused_cb)
		g_player_apis.set_paused_cb(_player, _mp_player_mgr_paused_cb, NULL);

	if(!g_player_pipe)
		g_player_pipe = ecore_pipe_add(_mp_player_mgr_callback_pipe_handler, ad);

	ad->player_state = PLAY_STATE_CREATED;
	return TRUE;
}


bool
mp_player_mgr_destroy(void *data)
{
	struct appdata *ad = data;
	int res = true;

	if (!mp_player_mgr_is_active())
		return FALSE;

	if (g_player_apis.destroy(_player) != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_destroy\n");
		res = false;
	}


	_player = 0;
	ad->player_state = PLAY_STATE_NONE;
	if (!ad->freeze_indicator_icon && !mp_util_is_other_player_playing())
	{
		vconf_set_int(VCONFKEY_MUSIC_STATE, VCONFKEY_MUSIC_OFF);
	}
	else
		ad->freeze_indicator_icon = FALSE;

	is_seeking = false;
	g_reserved_seek_pos = -1;
	g_reserved_cb = NULL;
	g_reserved_cb_data = NULL;
	g_requesting_cb = NULL;
	g_requesting_cb_data = NULL;

	WARN_TRACE("player handle is destroyed..");
	return res;
}

bool
mp_player_mgr_realize(void *data)
{
	struct appdata *ad = data;

	if (!mp_player_mgr_is_active())
		return FALSE;

	if (g_player_apis.prepare_async(_player, _mp_player_mgr_prepare_cb, ad) != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_realize\n");
		return FALSE;
	}
	ad->player_state = PLAY_STATE_PREPARING;
	return TRUE;
}

bool
mp_player_mgr_unrealize(void *data)
{
	if (!mp_player_mgr_is_active())
		return FALSE;

	if (g_player_apis.unprepare(_player) != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_unrealize\n");
		return FALSE;
	}
	return TRUE;
}

bool
mp_player_mgr_play(void *data)
{
	startfunc;
	struct appdata *ad = data;
	int err = -1;

	if (!mp_player_mgr_is_active())
		return FALSE;

	err = g_player_apis.start(_player);
	if (err != PLAYER_ERROR_NONE)
	{
		if (err == PLAYER_ERROR_SOUND_POLICY)
		{
			mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_DURING_CALL"));
		}
		else
		{
			mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_ERROR_OCCURRED"));
		}

		ERROR_TRACE("Error when mp_player_mgr_play. err[%x]\n", err);
		return FALSE;
	}

	is_seeking = false;
	g_reserved_seek_pos = -1;

	//mp_play_start(ad);
	if (!g_player_apis.set_started_cb && g_player_cbs->started_cb)	/* sync */
		g_player_cbs->started_cb(g_player_cbs->user_data[MP_PLAYER_CB_TYPE_STARTED]);

	return TRUE;
}


bool
mp_player_mgr_stop(void *data)
{
	startfunc;
	struct appdata *ad = data;

	if (!mp_player_mgr_is_active())
		return FALSE;

	if (g_player_apis.stop(_player) != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_stop\n");
		return FALSE;
	}

	is_seeking = false;
	g_reserved_seek_pos = -1;

	mp_play_stop(ad);
	return TRUE;
}

bool
mp_player_mgr_resume(void *data)
{
	startfunc;
	struct appdata *ad = data;
	int err = -1;

	if (!mp_player_mgr_is_active())
		return FALSE;

	err = g_player_apis.start(_player);
	if (err != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_resume. err[%x]\n", err);
		if (err == PLAYER_ERROR_SOUND_POLICY)
		{
			mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_DURING_CALL"));
		}
		else
		{
			mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_TO_PLAY_ERROR_OCCURRED"));
		}
		return FALSE;
	}

	is_seeking = false;
	g_reserved_seek_pos = -1;

	mp_play_view_update_progressbar(data);
	mp_play_view_progress_timer_thaw(data);

	return TRUE;
}

bool
mp_player_mgr_pause(void *data)
{
	startfunc;
	int err = -1;

#if 1	//Minjin
DEBUG_TRACE("%s:+++\n", __func__);
struct appdata *ad = (struct appdata *)data;
vconf_set_dbl("memory/private/org.tizen.music-player/pos", ad->music_pos);
#endif
	if (!mp_player_mgr_is_active())
		return FALSE;

	err = g_player_apis.pause(_player);
	if (err != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_pause. err[%x]\n", err);
		return FALSE;
	}

	if (!g_player_apis.set_paused_cb && g_player_cbs->paused_cb)
		g_player_cbs->paused_cb(g_player_cbs->user_data[MP_PLAYER_CB_TYPE_PAUSED]);

	return TRUE;
}

static Eina_Bool
_mp_player_mgr_seek_done_idler(void *data)
{
	mp_debug("seek done[%d]", (int)data);

	if (!is_seeking) {
		mp_debug("seek canceled");
		return ECORE_CALLBACK_DONE;
	}

	is_seeking = false;

	if (g_requesting_cb) {
		/* invoke seek done callback */
		g_requesting_cb(g_requesting_cb_data);

		g_requesting_cb = NULL;
		g_requesting_cb_data = NULL;
	}

	if (g_reserved_seek_pos >= 0) {
		mp_debug("request reseved seek");
		mp_player_mgr_set_position(g_reserved_seek_pos, g_reserved_cb, g_reserved_cb_data);
		g_reserved_seek_pos = -1;
		g_reserved_cb = NULL;
		g_reserved_cb_data = NULL;
	}

	return ECORE_CALLBACK_DONE;
}

static void
_mp_player_mgr_seek_done_cb(void *data)
{
	if (is_seeking)
		ecore_idler_add(_mp_player_mgr_seek_done_idler, data);
}

bool
mp_player_mgr_set_position(guint pos, Seek_Done_Cb done_cb, void *data)
{
	if (!mp_player_mgr_is_active())
		return FALSE;

	if (is_seeking) {
		mp_debug("previous seek is NOT completed.. reserve seek[%d]", pos);
		g_reserved_seek_pos = pos;
		g_reserved_cb = done_cb;
		g_reserved_cb_data = data;
		return TRUE;
	}

	int err = g_player_apis.set_position(_player, (int)pos, _mp_player_mgr_seek_done_cb, (void *)pos);
	if (err != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error [0x%x] when mp_player_mgr_set_position(%d)\n", err, pos);
		return FALSE;
	}

	mp_debug("seek reqeuesting.. [%d]", pos);
	is_seeking = true;
	g_requesting_cb = done_cb;
	g_requesting_cb_data = data;

	return TRUE;
}

bool
mp_player_mgr_set_play_speed(double rate)
{
	int err = PLAYER_ERROR_NONE;
	if (!mp_player_mgr_is_active())
		return FALSE;

	if(g_player_apis.set_play_rate)
		err = g_player_apis.set_play_rate(_player, rate);
	else
		WARN_TRACE("Unsupported function");

	if (err != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error [0x%x] when set_playback_rate(%f)\n", err, rate);
		return FALSE;
	}
	return TRUE;
}

int
mp_player_mgr_get_position(void)
{
	int pos = 0;

	if (!mp_player_mgr_is_active())
		return 0;

	if (g_player_apis.get_position(_player, &pos) != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_get_position\n");
		return 0;
	}

	return pos;
}

int
mp_player_mgr_get_duration(void)
{
	if (!mp_player_mgr_is_active())
		return -1;

	int duration = 0;

	if (!mp_player_mgr_is_active())
		return 0;

	if (g_player_apis.get_duration(_player, &duration) != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("Error when mp_player_mgr_get_position\n");
		return 0;
	}

	return duration;
}


int
mp_player_mgr_vol_type_set(void)
{
	return sound_manager_set_volume_key_type(VOLUME_KEY_TYPE_MEDIA);
}

int
mp_player_mgr_vol_type_unset(void)
{
	return sound_manager_set_volume_key_type(VOLUME_KEY_TYPE_NONE);
}

bool
mp_player_mgr_session_init(void)
{
	int ret = SOUND_MANAGER_ERROR_NONE;

	ret = sound_manager_set_session_type(SOUND_SESSION_TYPE_SHARE);

	if (ret != SOUND_MANAGER_ERROR_NONE)
		return FALSE;

	return TRUE;
}

bool
mp_player_mgr_session_finish(void)
{
	return TRUE;
}

void
mp_player_mgr_set_mute(bool bMuteEnable)
{

	if (!mp_player_mgr_is_active())
		return;

	if (g_player_apis.set_mute(_player, bMuteEnable) != PLAYER_ERROR_NONE)
	{
		ERROR_TRACE("[ERR] mm_player_set_mute");
	}
}

mp_player_type_e
mp_player_mgr_get_player_type()
{
	return _player_type;
}

