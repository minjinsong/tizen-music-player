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


#include <minicontrol-provider.h>
#include "mp-minicontroller.h"
#include "mp-player-control.h"
#include "mp-play.h"
#include "Ecore.h"
#include "mp-player-mgr.h"
#include "mp-util.h"
#include "mp-widget.h"
#include "mp-setting-ctrl.h"

#define MINI_CONTROLLER_WIDTH (692)
#define MINI_CONTROLLER_WIDTH_LANDSCAPE (1280)
#define MINI_CONTROLLER_HEIGHT (178)
#define WL_INDI_H 27		//Window Layout Indicator Height
#define PAUSE_TIME_OUT 120.

#define CTR_EDJ_SIG_SRC "ctrl_edj"
#define CTR_PROG_SIG_SRC "ctrl_prog"

#define BUFFER_MAX		256

static void _minicontroller_action_cb(void *data, Evas_Object * obj, const char *emission, const char *source);
static Evas_Object *_load_edj(Evas_Object * parent, const char *file, const char *group);
static Evas_Object *_create_bgimg(Evas_Object * parent);
static Evas_Object *_load_minicontroller(struct appdata *ad);

static inline void
_mp_minicontroller_update_elapsed_time(struct appdata *ad)
{
	MP_CHECK(ad);
	MP_CHECK(ad->minicontroller_layout);


	int sec = mp_player_mgr_get_position() / 1000;
	int min = sec / 60;
	sec = sec % 60;

	char *time_text = g_strdup_printf("%02d:%02d", min, sec);
	if (time_text) {
		edje_object_part_text_set(_EDJ(ad->minicontroller_layout), "elm.elapsed_time", time_text);
		free(time_text);
		time_text = NULL;
	}
}

static Eina_Bool
_minicontroller_update_progresstime_cb(void *data)
{
	struct appdata *ad = data;
	mp_retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "appdata is NULL");

	if (ad->player_state == PLAY_STATE_PLAYING)
	{
		_mp_minicontroller_update_elapsed_time(ad);
	}

	return ECORE_CALLBACK_RENEW;
}

static void
_minicontroller_progress_timer_add(void *data)
{
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");
	DEBUG_TRACE();

	mp_ecore_timer_del(ad->minicon_progress_timer);

	_mp_minicontroller_update_elapsed_time(ad);
	if (ad->player_state == PLAY_STATE_PLAYING)
		ad->minicon_progress_timer = ecore_timer_add(1.0, _minicontroller_update_progresstime_cb, ad);
}

static void
_minicontroller_action_cb(void *data, Evas_Object * obj, const char *emission, const char *source)
{
	DEBUG_TRACE("emission: %s, source: %s\n", emission, source);

	struct appdata *ad = (struct appdata *)data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	if (emission)
	{
		if (!g_strcmp0(emission, "rew_btn_down"))
		{
			DEBUG_TRACE("REW");
			mp_play_control_rew_cb(data, obj, "rew_btn_down", CTR_EDJ_SIG_SRC);

		}
		else if (!g_strcmp0(emission, "rew_btn_up"))
		{
			DEBUG_TRACE("REW_up");
			mp_play_control_rew_cb(data, obj, "rew_btn_up", CTR_EDJ_SIG_SRC);
		}
		else if (!g_strcmp0(emission, "play_btn_clicked")) {
			DEBUG_TRACE("PLAY clicked");
			mp_play_control_play_pause(data, true);
		}
		else if (!g_strcmp0(emission, "pause_btn_clicked")) {
			DEBUG_TRACE("PAUSE clicked");
			mp_play_control_play_pause(data, false);
		}
		else if (!g_strcmp0(emission, "ff_btn_down"))
		{
			DEBUG_TRACE("FWD_down");
			mp_play_control_ff_cb(data, obj, "ff_btn_down", CTR_EDJ_SIG_SRC);

		}
		else if (!g_strcmp0(emission, "ff_btn_up"))
		{
			DEBUG_TRACE("FWD_up");
			mp_play_control_ff_cb(data, obj, "ff_btn_up", CTR_EDJ_SIG_SRC);
		}
		else if (!g_strcmp0(emission, "repeat"))
		{
			DEBUG_TRACE("REPEAT");
			int repeat_state = 0;
			mp_setting_get_repeat_state(&repeat_state);
			repeat_state++;
			repeat_state %= 3;
			mp_setting_set_repeat_state(repeat_state);
			mp_play_control_repeat_set(ad, repeat_state);
		}
		else if (!g_strcmp0(emission, "shuffle"))
		{
			DEBUG_TRACE("SHUFFLE");
			int shuffle_state = 0;
			mp_setting_get_shuffle_state(&shuffle_state);
			shuffle_state = !shuffle_state;
			mp_setting_set_shuffle_state(shuffle_state);
			mp_play_control_shuffle_set(ad, shuffle_state);
		}
		else if (!g_strcmp0(emission, "albumart_clicked"))
		{
			DEBUG_TRACE("albumart");
			ad->load_play_view = true;
			elm_win_activate(ad->win_main);
			return;
		}
	}



}

static Evas_Object *
_load_edj(Evas_Object * parent, const char *file, const char *group)
{
	Evas_Object *eo;
	int r;

	eo = elm_layout_add(parent);
	if (eo)
	{
		r = elm_layout_file_set(eo, file, group);
		if (!r)
		{
			evas_object_del(eo);
			return NULL;
		}
		evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(parent, eo);
		evas_object_show(eo);
	}

	return eo;
}

static Evas_Object *
_create_bgimg(Evas_Object * parent)
{
	Evas_Object *bg;

	mp_retvm_if(parent == NULL, NULL, "parent is NULL");

	DEBUG_TRACE_FUNC();

	bg = elm_bg_add(parent);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, bg);
	evas_object_color_set(bg, 37, 37, 37, 255);
	evas_object_show(bg);

	return bg;
}

static Evas_Object *
_load_minicontroller(struct appdata *ad)
{
	DEBUG_TRACE_FUNC();
	mp_retvm_if(ad == NULL, NULL, "appdata is NULL");
	Evas_Object *win = NULL;
	Evas_Object *eo = NULL;
	Evas_Object *icon = NULL;

	win = minicontrol_win_add("musicplayer-mini");
	if (!win)
		return NULL;

	double scale = elm_config_scale_get();
	evas_object_resize(win, MINI_CONTROLLER_WIDTH * scale, MINI_CONTROLLER_HEIGHT * scale);

	_create_bgimg(win);
	ad->win_minicon = win;

	/* load edje */
	eo = _load_edj(win, MINICON_EDJ_NAME, "music-minicontroller");
	if (!eo)
		return NULL;

	elm_win_resize_object_add(win, eo);

	icon = elm_icon_add(eo);
	ad->minicon_icon = icon;
	edje_object_signal_callback_add(_EDJ(eo), "*", "*", _minicontroller_action_cb, ad);

	evas_object_show(win);
	//evas_object_show(eo);

	return eo;
}

static Eina_Bool
_hide_minicontroller_timer_cb(void *data)
{
	startfunc;
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);

	mp_minicontroller_destroy(ad);
	return EINA_FALSE;
}

int
mp_minicontroller_create(struct appdata *ad)
{
	DEBUG_TRACE_FUNC();
	mp_retvm_if(ad == NULL, -1, "appdata is NULL");

	if (!(ad->minicontroller_layout && ad->win_minicon))
	{
		ad->minicontroller_layout = _load_minicontroller(ad);
		if (ad->minicontroller_layout == NULL)
		{
			DEBUG_TRACE("ERROR");
			return -1;
		}
		mp_language_mgr_register_object(ad->minicontroller_layout, OBJ_TYPE_EDJE_OBJECT, "elm.text.app_name", "IDS_COM_BODY_MUSIC");
		elm_object_part_text_set(ad->minicontroller_layout, "elm.text.app_name", GET_SYS_STR("IDS_COM_BODY_MUSIC"));
	}

	int angle = elm_win_rotation_get(ad->win_minicon);
	mp_minicontroller_rotate(ad, angle);

	mp_minicontroller_show(ad);
	return 0;
}


int
mp_minicontroller_show(struct appdata *ad)
{
	DEBUG_TRACE("minicontroller view show!!");
	mp_retvm_if(ad == NULL, -1, "appdata is NULL");
	MP_CHECK_VAL(ad->win_minicon, -1);

	ad->b_minicontroller_show = TRUE;
	mp_minicontroller_update(ad);
	evas_object_show(ad->win_minicon);
	return 0;

}

void
mp_minicontroller_update_control(struct appdata *ad)
{
	startfunc;
	mp_retm_if(ad == NULL, "appdata is NULL");
	MP_CHECK(ad->win_minicon);

	if (ad->player_state == PLAY_STATE_PLAYING)
	{
		edje_object_signal_emit(elm_layout_edje_get(ad->minicontroller_layout), "set_pause", "c_source");
	}
	else
	{
		edje_object_signal_emit(elm_layout_edje_get(ad->minicontroller_layout), "set_play", "c_source");
	}
}

void
mp_minicontroller_update(struct appdata *ad)
{

	DEBUG_TRACE();
	mp_retm_if(ad == NULL, "appdata is NULL");
	MP_CHECK(ad->win_minicon);

	mp_ecore_timer_del(ad->minicon_timer);

	if (ad->player_state == PLAY_STATE_PLAYING)
	{
		edje_object_signal_emit(elm_layout_edje_get(ad->minicontroller_layout), "set_pause", "c_source");

		if (ad->minicon_visible)
			_minicontroller_progress_timer_add(ad);
	}
	else
	{
		edje_object_signal_emit(elm_layout_edje_get(ad->minicontroller_layout), "set_play", "c_source");
		ad->minicon_timer = ecore_timer_add(PAUSE_TIME_OUT, _hide_minicontroller_timer_cb, ad);
	}

	mp_track_info_t *current_item = ad->current_track_info;
	if (current_item) {

		DEBUG_TRACE("album art is %s", current_item->thumbnail_path);
		if (mp_util_is_image_valid(ad->evas, current_item->thumbnail_path))
			elm_image_file_set(ad->minicon_icon, current_item->thumbnail_path, NULL);
		else
			elm_image_file_set(ad->minicon_icon, DEFAULT_THUMBNAIL, NULL);
		edje_object_part_swallow(_EDJ(ad->minicontroller_layout), "albumart_image", ad->minicon_icon);

		char *title = g_strdup_printf("%s / %s", current_item->title, current_item->artist);
		edje_object_part_text_set(_EDJ(ad->minicontroller_layout), "elm.text", title);
		SAFE_FREE(title);

		if (!ad->minicon_progress_timer)
			_mp_minicontroller_update_elapsed_time(ad);

		const char *signal = NULL;
		/* repeat state */
		int repeat_state = 0;
		mp_setting_get_repeat_state(&repeat_state);
		if (repeat_state == MP_SETTING_REP_ALL)
			signal = "set_repeat_all";
		else if (repeat_state == MP_SETTING_REP_1)
			signal = "set_repeat_one";
		else
			signal = "set_repeat_none";
		edje_object_signal_emit(_EDJ(ad->minicontroller_layout), signal, "c_source");

		/* shuffle state */
		int shuffle_state = false;
		mp_setting_get_shuffle_state(&shuffle_state);
		if (shuffle_state)
			signal = "set_shuffle_on";
		else
			signal = "set_shuffle_off";
		edje_object_signal_emit(_EDJ(ad->minicontroller_layout), signal, "c_source");

		evas_object_show(ad->minicontroller_layout);
	}
}

int
mp_minicontroller_hide(struct appdata *ad)
{
	DEBUG_TRACE("minicontroller view hide!!\n");
	mp_retvm_if(ad == NULL, -1, "appdata is NULL");
	MP_CHECK_VAL(ad->win_minicon, -1);

	evas_object_hide(ad->win_minicon);
	ad->b_minicontroller_show = FALSE;

	if (ad->minicon_progress_timer)
	{
		ecore_timer_del(ad->minicon_progress_timer);
		ad->minicon_progress_timer = NULL;
	}

	return 0;

}

int
mp_minicontroller_destroy(struct appdata *ad)
{
	DEBUG_TRACE("minicontroller view destroy!!");
	mp_retvm_if(ad == NULL, -1, "appdata is NULL");
	MP_CHECK_VAL(ad->win_minicon, -1);

	if (ad->minicontroller_layout != NULL)
	{
		evas_object_hide(ad->minicontroller_layout);
		evas_object_del(ad->minicontroller_layout);
		ad->minicontroller_layout = NULL;
		ad->b_minicontroller_show = FALSE;
	}

	if (ad->win_minicon)
	{
		evas_object_del(ad->win_minicon);
		ad->win_minicon = NULL;
	}

	mp_ecore_timer_del(ad->minicon_timer);

	ad->minicon_visible = false;

	return 0;
}

void
mp_minicontroller_rotate(struct appdata *ad, int angle)
{
	MP_CHECK(ad);
	MP_CHECK(ad->win_minicon);

	int w = 0;
	const char *signal = NULL;
	if (angle == 90 || angle == 270) {
		signal = "sig_set_landscape_mode";
		w = MINI_CONTROLLER_WIDTH_LANDSCAPE;
	} else {
		signal = "sig_set_portrait_mode";
		w = MINI_CONTROLLER_WIDTH;
	}

	edje_object_signal_emit(_EDJ(ad->minicontroller_layout), signal, "c_source");

	double scale = elm_config_scale_get();
	evas_object_resize(ad->win_minicon, w * scale, MINI_CONTROLLER_HEIGHT * scale);
	elm_win_rotation_with_resize_set(ad->win_minicon, angle);
}

void
mp_minicontroller_visible_set(struct appdata *ad, bool visible)
{
	MP_CHECK(ad);
	MP_CHECK(ad->win_minicon);

	ad->minicon_visible = visible;
	if (visible)
	{
		_minicontroller_progress_timer_add(ad);

	}
	else
	{
		mp_ecore_timer_del(ad->minicon_progress_timer);
	}
}

