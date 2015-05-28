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

#include "music.h"
#include "mp-streaming-mgr.h"
#include "mp-http-mgr.h"
#include "mp-player-debug.h"
#include "mp-play.h"
#include "mp-widget.h"
#include "mp-player-mgr.h"
#include "mp-util.h"

static bool _mp_streaming_mgr_play_streaming_real(struct appdata *ad);

static void
_mp_streaming_mgr_utils_show_buffering_popup(void *data)
{
	DEBUG_TRACE("");
	MP_CHECK(data);
	struct appdata *ad = (struct appdata *)data;
	if (ad->buffering_popup)
	{
		evas_object_del(ad->buffering_popup);
		ad->buffering_popup = NULL;
	}

	Evas_Object *progressbar = NULL;
	progressbar = elm_progressbar_add(ad->win_main);
	elm_object_style_set(progressbar, "list_process");
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	evas_object_show(progressbar);

	if (ad->playing_view && ad->playing_view->play_view)
		elm_object_part_content_set(ad->playing_view->play_view, "buffering_area", progressbar);

	ad->buffering_popup = progressbar;
}

inline void
mp_streaming_mgr_buffering_popup_control(struct appdata *ad, bool is_show)
{
	startfunc;

	MP_CHECK(ad);

	if (is_show)
	{
		mp_debug("show buffering popup");
		/* show */
		if (!ad->buffering_popup)
			_mp_streaming_mgr_utils_show_buffering_popup(ad);
	}
	else
	{
		/* hide */
		if (ad->buffering_popup)
		{
			evas_object_del(ad->buffering_popup);
			ad->buffering_popup = NULL;
		}
	}
}

bool mp_streaming_mgr_check_streaming(struct appdata *ad, const char *path)
{
	MP_CHECK_FALSE(path);

	if (!mp_check_file_exist(path) && mp_util_check_uri_available(path)) {
		mp_debug("streaming....");
		return TRUE;
	}

	return FALSE;
}

bool mp_streaming_mgr_set_attribute(struct appdata *ad, player_h player)
{
	startfunc;

	return TRUE;
}

bool mp_streaming_mgr_play_new_streaming(struct appdata *ad)
{
	startfunc;
	MP_CHECK_FALSE(ad);

	bool ret = FALSE;
	MpHttpState_t state = mp_http_mgr_get_state(ad);
	if ( state == MP_HTTP_STATE_OFF) {
		mp_widget_text_popup(ad, GET_SYS_STR("IDS_COM_POP_CONNECTION_FAILED"));
		return FALSE;
	}
	else {	/* connected */
		mp_streaming_mgr_buffering_popup_control(ad, TRUE);
		ret = _mp_streaming_mgr_play_streaming_real(ad);
	}

	if (ret)
		mp_streaming_mgr_buffering_popup_control(ad, FALSE);

	return ret;
}

static bool _mp_streaming_mgr_play_streaming_real(struct appdata *ad)
{
	startfunc;
	MP_CHECK_FALSE(ad);

	return mp_play_new_file_real(ad, TRUE);
}

