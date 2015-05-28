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
#include "mp-play-view.h"
#include "mp-player-control.h"
#include "mp-play.h"
#include "mp-item.h"
#include "mp-player-drm.h"
#include "mp-smart-event-box.h"
#include "mp-file-tag-info.h"
#include "mp-player-mgr.h"
#include "mp-player-debug.h"
#include "mp-playlist-mgr.h"
#include "mp-util.h"
#include "mp-app.h"
#include "mp-menu.h"
#include "mp-widget.h"
#include "mp-popup.h"
#include "mp-ug-launch.h"
#include "mp-streaming-mgr.h"
#include "mp-ctxpopup.h"

#include "mp-minicontroller.h"
#include "mp-setting-ctrl.h"


#ifdef MP_SOUND_PLAYER
#include "sp-view-manager.h"
#else
#include "mp-common.h"
#include "mp-library.h"
#endif



#include "mp-volume.h"
#include "mp-volume-widget.h"


#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

#define MAIN_W			480
#define MAIN_H			800

#define HD_SCREEN_WIDTH 720.0
#define HD_INFO_RIGHT_WIDTH 550.0
#define HD_ALBUM_IMAGE_H_SCALE 720/1280

#define PROGRESS_BAR_POSITION 20	/*80 */
#define LS_PROGRESS_BAR_POSITION 340	/*400 */

#define CTR_EDJ_SIG_SRC "ctrl_edj"
#define CTR_PROG_SIG_SRC "ctrl_prog"
#define PLAYVIEW_TRANSIT_TIME (0.35)
#define FAVOUR_LONG_PRESS_TIME (1.5)

#define VOLUME_WIDGET_HIDE_TIME	(3.0)	/* sec */

#define ALBUMART_IMAGE_COLOR 160
#define ALBUMART_IMAGE_SHADOW_RGBA 60, 60, 60, 123
#define ALBUMART_IMAGE_SHADOW_RGBA1 100, 100, 100, 123
#define ALBUMART_IMAGE_SHADOW_RGBA2 0, 0, 0, 123
#define EVAS_OBJ_SHOW(obj) if(obj){evas_object_show(obj);}
#define EVAS_OBJ_HIDE(obj) if(obj){evas_object_hide(obj);}
#define EVAS_OBJ_MOVE(obj, x, y) if(obj){evas_object_move(obj, x, y);}

static void _mp_play_progress_val_set(void *data, double position);
static bool _mp_play_view_init_progress_bar(void *data);
static bool _mp_play_view_transit_by_item(struct appdata *ad, mp_plst_item *it, bool move_left);
static void _mp_play_view_destory_cb(void *data, Evas * e, Evas_Object * obj, void *event_info);
static Evas_Object *_mp_play_view_create_ctrl_layout(void *data, Evas_Object * parents, Evas_Object ** progress);
#ifndef MP_SOUND_PLAYER
static void _mp_play_view_start_request(void *data, Evas_Object * obj, void *event_info);
#endif
static void _mp_play_view_glist_free_cb(gpointer data);

static void
_mp_play_view_init(void *data)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	ad->show_optional_menu = FALSE;

	playing_view->layout = NULL;
	playing_view->play_view = NULL;
	playing_view->play_control = NULL;
	playing_view->play_menu = NULL;	//information, srs, shuffle, repeat
	playing_view->play_progressbar = NULL;
	playing_view->play_info = NULL;
	playing_view->albumart_img = NULL;

	playing_view->albumart_bg = NULL;
	playing_view->flick_direction = 0;
	ad->volume_long_pressed = false;

	playing_view->play_view_next = NULL;
	playing_view->x = 0;
	playing_view->y = 0;
	playing_view->favour_longpress = 0;
	playing_view->favourite_timer = NULL;
	playing_view->progressbar_timer = NULL;

	playing_view->play_view_screen_mode = MP_SCREEN_MODE_PORTRAIT;

	playing_view->transition_state = false;
	playing_view->b_play_all = false;

	endfunc;

	return;
}

void
mp_play_view_back_clicked_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");

	DEBUG_TRACE("ad->loadtype: %d", ad->loadtype);

	//reset mute flag
	ad->mute_flag = false;
	if (ad->playing_view)
		mp_evas_object_del(ad->playing_view->play_icon);

	{
#ifdef MP_SOUND_PLAYER
		mp_play_view_unswallow_info_ug_layout(ad);
		sp_view_mgr_pop_view_content(ad->view_mgr, FALSE);
#else
		view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
		MP_CHECK(view_data);
		mp_view_manager_pop_view_content(view_data, FALSE, FALSE);

		if (ad->music_setting_change_flag)
		{
			mp_util_set_library_controlbar_items(ad);
			ad->music_setting_change_flag = false;
		}
#endif
	}
	if (ad->buffering_popup)
		mp_streaming_mgr_buffering_popup_control(ad, false);
	evas_object_smart_callback_del(obj, "clicked", mp_play_view_back_clicked_cb);

	return;
}


static void
_mp_play_view_clear_animator(struct appdata *ad)
{
	MP_CHECK(ad);
	if(ad->minfo_ani)
	{
		ecore_animator_del(ad->minfo_ani);
		ad->minfo_ani = NULL;
	}

	if(ad->minfo_list)
	{
		g_list_free_full(ad->minfo_list, _mp_play_view_glist_free_cb);
		ad->minfo_list = NULL;
	}
}

void
mp_play_view_info_back_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");
	ad->info_click_flag = FALSE;
	mp_playing_view *playing_view = ad->playing_view;
	if(ad->info_ug)
	{
		int first_view = (int)evas_object_data_get(ad->info_ug_layout, "first_view");
		if(first_view)
		{
			ad->info_back_play_view_flag = TRUE;
#ifdef MP_SOUND_PLAYER
			mp_play_view_unswallow_info_ug_layout(ad);
			sp_view_mgr_pop_view_content(ad->view_mgr, FALSE);
#else
			view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
			mp_view_manager_pop_view_content(view_data, FALSE, FALSE);
#endif
			if (playing_view)
				playing_view->play_info = NULL;
		}
		else
		{
			ad->info_back_play_view_flag = FALSE;
			mp_ug_send_message(ad, MP_UG_MESSAGE_BACK);
		}
	}
	else
	{
		_mp_play_view_clear_animator(ad);
		ad->info_back_play_view_flag = TRUE;
#ifdef MP_SOUND_PLAYER
		mp_play_view_unswallow_info_ug_layout(ad);
		sp_view_mgr_pop_view_content(ad->view_mgr, FALSE);
#else
		view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
		mp_view_manager_pop_view_content(view_data, FALSE, FALSE);
#endif
		if (playing_view) {
			playing_view->play_info = NULL;

			mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, true);
		}
	}

	endfunc;
	return;
}

/* callback function when click "album" image*/
static void
_mp_play_view_sb_click_info_albumart(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	mp_play_view_info_back_cb(ad, NULL, NULL);
	endfunc;
}

typedef struct
{
	char *header;
	char *detail;
}mp_media_info_t;

static char *
_mp_play_view_info_gl_label_get(void *data, Evas_Object * obj, const char *part)
{
	mp_media_info_t *info = (mp_media_info_t *)data;
	char *text;

	if (!g_strcmp0(part, "elm.text.1"))
	{
		MP_CHECK_NULL(info);
		DEBUG_TRACE("%s", info->header);
		if(info->header && strstr(info->header, "IDS_COM"))
			text = GET_SYS_STR(info->header);
		else
			text = GET_STR(info->header);
		return g_strdup(text);
	}
	else if (!g_strcmp0(part, "elm.text.2"))
	{
		MP_CHECK_NULL(info);
		if(info->detail && strstr(info->detail, "IDS_COM"))
			text = GET_SYS_STR(info->detail);
		else
			text = GET_STR(info->detail);
		return g_strdup(text);
	}

	return NULL;
}

static void
_mp_play_view_info_gl_item_del(void *data, Evas_Object * obj)
{
	mp_media_info_t *info = data;
	MP_CHECK(info);
	IF_FREE(info->header);
	IF_FREE(info->detail);
	free(info);
}

static Elm_Genlist_Item_Class info_view_info_item_class = {
	.version = ELM_GENGRID_ITEM_CLASS_VERSION,
	.refcount = 0,
	.delete_me = EINA_FALSE,
	.item_style = "multiline/music_player/info",
	.func.text_get = _mp_play_view_info_gl_label_get,
	.func.del = _mp_play_view_info_gl_item_del,
	.func.content_get = NULL,
};

static Eina_Bool
_ecore_animator_cb(void *data)
{
	struct appdata *ad = NULL;
	Elm_Object_Item *item;

	ad = data;
	MP_CHECK_FALSE(ad);

	if(ad->minfo_list)
	{
		ad->minfo_list = g_list_first(ad->minfo_list);

		item = elm_genlist_item_append(ad->minfo_genlist, &info_view_info_item_class,
						       ad->minfo_list->data,
						       NULL,
						       ELM_GENLIST_ITEM_NONE,
						       NULL, NULL);
		if(item)
			elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		mp_language_mgr_register_genlist_item(item);

		ad->minfo_list = g_list_delete_link(ad->minfo_list, ad->minfo_list);
		return EINA_TRUE;
	}

	ad->minfo_ani = NULL;
	return EINA_FALSE;

}

static void
_mp_play_view_create_info_item(Evas_Object * genlist, char *header, char *detail)
{
	MP_CHECK(genlist);

	struct appdata *ad =  evas_object_data_get(genlist, "ad");
	MP_CHECK(ad);

	mp_media_info_t *info = calloc(1, sizeof(mp_media_info_t));
	MP_CHECK(info);
	info->header = g_strdup(header);
	info->detail= g_strdup(detail);

	if(!ad->minfo_ani)
	{
		DEBUG_TRACE("create animator");
		ad->minfo_ani = ecore_animator_add(_ecore_animator_cb, ad);
		ad->minfo_genlist = genlist;
	}

	ad->minfo_list = g_list_append(ad->minfo_list, info);

}

static void
_mp_play_view_glist_free_cb(gpointer data)
{
	startfunc;
	_mp_play_view_info_gl_item_del(data, NULL);
}

static void
_mp_play_view_append_drm_info(Evas_Object *genlist, const char *path)
{
	startfunc;
	MP_CHECK(genlist);
	MP_CHECK(path);

	GList *rs_list = mp_drm_get_right_status(path);
	if (rs_list) {
		mp_debug("show right_status");
		GList *current = rs_list;
		while (current) {
			mp_drm_right_status_t *rs = current->data;
			if (rs)
				_mp_play_view_create_info_item(genlist,
							      rs->type,
							      rs->validity);

			current = current->next;
		}

		mp_drm_free_right_status(rs_list);
		rs_list = NULL;
	}
}

static Evas_Object *
_mp_play_view_create_info_detail(Evas_Object * parent, void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_NULL(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_NULL(playing_view);

	mp_track_info_t *track_info = ad->current_track_info;
	MP_CHECK_NULL(track_info);

	int ret = 0;
	Evas_Object *genlist;

	_mp_play_view_clear_animator(ad);

	genlist = elm_genlist_add(parent);
	MP_CHECK_NULL(genlist);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	evas_object_data_set(genlist, "ad", ad);

	if (ret == 0)
	{

		if (track_info->artist && strlen(track_info->artist))
			_mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_ARTIST"), track_info->artist);

		_mp_play_view_create_info_item(genlist, ("IDS_COM_BODY_DETAILS_TITLE"), track_info->title);

		if (track_info->album && strlen(track_info->album))
			 _mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_ALBUM"), track_info->album);

		char duration_format[10] = { 0, };
		int dur_sec = track_info->duration / 1000;
		int sec = dur_sec % 60;
		int min = dur_sec / 60;
		snprintf(duration_format, sizeof(duration_format), "%02u:%02u", min, sec);

		_mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_TRACK_LENGTH"),
							      duration_format);


		if (track_info->genre && strlen(track_info->genre))
			 _mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_GENRE"), track_info->genre);

		if (track_info->author && strlen(track_info->author))
			_mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_AUTHOR"), track_info->author);

		if (track_info->copyright && strlen(track_info->copyright))
			 _mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_COPYRIGHT"), track_info->copyright);

		if (mp_drm_file_right(track_info->uri))
		{
			if (mp_drm_get_description(ad, track_info->uri))
				_mp_play_view_create_info_item(genlist,
									      ("IDS_MUSIC_BODY_DESCRIPTION"),
									      ad->drm_info.description);
			else
				_mp_play_view_create_info_item(genlist,
									      ("IDS_MUSIC_BODY_DESCRIPTION"),
									      "-");


			if (ad->drm_info.forward)
				_mp_play_view_create_info_item(genlist,
									      ("IDS_MUSIC_BODY_FORWARDING"),
									      ("IDS_MUSIC_BODY_POSSIBLE"));
			else
				_mp_play_view_create_info_item(genlist,
									      ("IDS_MUSIC_BODY_FORWARDING"),
									      ("IDS_MUSIC_BODY_IMPOSSIBLE"));

			/* right status */
			_mp_play_view_append_drm_info(genlist, track_info->uri);
		}

		if (track_info->track_num && strlen(track_info->track_num))
		{
			 _mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_TRACK_NUMBER"), track_info->track_num);
		}

		if (track_info->format && strlen(track_info->format))
		{

			_mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_FORMAT"), track_info->format);

		}

			_mp_play_view_create_info_item(genlist, ("IDS_MUSIC_BODY_MUSIC_LOCATION"),
							      track_info->uri);
	}

	evas_object_show(genlist);
	elm_object_part_content_set(parent, "mi_content", genlist);
	return genlist;
}

static void
mp_play_view_create_info_contents(Evas_Object * parent, void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_track_info_t *cur_item = ad->current_track_info;
	MP_CHECK(cur_item);

	/***********create left area start********************/
	Evas_Object *info_view_left_area = mp_common_load_edj(parent, PLAY_VIEW_EDJ_NAME, "info_view_left_area");
	MP_CHECK(info_view_left_area);
	elm_object_part_content_set(parent, "left_area", info_view_left_area);
	evas_object_show(info_view_left_area);

	Evas_Object *video_play_img = elm_image_add(info_view_left_area);
	elm_image_file_set(video_play_img, IMAGE_EDJ_NAME, MP_ICON_VIDEO_PLAY);
	elm_object_part_content_set(info_view_left_area, "video_play_img", video_play_img);
	evas_object_show(video_play_img);

	//add current albumart image
	Evas_Object *album_image = elm_image_add(info_view_left_area);
	if (mp_util_is_image_valid(ad->evas, cur_item->thumbnail_path))
	{
		elm_image_file_set(album_image, cur_item->thumbnail_path, NULL);
	}
	else
	{
		elm_image_file_set(album_image, DEFAULT_THUMBNAIL, NULL);
	}


	elm_object_part_content_set(info_view_left_area, "left_album_bg", album_image);
	evas_object_show(album_image);
	evas_object_event_callback_add(video_play_img, EVAS_CALLBACK_MOUSE_DOWN, NULL, NULL);
	evas_object_event_callback_add(video_play_img, EVAS_CALLBACK_MOUSE_MOVE, NULL, NULL);
	evas_object_event_callback_add(video_play_img, EVAS_CALLBACK_MOUSE_UP,
				       _mp_play_view_sb_click_info_albumart, ad);
	/***********create left area end********************/

	/***********create right area start********************/
	Evas_Object *right_base_box = mp_common_load_edj(parent, PLAY_VIEW_EDJ_NAME, "base_box_no_xml");
	MP_CHECK(right_base_box);
	elm_object_part_content_set(parent, "base_box", right_base_box);
	evas_object_show(right_base_box);

	Evas_Object *right_base_box_detail =
		mp_common_load_edj(right_base_box, PLAY_VIEW_EDJ_NAME, "mi_base_box_detail");
	MP_CHECK(right_base_box_detail);
	elm_object_part_content_set(right_base_box, "no_sim_media_content", right_base_box_detail);
	evas_object_show(right_base_box_detail);
	edje_object_part_text_set(_EDJ(right_base_box_detail), "title", GET_STR("IDS_MUSIC_BODY_DETAILS_MEADIA_INFO"));
	mp_language_mgr_register_object(right_base_box_detail, OBJ_TYPE_EDJE_OBJECT, "title", "IDS_MUSIC_BODY_DETAILS_MEADIA_INFO");
	Evas_Object *item = _mp_play_view_create_info_detail(right_base_box_detail, ad);
	MP_CHECK(item);
	elm_object_part_content_set(right_base_box_detail, "mi_scroller", item);
	/***********create right area end********************/

	//add scroller to right area
	elm_object_part_content_set(parent, "no_xml_no_sim_detail_scroller", right_base_box_detail);
}

static void
_mp_play_view_popup_bt_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;

	MP_CHECK(data);
	struct appdata *ad = (struct appdata *)data;

	int ret = sound_manager_set_active_route(SOUND_ROUTE_OUT_BLUETOOTH);
	if (ret != SOUND_MANAGER_ERROR_NONE)
		WARN_TRACE("Error: sound_manager_set_route_policy(0x%x)", ret);

	mp_popup_destroy(ad);
	endfunc;
}

static void
_mp_play_view_popup_headphone_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;

	MP_CHECK(data);
	struct appdata *ad = (struct appdata *)data;

	int ret = sound_manager_set_active_route(SOUND_ROUTE_OUT_WIRED_ACCESSORY);
	if (ret != SOUND_MANAGER_ERROR_NONE)
		WARN_TRACE("Error: sound_manager_set_route_policy(0x%x)", ret);

	mp_popup_destroy(ad);
	endfunc;
}

static void
_mp_play_view_popup_speaker_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;

	int ret = 0;
	ret = sound_manager_set_active_route(SOUND_ROUTE_OUT_SPEAKER);
	if (ret != SOUND_MANAGER_ERROR_NONE)
		WARN_TRACE("Error: sound_manager_set_route_policy(0x%x)", ret);

	mp_popup_destroy(ad);
	endfunc;
}

static void
_mp_play_view_append_snd_path_device(struct appdata *ad, char *bt_name, int bt_connected, int earjack, Evas_Object *popup)
{
	MP_CHECK(ad);
	MP_CHECK(popup);

	if (bt_connected)
		mp_genlist_popup_item_append(popup, bt_name, ad->radio_group, _mp_play_view_popup_bt_cb, ad);

	if (earjack)
		mp_genlist_popup_item_append(popup, GET_SYS_STR("IDS_COM_OPT_HEADPHONES_ABB"), ad->radio_group, _mp_play_view_popup_headphone_cb, ad);

	mp_genlist_popup_item_append(popup, GET_SYS_STR("IDS_COM_OPT_SPEAKER_ABB"), ad->radio_group, _mp_play_view_popup_speaker_cb, ad);

	mp_util_get_sound_path(&(ad->snd_path));
}

void
mp_play_view_update_snd_path(struct appdata *ad)
{
	startfunc;
	MP_CHECK(ad);

	bool popup_exist = false;
	bool bt_connected = false;
	int earjack = 0;
	static char *bt_name;
	int ret = 0;
	Evas_Object *popup = NULL;

	popup = ad->popup[MP_POPUP_GENLIST];
	if(popup)
	{
		popup_exist = evas_object_data_get(popup, "sound_path");
	}

	//update or destroy popup..
	if(popup_exist)
	{
		IF_FREE(bt_name);
		ret = sound_manager_get_a2dp_status(&bt_connected, &bt_name);
		if (ret != SOUND_MANAGER_ERROR_NONE)
		{
			WARN_TRACE("Fail to sound_manager_get_a2dp_status ret = [%d]", ret);
		}
		if (vconf_get_int(VCONFKEY_SYSMAN_EARJACK, &earjack))
			WARN_TRACE("Earjack state get Fail...");

		if (!bt_connected && !earjack)
		{
			evas_object_del(popup);
		}
		else
		{
			// TODO: updating list is not working now.. there is timing issue... complete it after MMFW support..
			evas_object_del(popup);
		}
	}

	//update button
	mp_play_view_set_snd_path_sensitivity(ad);

}

void
mp_play_view_set_snd_path_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;

	Evas_Object *group = NULL;
	Evas_Object *popup = NULL;

	int ret = 0;
	bool bt_connected = false;
	int earjack = 0;

	// use bt_name as static. bt_name can be used again in gl_label_get function when genlist of popup scrolled.
	static char *bt_name = NULL;

	popup = mp_genlist_popup_create(ad->win_main, MP_POPUP_SOUND_PATH, ad, ad);
	mp_retm_if(!popup, "Error: popup is null...");

	group = elm_radio_add(popup);
	ad->radio_group = group;

	Evas_Object *genlist = evas_object_data_get(popup, "genlist");
	MP_CHECK(genlist);
	evas_object_data_set(genlist, "ad", ad);

	IF_FREE(bt_name);
	ret = sound_manager_get_a2dp_status(&bt_connected, &bt_name);
	if (ret != SOUND_MANAGER_ERROR_NONE)
	{
		WARN_TRACE("Fail to sound_manager_get_a2dp_status ret = [%d]", ret);
	}

	if (vconf_get_int(VCONFKEY_SYSMAN_EARJACK, &earjack))
		WARN_TRACE("Earjack state get Fail...");

	_mp_play_view_append_snd_path_device(ad, bt_name, bt_connected, earjack, popup);

}

static void
_mp_play_view_destory_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	playing_view->flick_direction = 0;
	//hide mute popup
	if (playing_view->play_view)
		edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_HIDE, "*");
	mp_ecore_timer_del(ad->volume_down_timer);
	mp_ecore_timer_del(ad->mute_popup_show_timer);
	//reset flag
	ad->volume_long_pressed = false;

	mp_ecore_timer_del(playing_view->favourite_timer);
	mp_ecore_timer_del(playing_view->progressbar_timer);
	mp_ecore_timer_del(playing_view->show_ctrl_timer);

	/* volume */
	mp_evas_object_del(playing_view->volume_widget);
	mp_ecore_timer_del(playing_view->volume_widget_timer);
	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, false);
	mp_volume_key_event_callback_del();

	if(ad->backup_playing_view == playing_view)
	{
		IF_FREE(ad->backup_playing_view);
		ad->backup_layout_play_view = NULL;
	}
	else if(ad->playing_view == playing_view)
	{
		IF_FREE(ad->playing_view);
	}
	else
		ERROR_TRACE("++++++++ check here ++++++++++");

	mp_lyric_view_destroy(ad);

	endfunc;

	return;
}

void
mp_play_view_show_default_info(struct appdata *ad)
{
	MP_CHECK(ad);
	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, false);

	mp_evas_object_del(playing_view->play_info);
	Evas_Object *parent = NULL;
#ifdef MP_SOUND_PLAYER
	parent = sp_view_mgr_get_naviframe(ad->view_mgr);
#else
	parent = ad->naviframe;
#endif
	playing_view->play_info =
		mp_common_load_edj(parent, PLAY_VIEW_EDJ_NAME, "richinfo/test_rich_info");

	mp_play_view_create_info_contents(playing_view->play_info, ad);

#ifdef MP_SOUND_PLAYER
	sp_view_mgr_push_view_content(ad->view_mgr, playing_view->play_info, SP_VIEW_TYPE_INFO);
	sp_view_mgr_set_title_label(ad->view_mgr, GET_STR("IDS_MUSIC_BODY_MEDIA_INFO"));
	sp_view_mgr_set_back_button(ad->view_mgr, mp_play_view_info_back_cb, ad);
#else
	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	mp_view_manager_push_view_content(view_data, playing_view->play_info, MP_VIEW_CONTENT_INFO);
	mp_view_manager_set_title_and_buttons(view_data, "IDS_MUSIC_BODY_MEDIA_INFO",
					      ad);
#endif
}

void
mp_play_view_info_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	if (ad->info_click_flag && !ad->info_ug)
		return ;
	if (!ad->info_ug)
		ad->info_click_flag = TRUE;
	else
		ad->info_click_flag = FALSE;
	ad->info_back_play_view_flag = FALSE;
	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	EVAS_OBJ_HIDE(playing_view->albumart_img);
	//hide mute popup
	if (playing_view->play_view)
		edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_HIDE, "*");
	//show volume button
	if (mp_volume_get_current()>0)
		edje_object_signal_emit(_EDJ(playing_view->play_icon), "unmute", "volume");
	else
		edje_object_signal_emit(_EDJ(playing_view->play_icon), "mute", "volume");
	mp_ecore_timer_del(ad->volume_down_timer);
	mp_ecore_timer_del(ad->mute_popup_show_timer);

	mp_evas_object_del(playing_view->volume_widget);
	mp_ecore_timer_del(playing_view->volume_widget_timer);

#ifndef ENABLE_RICHINFO
	mp_play_view_show_default_info(ad);
#else
	mp_plst_item *cur_item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK(cur_item);

	if(mp_drm_file_right(cur_item->uri) || mp_ug_show_info(ad) < 0)
	{
		DEBUG_TRACE("fail to load ug, Create default Info layout");
		mp_play_view_show_default_info(ad);
	}
#endif

}

static bool
mp_common_refresh_track_info(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	if (mp_volume_get_current() == 0)
	{
		//set mute
		edje_object_signal_emit(_EDJ(playing_view->play_icon), "mute", "volume");
	}
	else
	{
		//reset volume
		edje_object_signal_emit(_EDJ(playing_view->play_icon), "unmute", "volume");
	}

	mp_plst_item *current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK_FALSE(current_item);

	mp_track_info_t *track_info = ad->current_track_info;
	MP_CHECK_FALSE(track_info);

#ifdef MP_SOUND_PLAYER
	edje_object_signal_emit(_EDJ(playing_view->play_menu), "add_to_playlist_invisible", "play_view");
#else
	if(current_item->track_type != MP_TRACK_URI || current_item->uid == NULL )
	{
		edje_object_signal_emit(_EDJ(playing_view->play_menu), "add_to_playlist_invisible", "play_view");
	}
#endif


	if(playing_view->play_head)
	{
		elm_object_item_text_set(playing_view->play_head, track_info->title);
	}
	edje_object_part_text_set(_EDJ(playing_view->play_title), "title", track_info->title);
	edje_object_part_text_set(_EDJ(playing_view->play_title), "artist_name", track_info->artist);

	return true;
}

static Evas_Object *
_mp_play_view_create_next_item(mp_plst_item * next_item, struct appdata *ad)
{
	Evas_Object *next_view = NULL;
	Evas_Object *album_img = NULL;
	mp_track_info_t *track_info = NULL;

	MP_CHECK_NULL(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_NULL(playing_view);

	track_info = ad->current_track_info;
	MP_CHECK_NULL(track_info);

	mp_debug("next item : %s", track_info->title);
	next_view = elm_layout_add(playing_view->layout);

	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{
		mp_debug_temp
			(" invalid state MP_SCREEN_MODE_LANDSCAPE MP_SCREEN_MODE_LANDSCAPE MP_SCREEN_MODE_LANDSCAPE MP_SCREEN_MODE_LANDSCAPE");
		elm_layout_file_set(next_view, PLAY_VIEW_EDJ_NAME, "mp_play_view_landscape");
		evas_object_size_hint_max_set(next_view, 480 * elm_config_scale_get(), 480 * elm_config_scale_get());
		evas_object_size_hint_align_set(next_view, EVAS_HINT_FILL, EVAS_HINT_EXPAND);
		evas_object_size_hint_weight_set(next_view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	}
	else
	{

		next_view = mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "mp_play_view");
		evas_object_size_hint_max_set(next_view, 480 * elm_config_scale_get(), 800 * elm_config_scale_get());
	}

	evas_object_show(next_view);

	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{
		album_img = elm_bg_add(next_view);
		if (mp_util_is_image_valid(ad->evas, track_info->thumbnail_path))
		{
			elm_bg_file_set(album_img, track_info->thumbnail_path, NULL);
		}
		else
		{
			elm_bg_file_set(album_img, DEFAULT_THUMBNAIL, NULL);
		}
	}
	else
	{
		album_img = evas_object_image_add(ad->evas);
		int w, h;
		edje_object_part_geometry_get(_EDJ(playing_view->play_view), "album_bg", NULL, NULL, &w, &h);
		mp_debug("album bg size =  [%d * %d]", w, h);
		evas_object_image_load_size_set(album_img, w, h);
		evas_object_image_fill_set(album_img, 0, 0, w, h);
		if (mp_util_is_image_valid(ad->evas, track_info->thumbnail_path))
		{
			evas_object_image_file_set(album_img, track_info->thumbnail_path, NULL);
		}
		else
		{
			evas_object_image_file_set(album_img, DEFAULT_THUMBNAIL, NULL);
		}
	}

	elm_object_part_content_set(next_view, "album_art", album_img);
	evas_object_show(album_img);
	return next_view;
}

static Evas_Object *
_mp_play_view_create_next_bg_item(mp_plst_item * next_item, struct appdata *ad)
{
	Evas_Object *next_view = NULL;
	Evas_Object *album_img = NULL;

	MP_CHECK_NULL(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_NULL(playing_view);

	mp_track_info_t *track_info = ad->current_track_info;
	MP_CHECK_NULL(track_info);

	mp_debug("next item : %s", track_info->title);

	next_view = elm_layout_add(playing_view->layout);

	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{
		mp_debug_temp
			(" invalid state MP_SCREEN_MODE_LANDSCAPE MP_SCREEN_MODE_LANDSCAPE MP_SCREEN_MODE_LANDSCAPE MP_SCREEN_MODE_LANDSCAPE");
		elm_layout_file_set(next_view, PLAY_VIEW_EDJ_NAME, "mp_play_view_landscape");
		evas_object_size_hint_max_set(next_view, 480 * elm_config_scale_get(), 480 * elm_config_scale_get());
		evas_object_size_hint_align_set(next_view, EVAS_HINT_FILL, EVAS_HINT_EXPAND);
		evas_object_size_hint_weight_set(next_view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	}
	else
	{
		next_view = mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "mp_play_view");
		evas_object_size_hint_max_set(next_view, 480 * elm_config_scale_get(), 800 * elm_config_scale_get());
	}

	evas_object_show(next_view);

	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{
		album_img = elm_bg_add(next_view);
		if (mp_util_is_image_valid(ad->evas, track_info->thumbnail_path))
		{
			elm_bg_file_set(album_img, track_info->thumbnail_path, NULL);
		}
		else
		{
			elm_bg_file_set(album_img, DEFAULT_THUMBNAIL, NULL);
		}
	}
	else
	{
		album_img = evas_object_image_add(ad->evas);
		int w, h;
		edje_object_part_geometry_get(_EDJ(playing_view->play_view_bg), "album_bg", NULL, NULL, &w, &h);
		mp_debug("album bg size =  [%d * %d]", w, h);
		evas_object_image_load_size_set(album_img, w, h);
		evas_object_image_fill_set(album_img, 0, 0, w, h);

		playing_view->mode = rand()%MP_PLAYING_VIEW_BOTTOM_RIGHT+1;

		if (mp_util_is_image_valid(ad->evas, track_info->thumbnail_path))
		{
			mp_util_edit_image(ad->evas, album_img, track_info->thumbnail_path, playing_view->mode);
		}
		else
		{
			mp_util_edit_image(ad->evas, album_img, DEFAULT_THUMBNAIL, playing_view->mode);
		}
	}

	elm_object_part_content_set(next_view, "album_art", album_img);
	evas_object_show(album_img);

	return next_view;
}

static void
_mp_play_view_eventbox_clicked_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	DEBUG_TRACE("[%d]", ad->show_optional_menu);

	mp_play_view_set_menu_state(ad, !ad->show_optional_menu, true);

	return;
}

static bool
_mp_play_view_clear_next_bg_view(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	Evas_Object *content = elm_object_part_content_unset(playing_view->layout, "bg_list_content_temp");
	evas_object_del(content);
	playing_view->play_view_bg_next = NULL;

	edje_object_signal_emit(elm_layout_edje_get(playing_view->layout), "set_default", "bg_list_content");

	playing_view->transition_state = false;

	return true;
}

static bool
_mp_play_view_clear_next_view(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	Evas_Object *content = elm_object_part_content_unset(playing_view->layout, "list_content_temp");
	evas_object_del(content);
	playing_view->play_view_next = NULL;

	edje_object_signal_emit(elm_layout_edje_get(playing_view->layout), "set_default", "list_content");

	playing_view->transition_state = false;

	return true;
}

static void
_mp_play_view_flick_event(struct appdata *ad, bool left)
{
	MP_CHECK(ad);

	if (ad->lyric_view != NULL && ad->b_show_lyric)
	{
		DEBUG_TRACE("lyric view is exist");
		return;
	}

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_plst_item * next_item = NULL;
	if(left)
	{
		playing_view->flick_direction = 1;
	 	next_item = mp_playlist_mgr_get_next(ad->playlist_mgr, EINA_TRUE);
	}
	else
	{
		playing_view->flick_direction = 2;
		next_item = mp_playlist_mgr_get_prev(ad->playlist_mgr);
	}
	MP_CHECK(next_item);
	mp_playlist_mgr_set_current(ad->playlist_mgr, next_item);

	if(mp_playlist_mgr_count(ad->playlist_mgr) == 1)
	{
		DEBUG_TRACE("There is only one playing item. skip transition effect");
		ad->freeze_indicator_icon = true;
		mp_play_destory(ad);
		mp_play_new_file(ad, true);
		return ;
	}

	_mp_play_view_transit_by_item(ad, next_item, left);
}

static void
_mp_play_view_eventbox_flick_left_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	_mp_play_view_flick_event(data, true);
}

static void
_mp_play_view_eventbox_flick_right_cb(void *data, Evas_Object * obj, void *event_info)
{
	startfunc;
	_mp_play_view_flick_event(data, false);
}

static void
_mp_play_view_add_to_playlist_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	DEBUG_TRACE("%s", emission);
#ifndef MP_SOUND_PLAYER
	Evas_Object *add_to_playlist = edje_object_part_object_get(_EDJ(playing_view->play_menu), "add_to_playlist");
	MP_CHECK(add_to_playlist);
	mp_menu_add_to_playlist_cb(ad, add_to_playlist, NULL);
#endif
	return;
}

static void
_mp_play_view_progressbar_down_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	if (data == NULL && obj == NULL && event_info == NULL)
		return;

	evas_object_data_set(obj, "pressed", (void *)1);

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	Evas_Event_Mouse_Down *ev = event_info;
	Evas_Object *progressbar = obj;
	int duration = 0, w = 0, current = 0, x = 0;
	double ratio = 0.0;

	if (ad->player_state == PLAY_STATE_NONE)
	{
		ERROR_TRACE("Invaild player_state : %d", ad->player_state);
		return;
	}

	mp_play_view_progress_timer_freeze(ad);
	evas_object_geometry_get(progressbar, &x, NULL, &w, NULL);
	current = ev->canvas.x - x;

	if (current < 0)
		current = 0;
	else if (current > w)
		current = w;

	ratio = (double)current / w;
	duration = mp_player_mgr_get_duration();

	ad->music_length = duration / 1000.;

	ad->music_pos = ratio * ad->music_length;
	mp_play_view_update_progressbar(ad);

	return;
}

static void
_mp_play_view_progressbar_seek_done_cb(void *data)
{
	startfunc;
	struct appdata *ad = data;

	mp_play_view_progress_timer_thaw(ad);
}

static void
_mp_play_view_progressbar_up_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	startfunc;
	if (data == NULL && obj == NULL && event_info == NULL)
		return;

	evas_object_data_set(obj, "pressed", (void *)0);

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	Evas_Event_Mouse_Up *ev = event_info;
	Evas_Object *progressbar = obj;
	int duration = 0, w = 0, current = 0, x = 0;
	double ratio = 0.0;

	if (ad->player_state == PLAY_STATE_NONE)
	{
		ERROR_TRACE("Invaild player_state : %d", ad->player_state);
		return;
	}

	evas_object_geometry_get(progressbar, &x, NULL, &w, NULL);

	current = ev->canvas.x - x;

	if (current < 0)
		current = 0;
	else if (current > w)
		current = w;

	ratio = (double)current / w;

	duration = mp_player_mgr_get_duration();

	ad->music_length = duration / 1000.;

	ad->music_pos = ratio * ad->music_length;

	if (mp_player_mgr_set_position(ad->music_pos * 1000, _mp_play_view_progressbar_seek_done_cb, ad))
		mp_play_view_update_progressbar(ad);
	else
		_mp_play_view_progressbar_seek_done_cb(ad);


	return;
}

static void
_mp_play_progressbar_move_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	if (data == NULL && obj == NULL && event_info == NULL)
		return;

	int pressed = (int)evas_object_data_get(obj, "pressed");
	if (!pressed) {
		mp_debug("-_- progressbar is not pressed yet!");
		return;
	}

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	Evas_Event_Mouse_Move *ev = event_info;

	if (ad->player_state == PLAY_STATE_NONE)
	{
		ERROR_TRACE("Invaild player_state : %d", ad->player_state);
		return;
	}

	Evas_Object *progressbar = obj;
	int w = 0, current = 0;
	int x;
	double ratio = 0.0;
	int new_pos;

	evas_object_geometry_get(progressbar, &x, NULL, &w, NULL);

	current = ev->cur.canvas.x - x;

	if (current < 0)
		current = 0;
	else if (current > w)
		current = w;

	ratio = (double)current / w;

	new_pos= ratio * ad->music_length;
	ad->music_pos = new_pos;
	mp_play_view_update_progressbar(ad);

	return;
}


static Eina_Bool
_mp_play_view_update_progressbar_cb(void *data)
{
	struct appdata *ad = data;
	MP_CHECK_CANCEL(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_CANCEL(playing_view);

	if(ad->is_lcd_off)
	{
		WARN_TRACE("Check here.. progress timer should be freezed..");
		ecore_timer_freeze(playing_view->progressbar_timer);
	}

	int duration = 0, pos = 0;
	double get_length = 0., get_pos = 0.;

	if (ad->player_state == PLAY_STATE_PLAYING || ad->player_state == PLAY_STATE_PAUSED)
	{
		duration = mp_player_mgr_get_duration();
		if (duration <= 0) {
			mp_track_info_t *track_info = ad->current_track_info;
			duration = track_info->duration;
		}

		if ((duration / 1000) > 0)
		{
			pos = mp_player_mgr_get_position();
			get_length = duration / 1000.;
			get_pos = pos / 1000.;
		}

		if (get_length != ad->music_length || get_pos != ad->music_pos)
		{
			// update only value is changed
			ad->music_length = get_length;
			ad->music_pos = get_pos;

			mp_play_view_update_progressbar(ad);
		}
	}

	ecore_timer_interval_set(playing_view->progressbar_timer, 0.5);

	if (ad->b_show_lyric)
	{
	    mp_lyric_view_refresh(ad);
	}

	return 1;
}

static void
_mp_play_view_menu_visible_set(void *data, bool menu_enable, bool animation)
{
	struct appdata *ad = data;

	mp_retm_if(ad == NULL, "appdata is NULL");
	mp_retm_if(ad->playing_view == NULL, "playing_view is NULL");
	mp_retm_if(ad->playing_view->play_view == NULL, "play_view is NULL");

	if (menu_enable)
		edje_object_signal_emit(_EDJ(ad->playing_view->layout), "option_menu_visible", "player_option_menu");
	else
	{
		if(animation)
			edje_object_signal_emit(_EDJ(ad->playing_view->layout), "option_menu_invisible",
					"player_option_menu");
		else
			edje_object_signal_emit(_EDJ(ad->playing_view->layout), "option_hide",
					"player_option_menu");
	}
}

static void
_mp_play_view_progress_visible_set(void *data, bool progressbar_enable)
{
	struct appdata *ad = data;

	mp_retm_if(ad == NULL, "appdata is NULL");
	mp_retm_if(ad->playing_view == NULL, "playing_view is NULL");
	mp_retm_if(ad->playing_view->play_view == NULL, "play_view is NULL");

	if (progressbar_enable)
		edje_object_signal_emit(_EDJ(ad->playing_view->play_ctrl), "progressbar_visible", "player_progress");
	else
		edje_object_signal_emit(_EDJ(ad->playing_view->play_ctrl), "progressbar_invisible", "player_progress");
}


void
_mp_play_view_add_progress_timer(void *data)
{
	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_ecore_timer_del(playing_view->progressbar_timer);

	playing_view->progressbar_timer = ecore_timer_add(0.1, _mp_play_view_update_progressbar_cb, ad);
	if(ad->player_state != PLAY_STATE_PLAYING || ad->is_lcd_off)
		ecore_timer_freeze(playing_view->progressbar_timer);

	return;
}

static bool
_mp_play_view_init_progress_bar(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);
DEBUG_TRACE("%s:+++\n", __func__); //Minjin
	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	int pos = 0, duration = 0;
	pos = mp_player_mgr_get_position();
	duration = mp_player_mgr_get_duration();
	ad->music_pos = pos / 1000.;
	ad->music_length = duration / 1000.;
	mp_play_view_update_progressbar(ad);
	_mp_play_view_add_progress_timer(ad);

DEBUG_TRACE("%s:---:pos=%d, duration=%d\n ", __func__, pos, duration); //Minjin
	return true;
}

bool
_mp_play_view_set_menu_item(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_play_control_shuffle_set(ad, mp_playlist_mgr_is_shuffle(ad->playlist_mgr));
	mp_play_control_repeat_set(ad, mp_playlist_mgr_get_repeat(ad->playlist_mgr));

	return true;
}


static bool
_mp_play_view_create_menu(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

#ifdef MP_SOUND_PLAYER
	if (mp_playlist_mgr_count(ad->playlist_mgr) <= 1) {
		mp_debug("sound player only 1 file.. do not show menu");
		return true;
	}
#endif

	playing_view->play_menu = mp_common_load_edj(playing_view->play_view, PLAY_VIEW_EDJ_NAME, "mp_play_menu");
	elm_object_part_content_set(playing_view->layout, "player_option_menu", playing_view->play_menu);
	edje_object_signal_callback_add(_EDJ(playing_view->play_menu), "add_to_playlist_clicked", "*", _mp_play_view_add_to_playlist_cb,
					ad);

	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
		evas_object_size_hint_max_set(playing_view->play_menu, 480 * elm_config_scale_get(), 70 * elm_config_scale_get());

	edje_object_signal_callback_add(_EDJ(playing_view->play_menu), SIGNAL_SHUFFLE, "*", mp_play_control_menu_cb,
					ad);
	edje_object_signal_callback_add(_EDJ(playing_view->play_menu), SIGNAL_SHUFNON, "*", mp_play_control_menu_cb,
					ad);
	edje_object_signal_callback_add(_EDJ(playing_view->play_menu), SIGNAL_REPALL, "*", mp_play_control_menu_cb, ad);
	edje_object_signal_callback_add(_EDJ(playing_view->play_menu), SIGNAL_REPNON, "*", mp_play_control_menu_cb, ad);
	edje_object_signal_callback_add(_EDJ(playing_view->play_menu), SIGNAL_REP1, "*", mp_play_control_menu_cb, ad);

	_mp_play_view_set_menu_item(ad);
	_mp_play_view_menu_visible_set(ad, FALSE, false);
	endfunc;
	return true;

}

void
mp_play_view_set_snd_path_sensitivity(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	bool bt_connected = 0;
	bool earjack = 0;

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_evas_object_del(playing_view->snd_button);

	bt_connected = sound_manager_is_route_available(SOUND_ROUTE_OUT_BLUETOOTH);
	earjack = sound_manager_is_route_available(SOUND_ROUTE_OUT_WIRED_ACCESSORY);

	if (bt_connected || earjack) {
		playing_view->snd_button = mp_widget_create_title_btn(playing_view->play_title, NULL, MP_ICON_SOUND_PATH,
									mp_play_view_set_snd_path_cb, ad);
	}
	elm_object_part_content_set(playing_view->play_title, "sound_path", playing_view->snd_button);
}

static void
mp_play_view_back_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
#ifdef MP_SOUND_PLAYER
	struct appdata *ad = (struct appdata *)data;
#else
	view_data_t *view_data = (view_data_t *) data;
	MP_CHECK(view_data);
	MP_CHECK_VIEW_DATA(view_data);

	struct appdata *ad = view_data->ad;
#endif
	MP_CHECK(ad);
	mp_retm_if(ad->navi_effect_in_progress, "navi effect in progress");

#ifdef MP_SOUND_PLAYER
#ifdef MP_FEATURE_EXIT_ON_BACK
	if(ad->caller_win_id)
	{
		mp_app_exit(ad);
	}
	else
#endif
	if (ad->player_state == PLAY_STATE_PLAYING || ad->player_state == PLAY_STATE_PAUSED) {
		mp_debug("mm player alive");
		elm_win_lower(ad->win_main);
	} else {
		mp_debug("terminate");
		mp_app_exit(ad);
	}
#else
	if(ad->direct_win_minimize)
		elm_win_lower(ad->win_main);
	else
		mp_common_back_button_cb(data, obj, NULL);

	if (ad->music_setting_change_flag)
	{
		mp_debug("# menu update #");
		mp_util_set_library_controlbar_items(ad);
		ad->music_setting_change_flag = false;
	}
#endif
}

static void
_mp_play_view_play_pause_button_cb(void *data, Evas_Object * o, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	DEBUG_TRACE("[%s], ad->player_state: %d", emission, ad->player_state);
	if (!strcmp(emission, SIGNAL_PLAY))
		mp_play_control_play_pause(ad, true);
	else
		mp_play_control_play_pause(ad, false);
}

static void
_mp_play_view_volume_change_cb(int volume, void *user_data)
{
	DEBUG_TRACE("volume level: %d", volume);
	struct appdata *ad = (struct appdata *)user_data;
	MP_CHECK(ad);
	MP_CHECK(ad->playing_view);

	if (volume>0)
		edje_object_signal_emit(_EDJ(ad->playing_view->play_icon), "unmute", "volume");
	else
		edje_object_signal_emit(_EDJ(ad->playing_view->play_icon), "mute", "volume");

}

static bool
_mp_play_view_create_control_bar(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	Evas_Object *play_control = NULL, *play_icon = NULL;

	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
		play_control =
			mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "music/playing/main/control_ls");
	else
		play_control =
			mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "music/playing/main/control");

	MP_CHECK_FALSE(play_control);

	play_icon = mp_common_load_edj(play_control, PLAY_VIEW_EDJ_NAME, "music/playing/main/control/buttons");

	if (!play_icon)
	{
		mp_evas_object_del(play_control);
		MP_CHECK_FALSE(play_icon);
	}

	elm_object_part_content_set(play_control, "buttons", play_icon);
	evas_object_data_set(play_control, "buttons", play_icon);
#ifdef MP_SOUND_PLAYER
		edje_object_signal_callback_add(_EDJ(play_icon), "info_clicked", CTR_EDJ_SIG_SRC, mp_play_view_back_cb, ad);
#else
		edje_object_signal_callback_add(_EDJ(play_icon), "info_clicked", CTR_EDJ_SIG_SRC, mp_play_view_back_cb,
					mp_util_get_view_data(ad));
#endif
	edje_object_signal_callback_add(_EDJ(play_icon), "volume_clicked", CTR_EDJ_SIG_SRC, mp_play_control_volume_cb,
					ad);
#ifdef MP_FEATURE_VOLMUE_MUTE
	edje_object_signal_callback_add(_EDJ(play_icon), "vol_btn_down", CTR_EDJ_SIG_SRC,
					mp_play_control_volume_down_cb, ad);
	edje_object_signal_callback_add(_EDJ(play_icon), "vol_btn_up", CTR_EDJ_SIG_SRC, mp_play_control_volume_up_cb,
					ad);
#endif
	edje_object_signal_callback_add(_EDJ(play_icon), "play_clicked", CTR_EDJ_SIG_SRC, _mp_play_view_play_pause_button_cb, ad);
	edje_object_signal_callback_add(_EDJ(play_icon), "pause_clicked", CTR_EDJ_SIG_SRC, _mp_play_view_play_pause_button_cb, ad);

	edje_object_signal_callback_add(_EDJ(play_icon), "ff_btn_down", CTR_EDJ_SIG_SRC, mp_play_control_ff_cb, ad);
	edje_object_signal_callback_add(_EDJ(play_icon), "ff_btn_up", CTR_EDJ_SIG_SRC, mp_play_control_ff_cb, ad);

	edje_object_signal_callback_add(_EDJ(play_icon), "rew_btn_down", CTR_EDJ_SIG_SRC, mp_play_control_rew_cb, ad);
	edje_object_signal_callback_add(_EDJ(play_icon), "rew_btn_up", CTR_EDJ_SIG_SRC, mp_play_control_rew_cb, ad);

	player_state_e player_state = mp_player_mgr_get_state();
	if (player_state != PLAYER_STATE_PLAYING)
		edje_object_signal_emit(_EDJ(play_icon), "play", CTR_PROG_SIG_SRC);
	else
		edje_object_signal_emit(_EDJ(play_icon), "pause", CTR_PROG_SIG_SRC);

	playing_view->play_control = play_control;

	//show volume button
	if (mp_volume_get_current()>0)
		edje_object_signal_emit(_EDJ(play_icon), "unmute", "volume");
	else
		edje_object_signal_emit(_EDJ(play_icon), "mute", "volume");
	playing_view->play_icon = play_icon;

	mp_volume_add_change_cb(_mp_play_view_volume_change_cb, ad);

	elm_object_part_content_set(playing_view->layout, "elm.swallow.controlbar", playing_view->play_control);
	endfunc;
	return true;
}



static void
_mp_play_progress_val_set(void *data, double position)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);
	MP_CHECK(playing_view->play_progressbar);

	Evas_Object *pbar = evas_object_data_get(playing_view->play_progressbar, "progress_bar");

	edje_object_part_drag_value_set(_EDJ(pbar), "progressbar_control", position, 0.0);

	return;
}


static bool
_mp_play_view_show_parts(void *data, bool show_flag)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);
	MP_CHECK_FALSE(playing_view->layout);

	if (show_flag) {
		edje_object_signal_emit(_EDJ(playing_view->layout), "set_default", "mp_play_view_layout");
	} else {
		edje_object_signal_emit(_EDJ(playing_view->layout), "set_hide", "mp_play_view_layout");
	}
	return true;
}

static void
_mp_play_view_transit_by_item_complete_cb(void *data, Evas_Object * obj, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	DEBUG_TRACE("emission: %s, source: %s", emission, source);
	if (source)
	{
		if (strcmp(source, "list") == 0)
		{
			mp_plst_item *cur = mp_playlist_mgr_get_current(ad->playlist_mgr);
			MP_CHECK(cur);
			ad->freeze_indicator_icon = true;
			mp_play_item_play(ad, cur->uid);
			mp_play_view_refresh(ad);

			_mp_play_view_clear_next_view(data);
			_mp_play_view_clear_next_bg_view(data);
			_mp_play_view_init_progress_bar(data);
			evas_object_show(playing_view->layout);

			edje_object_signal_callback_del(elm_layout_edje_get(playing_view->layout), "transit_done", "*",
							_mp_play_view_transit_by_item_complete_cb);
		}
	}

	return;
}

static bool
_mp_play_view_transit_by_item(struct appdata *ad, mp_plst_item *it, bool move_left)
{
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	if (playing_view->play_view)
	{
		mp_ecore_timer_del(ad->mute_popup_show_timer);
		//hide mute popup
		edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_HIDE, "*");
		//show volume button
		if (mp_volume_get_current()>0)
			edje_object_signal_emit(_EDJ(playing_view->play_icon), "unmute", "volume");
		else
			edje_object_signal_emit(_EDJ(playing_view->play_icon), "mute", "volume");
	}

	Evas_Object *next = NULL;
	if (!g_strcmp0(ad->latest_playing_key_id ,it->uid))
	{
		mp_debug("same file selected plz check state");
		return FALSE;
	}

	if (playing_view->transition_state)	// transiton(transition_state)  should be transiit callback
	{
		mp_debug("skip_by transiton effect");
		return FALSE;
	}

	mp_play_view_stop_transit(ad);

	mp_play_view_progress_timer_freeze(ad);

	mp_play_view_set_menu_state(ad, ad->b_show_lyric, false);

	next = _mp_play_view_create_next_item(it, ad);
	Evas_Object *bg_next = NULL;
	bg_next = _mp_play_view_create_next_bg_item(it, ad);

	MP_CHECK_FALSE(next);

	MP_CHECK_FALSE(bg_next);
	/* hide menu, controlbar, progressbar, options and title */
	_mp_play_view_show_parts(ad, false);
	playing_view->play_view_bg_next = bg_next;

	playing_view->play_view_next = next;
	elm_object_part_content_set(playing_view->layout, "list_content_temp", next);
	elm_object_part_content_set(playing_view->layout, "bg_list_content_temp", bg_next);
	edje_object_signal_callback_add(elm_layout_edje_get(playing_view->layout), "transit_done", "*",
					_mp_play_view_transit_by_item_complete_cb, ad);

	if (!move_left)
	{
		edje_object_signal_emit(elm_layout_edje_get(playing_view->layout), "set_left", "list_content_temp");
		edje_object_signal_emit(elm_layout_edje_get(playing_view->layout), "flick_right", "list_content");
	}
	else
	{
		edje_object_signal_emit(elm_layout_edje_get(playing_view->layout), "set_right", "list_content_temp");
		edje_object_signal_emit(elm_layout_edje_get(playing_view->layout), "flick_left", "list_content");
	}

	edje_object_signal_emit(elm_layout_edje_get(playing_view->layout), "set_default", "list_content_temp");

	playing_view->transition_state = true;
	/* set next song title and artist */
	mp_track_info_t *track_info = ad->current_track_info;
	edje_object_part_text_set(_EDJ(playing_view->play_title), "title", track_info->title);
	edje_object_part_text_set(_EDJ(playing_view->play_title), "artist_name", track_info->artist);

	return true;
}


static bool
_mp_play_view_push_navibar(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);
	{
		elm_object_part_content_set(playing_view->layout, "list_content", playing_view->play_view);
		elm_object_part_content_set(playing_view->layout, "bg_list_content", playing_view->play_view_bg);
		mp_playing_view *playing_view = ad->playing_view;
		MP_CHECK_FALSE(playing_view);

#ifdef MP_SOUND_PLAYER
		sp_view_mgr_push_view_content(ad->view_mgr, playing_view->layout, SP_VIEW_TYPE_PLAY);
		mp_play_view_set_snd_path_sensitivity(ad);
#else
		MP_CHECK_FALSE(ad->naviframe);

		view_data_t *view_data = NULL;
		view_data = evas_object_data_get(ad->naviframe, "view_data");
		MP_CHECK_FALSE(view_data);
		mp_view_manager_push_view_content(view_data, playing_view->layout, MP_VIEW_CONTENT_PLAY);
		mp_view_manager_set_title_and_buttons(view_data, NULL, ad);
#endif
	}

	return true;

}

static void
_mp_play_view_play_option_cb(void *data, Evas_Object * obj, const char *emission, const char *source)
{
	MP_CHECK(emission);

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	if (!strcmp(emission, "share_clicked"))
	{
		MP_CHECK(ad->playing_view);
		Evas_Object *share = edje_object_part_object_get(_EDJ(ad->playing_view->play_options), "share");
		MP_CHECK(share);
		mp_menu_share_cb(data, share, NULL);
	}
	else if (!strcmp(emission, "set_clicked"))
	{
		MP_CHECK(ad->playing_view);
		Evas_Object *set = edje_object_part_object_get(_EDJ(ad->playing_view->play_options), "set");
		MP_CHECK(set);
		mp_menu_set_cb(data, set, NULL);
	}
	else if (!strcmp(emission, "details_clicked"))	{
		MP_CHECK(ad->playing_view);
		if (ad->playing_view->play_view_screen_mode != MP_SCREEN_MODE_LANDSCAPE)
			mp_play_view_info_cb(data, obj, NULL);
	}
}

static Evas_Object *
_mp_play_view_create_ctrl_layout(void *data, Evas_Object * parents, Evas_Object ** progress)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	Evas_Object *ctrl_layout = NULL;

	ctrl_layout = mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "music/playing/play_ctrl");

	elm_object_part_content_set(parents, "player_info", ctrl_layout);

	//2 Create progress bar
	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
		*progress =
			mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "music/playing/progress_box_ls");
	else
		*progress = mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "music/playing/progress_box");

	Evas_Object *play_progress =
		mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "music/playing/progress_box/progress_bar");
	elm_object_part_content_set(*progress, "progress_bar", play_progress);
	evas_object_data_set(*progress, "progress_bar", play_progress);

	evas_object_event_callback_add(play_progress, EVAS_CALLBACK_MOUSE_DOWN,
			       _mp_play_view_progressbar_down_cb, ad);
	evas_object_event_callback_add(play_progress, EVAS_CALLBACK_MOUSE_UP, _mp_play_view_progressbar_up_cb,
			       ad);
	evas_object_event_callback_add(play_progress, EVAS_CALLBACK_MOUSE_MOVE, _mp_play_progressbar_move_cb,
			       ad);

	elm_object_part_content_set(playing_view->layout, "player_progress", *progress);

	Evas_Object *play_info = mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "player_view_info");
	elm_object_part_content_set(playing_view->layout, "player_info", play_info);
	playing_view->play_title = play_info;

	if (playing_view->play_view_screen_mode != MP_SCREEN_MODE_LANDSCAPE)
	{
		Evas_Object *play_options =
			mp_common_load_edj(playing_view->layout, PLAY_VIEW_EDJ_NAME, "player_view_options");
		playing_view->play_options = play_options;
		edje_object_part_text_set(_EDJ(play_options), "option_1", GET_SYS_STR("IDS_COM_BUTTON_SHARE"));
		mp_language_mgr_register_object(play_options, OBJ_TYPE_EDJE_OBJECT, "option_1", "IDS_COM_BUTTON_SHARE");
		edje_object_part_text_set(_EDJ(play_options), "option_2", GET_SYS_STR("IDS_COM_SK_SET"));
		mp_language_mgr_register_object(play_options, OBJ_TYPE_EDJE_OBJECT, "option_2", "IDS_COM_SK_SET");
		edje_object_part_text_set(_EDJ(play_options), "option_3", GET_SYS_STR("IDS_COM_BODY_DETAILS"));
		mp_language_mgr_register_object(play_options, OBJ_TYPE_EDJE_OBJECT, "option_3", "IDS_COM_BODY_DETAILS");

		elm_object_part_content_set(playing_view->layout, "player_options", play_options);
		edje_object_signal_callback_add(_EDJ(play_options), "share_clicked", "*", _mp_play_view_play_option_cb,
						ad);
		edje_object_signal_callback_add(_EDJ(play_options), "set_clicked", "*", _mp_play_view_play_option_cb,
						ad);
		edje_object_signal_callback_add(_EDJ(play_options), "details_clicked", "*", _mp_play_view_play_option_cb,
						ad);
	}

	return ctrl_layout;
}


static void
_mp_play_view_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	Evas_Coord w, h;
	edje_object_part_geometry_get(_EDJ(playing_view->play_view), "album_bg", NULL, NULL, &w, &h);
	mp_debug("album bg size =  [%d * %d]", w, h);

	if (playing_view->albumart_bg)
	{
		evas_object_image_load_size_set(playing_view->albumart_bg, w, h);
		evas_object_image_fill_set(playing_view->albumart_bg, 0, 0, w, h);
	}
}

static void
_mp_play_view_bg_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	Evas_Coord w, h;
	edje_object_part_geometry_get(_EDJ(playing_view->play_view_bg), "album_bg", NULL, NULL, &w, &h);
	mp_debug("album bg size =  [%d * %d]", w, h);

	if (playing_view->bg_albumart_bg)
	{
		evas_object_image_load_size_set(playing_view->bg_albumart_bg, w, h);
		evas_object_image_fill_set(playing_view->bg_albumart_bg, 0, 0, w, h);
	}
}

static bool
_mp_play_view_create_playing_layout(void *data, Evas_Object * parents_layout, bool b_next)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	//2 Create playing view
	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
		playing_view->play_view =
			mp_common_load_edj(parents_layout, PLAY_VIEW_EDJ_NAME, "mp_play_view_landscape");
	else {
		playing_view->play_view_bg = mp_common_load_edj(parents_layout, PLAY_VIEW_EDJ_NAME, "mp_play_view_bg");
		playing_view->play_view = mp_common_load_edj(parents_layout, PLAY_VIEW_EDJ_NAME, "mp_play_view");
	}
	MP_CHECK_FALSE(playing_view->play_view);

	evas_object_event_callback_add(playing_view->play_view, EVAS_CALLBACK_RESIZE, _mp_play_view_resize_cb, ad);
	evas_object_event_callback_add(playing_view->play_view_bg, EVAS_CALLBACK_RESIZE, _mp_play_view_bg_resize_cb, ad);
	//hide mute popup
	edje_object_signal_emit(_EDJ(playing_view->play_view), SIGNAL_MAIN_MUTE_HIDE, "*");

	Evas_Object *event_box = mp_smart_event_box_add(playing_view->play_view);

	evas_object_size_hint_weight_set(event_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(event_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(event_box, "mouse.clicked", _mp_play_view_eventbox_clicked_cb, ad);
	evas_object_smart_callback_add(event_box, "mouse.flick.left", _mp_play_view_eventbox_flick_left_cb, ad);
	evas_object_smart_callback_add(event_box, "mouse.flick.right", _mp_play_view_eventbox_flick_right_cb, ad);

	evas_object_show(event_box);
	elm_object_part_content_set(playing_view->play_view, "event_box", event_box);
	evas_object_data_set(playing_view->play_view, "event_box", event_box);

	if (playing_view->play_view_screen_mode == MP_SCREEN_MODE_LANDSCAPE)
	{			// landscape mode
		playing_view->albumart_img = elm_bg_add(playing_view->play_view);
		elm_bg_load_size_set(playing_view->albumart_img, MP_PLAY_VIEW_ARTWORK_SIZE, MP_PLAY_VIEW_ARTWORK_SIZE);
		elm_object_part_content_set(playing_view->play_view, "album_art", playing_view->albumart_img);
	}
	else
	{
		mp_evas_object_del(playing_view->albumart_bg);
		playing_view->albumart_bg = evas_object_image_add(ad->evas);
		elm_object_part_content_set(playing_view->play_view, "album_bg", playing_view->albumart_bg);

		mp_evas_object_del(playing_view->bg_albumart_bg);
		playing_view->bg_albumart_bg = evas_object_image_add(ad->evas);
		elm_object_part_content_set(playing_view->play_view_bg, "album_bg", playing_view->bg_albumart_bg);
	}

	playing_view->play_ctrl =
		_mp_play_view_create_ctrl_layout(ad, playing_view->play_view, &playing_view->play_progressbar);

	_mp_play_view_progress_visible_set(ad, TRUE);
	_mp_play_progress_val_set(ad, 0.0);

	evas_object_show(playing_view->play_progressbar);

	endfunc;

	return TRUE;
}

static bool
_mp_play_view_create_layout(void *data)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	//2 Create main layout

	if (playing_view->layout)
	{
		mp_debug("already created skip create layout");
		return TRUE;
	}

	{
		Evas_Object *parent = NULL;
#ifdef MP_SOUND_PLAYER
		parent = sp_view_mgr_get_naviframe(ad->view_mgr);
#else
		MP_CHECK_FALSE(ad->naviframe);
		parent = ad->naviframe;
#endif
		playing_view->layout =
			mp_common_load_edj(parent, PLAY_VIEW_EDJ_NAME, "mp_play_view_layout");
		MP_CHECK_FALSE(playing_view->layout);
	}

	MP_CHECK_FALSE(playing_view->layout);
	evas_object_event_callback_add(playing_view->layout, EVAS_CALLBACK_DEL, _mp_play_view_destory_cb, ad);
	//2 Create playing view
	_mp_play_view_create_playing_layout(ad, playing_view->layout, FALSE);

	//2 Create menu layout
	_mp_play_view_create_menu(ad);

	//2 Create control bar
	_mp_play_view_create_control_bar(ad);

	//2 Push at navibar
	_mp_play_view_push_navibar(ad);

	mp_debug("");

	endfunc;

	return true;
}
void
_mp_play_view_delete_progress_timer(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_ecore_timer_del(playing_view->progressbar_timer);
}

bool
mp_play_view_set_menu_state(void *data, bool show, bool anim)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	ad->show_optional_menu = show;

	if (!show)
	{
		mp_debug("set hide state");

		mp_ecore_timer_del(ad->longpress_timer);

		_mp_play_view_menu_visible_set(ad, FALSE, anim);

                ad->b_show_lyric = false;
		mp_lyric_view_hide(ad);
	}
	else
	{
		mp_debug("set show state");

		_mp_play_view_menu_visible_set(ad, TRUE, anim);
		ad->b_show_lyric = true;
		mp_lyric_view_show(ad);
	}

	mp_play_control_repeat_set(ad, mp_playlist_mgr_get_repeat(ad->playlist_mgr));

	return true;

}

void
mp_play_view_progress_timer_thaw(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);
	MP_CHECK(playing_view->progressbar_timer);

	if (ad->player_state == PLAY_STATE_PLAYING)
	{
		ecore_timer_thaw(playing_view->progressbar_timer);
	}
	else if (ad->player_state == PLAY_STATE_PAUSED)
	{
		mp_play_view_update_progressbar(ad);
	}
}

void
mp_play_view_progress_timer_freeze(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	if (playing_view->progressbar_timer)
	{
		ecore_timer_freeze(playing_view->progressbar_timer);
	}
}

void
mp_play_view_update_progressbar(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	char play_time[16] = { 0, };
	char total_time[16] = { 0, };

DEBUG_TRACE("%s:+++\n", __func__); //Minjin

	if (ad->music_length > 0.)
	{
		if (ad->music_length > 3600.)
		{
			snprintf(total_time, sizeof(total_time), "%" MUSIC_TIME_FORMAT,
				 MUSIC_TIME_ARGS(ad->music_length + 0.5));
			snprintf(play_time, sizeof(play_time), "%" MUSIC_TIME_FORMAT, MUSIC_TIME_ARGS(ad->music_pos));
		}
		else
		{
			snprintf(total_time, sizeof(total_time), "%" PLAY_TIME_FORMAT,
				 PLAY_TIME_ARGS(ad->music_length + 0.5));
			snprintf(play_time, sizeof(play_time), "%" PLAY_TIME_FORMAT, PLAY_TIME_ARGS(ad->music_pos));
		}
	}
	else
	{
		if(ad->current_track_info)
			snprintf(total_time, sizeof(total_time), "%" PLAY_TIME_FORMAT,
					 PLAY_TIME_ARGS(ad->current_track_info->duration/1000. + 0.5));
		snprintf(play_time, sizeof(play_time), "%" PLAY_TIME_FORMAT, PLAY_TIME_ARGS(ad->music_pos));
	}

	double played_ratio = 0.;
	if(ad->music_length > 0. && ad->music_pos > 0.)
		played_ratio = ad->music_pos / ad->music_length;
	_mp_play_progress_val_set(ad, played_ratio);

	edje_object_part_text_set(_EDJ(playing_view->play_progressbar), "progress_text_total", total_time);
	edje_object_part_text_set(_EDJ(playing_view->play_progressbar), "progress_text_playing", play_time);

	_mp_play_view_progress_visible_set(ad, TRUE);

DEBUG_TRACE("%s:---:played_ratio=%d, ad->music_pos=%d, ad->music_length=%d\n", __func__, played_ratio, ad->music_pos, ad->music_length); //Minjin

	return;

}

bool
mp_play_view_stop_transit(struct appdata *ad)
{
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	if (playing_view->play_view_next)
	{
		mp_play_view_refresh(ad);
		_mp_play_view_clear_next_view(ad);

		if (playing_view->play_view_bg_next)
			_mp_play_view_clear_next_bg_view(ad);
		_mp_play_view_init_progress_bar(ad);
		edje_object_signal_callback_del(elm_layout_edje_get(playing_view->layout), "transit_done", "*",
						_mp_play_view_transit_by_item_complete_cb);
	}

	return true;
}

static void
_mp_play_view_update_option_btn_sensitivity(struct appdata *ad)
{
	startfunc;
	MP_CHECK(ad);
	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_plst_item *current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK(current_item);

	const char *filename =current_item->uri;
	if (playing_view->play_view_screen_mode != MP_SCREEN_MODE_LANDSCAPE)
	{
		Evas_Object *play_options = elm_object_part_content_get(playing_view->layout, "player_options");
		MP_CHECK(play_options);

		const char *signal = NULL;
		if (mp_check_file_exist(filename))
			signal = "sig_enable_option_btn";
		else
			signal = "sig_disable_option_btn";

		edje_object_signal_emit(_EDJ(play_options), signal, "c_source");
	}
}

bool
mp_play_view_refresh_view(void *data)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	mp_track_info_t *track_info = ad->current_track_info;
	MP_CHECK_FALSE(track_info);

	DEBUG_TRACE("ad->screen_mode=%d,flick_direction=%d", ad->screen_mode, playing_view->flick_direction);
	if (playing_view->play_view_screen_mode)
	{			// landscape mode
		// get alubmart
		if (mp_util_is_image_valid(ad->evas, track_info->thumbnail_path))
		{
			elm_bg_file_set(playing_view->albumart_img, track_info->thumbnail_path, NULL);
		}
		else
		{
			elm_bg_file_set(playing_view->albumart_img, DEFAULT_THUMBNAIL, NULL);
		}
		evas_object_show(playing_view->albumart_img);
	}
	else
	{			//portrait mode
		if (playing_view->flick_direction || MP_SCREEN_MODE_PORTRAIT == ad->screen_mode)
		{
			// get alubmart
			if (mp_util_is_image_valid(ad->evas, track_info->thumbnail_path))
			{
				mp_util_edit_image(ad->evas, playing_view->bg_albumart_bg, track_info->thumbnail_path, playing_view->mode);
				evas_object_image_file_set(playing_view->albumart_bg, track_info->thumbnail_path, NULL);
			}
			else
			{
				mp_util_edit_image(ad->evas, playing_view->bg_albumart_bg, DEFAULT_THUMBNAIL, playing_view->mode);
				evas_object_image_file_set(playing_view->albumart_bg, DEFAULT_THUMBNAIL, NULL);
			}

			evas_object_show(playing_view->bg_albumart_bg);
			evas_object_show(playing_view->albumart_bg);
			playing_view->flick_direction = 0;
		}
	}

	mp_common_refresh_track_info(data);


	/* disable share/set button when file do not exist */
	_mp_play_view_update_option_btn_sensitivity(ad);

	endfunc;

	return true;
}



#ifndef MP_SOUND_PLAYER
/**
 * load play view by path use for add to home case
 *
 * @param struct appdata *ad, char *path, int fid
 * @return FALSE or TRUE if it success create
 * @author aramie.kim@samsung.com
 */

bool
mp_play_view_load_by_path(struct appdata * ad, char *path)
{
	MP_CHECK_FALSE(ad);
	MP_CHECK_FALSE(path);

	//create play list and set playing request item

	mp_playlist_mgr_clear(ad->playlist_mgr);
	char *uid = mp_util_get_fid_by_full_path(path);
	mp_plst_item *item = mp_playlist_mgr_item_append(ad->playlist_mgr, path, uid, MP_TRACK_URI);
	mp_playlist_mgr_set_current(ad->playlist_mgr, item);
	IF_FREE(uid);

	mp_play_destory(ad);
	ad->paused_by_user = FALSE;

	if(ad->playing_view)
	{
		mp_evas_object_del(ad->playing_view->play_control);
		_mp_play_view_create_control_bar(ad);
		mp_play_view_play_item(ad, item, false, true);
	}
	else
	{
		mp_play_view_load(ad);
		MP_CHECK_FALSE(ad->naviframe);
		evas_object_smart_callback_add(ad->naviframe, "transition,finished",
								       _mp_play_view_start_request, ad);
	}

	return true;
}


static void
_mp_play_view_start_request(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	mp_retm_if(!ad, "ad");

	mp_playing_view *playing_view = ad->playing_view;
	mp_retm_if(!playing_view, "ad");

	startfunc;
	ad->paused_by_user = FALSE;
	mp_play_new_file(data, TRUE);

	_mp_play_view_init_progress_bar(data);

	evas_object_smart_callback_del(ad->naviframe, "transition,finished", _mp_play_view_start_request);

	endfunc;

	return;
}


bool
mp_play_view_load_and_play(struct appdata *ad, char *prev_item_uid, bool effect_value)
{
	MP_CHECK_FALSE(ad);

	bool reqest_play = false;

	mp_plst_item *current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);

	if (!prev_item_uid)
		reqest_play = true;

	MP_CHECK_EXCEP(current_item);


	{
		if (g_strcmp0(prev_item_uid, current_item->uid))
		{
			reqest_play = true;
			mp_play_destory(ad);
		}

		if (effect_value)
		{
			_mp_play_view_transit_by_item(ad, current_item, true);
		}
		else
		{
			mp_play_view_load(ad);
			{
				if (reqest_play)
				{
					evas_object_smart_callback_add(ad->naviframe, "transition,finished",
								       _mp_play_view_start_request, ad);

					Evas_Object *play_icon = elm_object_part_content_get(ad->playing_view->play_control, "buttons");
					edje_object_signal_emit(_EDJ(play_icon), "pause", CTR_PROG_SIG_SRC);
				}
			}
		}
	}

	return true;

	mp_exception:
	return false;

}
#endif

bool
mp_play_view_play_item(void *data, mp_plst_item * it, bool effect_value, bool move_left)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	MP_CHECK_FALSE(it);

	if (effect_value && (mp_playlist_mgr_count(ad->playlist_mgr) > 1))
	{
		if(ad->app_is_foreground)
			_mp_play_view_transit_by_item(ad, it, move_left);
		else
		{
			mp_play_item_play(ad, it->uid);
			mp_play_view_refresh(data);
		}
	}
	else
	{
		mp_play_item_play(data, it->uid );
		mp_play_view_refresh(data);
	}

	return true;
}



bool
mp_play_view_set_screen_mode(void *data, int mode)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	if (mode == MP_SCREEN_MODE_LANDSCAPE)
	{
		mp_playing_view *playing_view = ad->playing_view;

		if (playing_view)
		{
			playing_view->play_view_screen_mode = mode;

			if (playing_view->layout)
			{
				evas_object_hide(playing_view->layout);

				mp_play_view_stop_transit(ad);

				if (playing_view->layout)
				{
					ad->backup_layout_play_view = playing_view->layout;
					playing_view->layout = NULL;
				}

				ad->backup_playing_view = playing_view;
				if(playing_view->progressbar_timer)
					ecore_timer_freeze(playing_view->progressbar_timer);

				ad->playing_view = NULL;
			}
		}
	}
	else
	{
		if (mp_playlist_mgr_get_current(ad->playlist_mgr)
		    && ad->backup_playing_view && ad->backup_layout_play_view)
		{
			ad->playing_view = ad->backup_playing_view;
			ad->playing_view->layout = ad->backup_layout_play_view;
			ad->playing_view->play_view_screen_mode = MP_SCREEN_MODE_PORTRAIT;

			mp_play_view_stop_transit(ad);	//reset transition effect

			_mp_play_view_set_menu_item(ad);	//reset menu item

			mp_play_view_refresh(ad);

			evas_object_show(ad->playing_view->layout);
			evas_object_show(ad->conformant);

			ad->backup_playing_view = NULL;
			ad->backup_layout_play_view = NULL;

		}
		else
			evas_object_show(ad->conformant);

	}

	endfunc;

	return true;
}

static Eina_Bool
_mp_play_view_volume_widget_timer_cb(void *data)
{
	struct appdata *ad = data;
	MP_CHECK_VAL(ad, ECORE_CALLBACK_CANCEL);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_VAL(playing_view, ECORE_CALLBACK_CANCEL);

	mp_evas_object_del(playing_view->volume_widget);
	playing_view->volume_widget_timer = NULL;

	return ECORE_CALLBACK_DONE;
}

static void
_mp_player_view_volume_widget_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	playing_view->volume_widget = NULL;
	mp_ecore_timer_del(playing_view->volume_widget_timer);
}

static void
_mp_play_view_volume_widget_event_cb(void *user_data, Evas_Object *obj, volume_widget_event_e event)
{
	struct appdata *ad = user_data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	if (event == VOLUME_WIDGET_EVENT_DRAG_START) {
		mp_ecore_timer_del(playing_view->volume_widget_timer);
	} else if (event == VOLUME_WIDGET_EVENT_DRAG_STOP) {
		playing_view->volume_widget_timer = ecore_timer_add(VOLUME_WIDGET_HIDE_TIME, _mp_play_view_volume_widget_timer_cb, ad);
	}
}

void
mp_play_view_volume_widget_show(void *data, bool volume_clicked)
{
	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_ecore_timer_del(playing_view->volume_widget_timer);

	DEBUG_TRACE("volume_widget=%p, volume_clicked=%d", playing_view->volume_widget, volume_clicked);
	if (!playing_view->volume_widget) {
		playing_view->volume_widget = mp_volume_widget_add(playing_view->layout);
		elm_object_part_content_set(playing_view->layout, "volume_layout", playing_view->volume_widget);
		mp_volume_widget_event_callback_add(playing_view->volume_widget, _mp_play_view_volume_widget_event_cb, ad);
		evas_object_event_callback_add(playing_view->volume_widget, EVAS_CALLBACK_DEL, _mp_player_view_volume_widget_del_cb, ad);
	} else if (volume_clicked) {
		mp_evas_object_del(playing_view->volume_widget);
		return;
	}

	playing_view->volume_widget_timer = ecore_timer_add(VOLUME_WIDGET_HIDE_TIME, _mp_play_view_volume_widget_timer_cb, ad);
}

static void
_mp_play_view_volume_key_cb(void *user_data, mp_volume_key_e key, bool released)
{
	struct appdata *ad = user_data;
	MP_CHECK(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK(playing_view);

	mp_ecore_timer_del(playing_view->volume_widget_timer);
	if (released) {
		playing_view->volume_widget_timer = ecore_timer_add(VOLUME_WIDGET_HIDE_TIME, _mp_play_view_volume_widget_timer_cb, ad);
		return;
	}

	mp_play_view_volume_widget_show(ad, false);

	if (playing_view->volume_widget) {
		if (key == MP_VOLUME_KEY_DOWN)
			mp_volume_widget_volume_down(playing_view->volume_widget);
		else
			mp_volume_widget_volume_up(playing_view->volume_widget);
	}
}

/**
 * create play view
 *
 * @param data ,appdata
 * @return FALSE or TRUE if it success create
 * @author aramie.kim@samsung.com
 */

bool
mp_play_view_create(void *data)
{
	struct appdata *ad = data;
	mp_retvm_if(!ad, FALSE, "ad is invalid");

	mp_retvm_if(ad->playing_view != NULL, FALSE, "ad playing_view is is valid exit create");

	ad->playing_view = malloc(sizeof(mp_playing_view));
	MP_CHECK_FALSE(ad->playing_view);
	memset(ad->playing_view, 0, sizeof(mp_playing_view));

	_mp_play_view_init(ad);

	mp_volume_key_event_callback_add(_mp_play_view_volume_key_cb, ad);
	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, true);

	return true;
}

bool
mp_play_view_destory(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	if (playing_view->layout)
	{
		mp_evas_object_del(playing_view->layout);	//=> call _mp_play_view_layout_del_cb watchout
	}
	else
		IF_FREE(ad->playing_view);

	return true;
}


bool
mp_play_view_load(void *data)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_play_view_create(data);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	_mp_play_view_create_layout(data);
	MP_CHECK_FALSE(playing_view->layout);

	mp_play_view_refresh(data);

	if (ad->b_show_lyric)
	{
		mp_lyric_view_show(ad);
	}
	endfunc;

	return true;
}

bool
mp_play_view_pop(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);
#ifdef MP_SOUND_PLAYER
	mp_play_view_unswallow_info_ug_layout(ad);

	if (ad->playing_view)
		sp_view_mgr_pop_view_to(ad->view_mgr, SP_VIEW_TYPE_PLAY);
#else
	mp_view_manager_pop_play_view(ad);
#endif
	endfunc;
	return true;
}

bool
mp_play_view_refresh(void *data)
{
	startfunc;

	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	mp_playing_view *playing_view = ad->playing_view;
	MP_CHECK_FALSE(playing_view);

	mp_play_view_refresh_view(ad);

	mp_play_view_set_menu_state(ad, ad->b_show_lyric, false);

	_mp_play_view_init_progress_bar(data);

	const Evas_Object *play_icon = elm_object_part_content_get(ad->playing_view->play_control, "buttons");
	if (play_icon != NULL)
	{
	#ifndef MP_3D_FEATURE
		player_state_e player_state = mp_player_mgr_get_state();
		if (player_state != PLAYER_STATE_PLAYING)
		{
			edje_object_signal_emit(_EDJ(play_icon), "play", CTR_PROG_SIG_SRC);
		}
		else
		{
			edje_object_signal_emit(_EDJ(play_icon), "pause", CTR_PROG_SIG_SRC);
		}
	#endif
	}

	endfunc;
	return true;
}

#ifdef MP_SOUND_PLAYER
void mp_play_view_unswallow_info_ug_layout(struct appdata *ad)
{
	MP_CHECK(ad);

	if(ad->info_ug_base)
	{
		edje_object_part_unswallow(ad->info_ug_base, ad->info_ug_layout);
		evas_object_hide(ad->info_ug_layout);
		mp_ug_send_message(ad, MP_UG_MESSAGE_DEL);
	}
}
#endif

