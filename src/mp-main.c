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

#include "mp-ta.h"
#include "music.h"
#include "mp-widget.h"
#include "mp-util.h"
#include "mp-setting-ctrl.h"
#include "mp-item.h"
#include "mp-player-control.h"
#include "mp-play-view.h"
#include "mp-http-mgr.h"
#include "mp-playlist-mgr.h"
#include "mp-ug-launch.h"
#include "mp-popup.h"

#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include "mp-player-mgr.h"
#include "mp-player-debug.h"
#include <syspopup_caller.h>
#include <power.h>
#include "mp-minicontroller.h"
#include "mp-app.h"
#include "mp-play.h"
#include "mp-volume.h"
#include "mp-common-defs.h"


#ifdef MP_SOUND_PLAYER
#include "sp-view-manager.h"
#else
#include "mp-library.h"
#include "mp-common.h"
#include "mp-view-manager.h"
#endif




#ifdef MP_FEATURE_AVRCP_13
#include "mp-avrcp.h"
#endif




#ifdef MP_FEATURE_EXIT_ON_BACK
#define MP_EXIT_ON_BACK "ExitOnBack"
#endif


static bool _mp_main_init(struct appdata *ad);
static Eina_Bool _mp_main_win_visibility_change_cb(void *data, int type, void *event);
static Eina_Bool _mp_main_win_focus_in_cb(void *data, int type, void *event);
static Eina_Bool _mp_main_win_focus_out_cb(void *data, int type, void *event);
static Eina_Bool _mp_main_app_init_idler_cb(void *data);

static void
_mp_main_exit_cb(void *data, Evas_Object * obj, void *event_info)
{
	mp_evas_object_del(obj);

	mp_app_exit(data);
}

static void
_mp_evas_flush_post(void *data, Evas * e, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	evas_event_callback_del(e, EVAS_CALLBACK_RENDER_FLUSH_POST, _mp_evas_flush_post);

}

static bool
_mp_main_init(struct appdata *ad)
{
	EDBusHandle = NULL;
	ad->request_play_id = MP_SYS_PLST_NONE;
	ad->music_setting_change_flag = false;

	mp_media_info_connect();

	ad->focus_in = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, _mp_main_win_focus_in_cb, ad);
	ad->focus_out = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _mp_main_win_focus_out_cb, ad);

	ad->app_init_idler = ecore_idler_add(_mp_main_app_init_idler_cb, ad);

	return TRUE;
}

static bool
_mp_main_set_current_playing_item(struct appdata *ad, const char *current_player_path)
{
	startfunc;
	ad->loadtype = LOAD_TRACK;

	mp_playlist_mgr_clear(ad->playlist_mgr);
	mp_plst_item *item = mp_playlist_mgr_item_append(ad->playlist_mgr, current_player_path, NULL, MP_TRACK_URI);
	mp_playlist_mgr_set_current(ad->playlist_mgr, item);

	ad->launch_type = MP_LAUNCH_BY_PATH;

	return true;

}

static bool
_mp_main_is_launching_available(struct appdata *ad)
{
	TA_S(2, "mp_check_battery_available");
	if (mp_check_battery_available())
	{
		Evas_Object *popup = mp_popup_create(ad->win_main, MP_POPUP_NORMAL, NULL, ad, _mp_main_exit_cb, ad);
		elm_object_text_set(popup, GET_SYS_STR("IDS_COM_BODY_LOW_BATTERY"));
		mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_SK_OK"), MP_POPUP_YES);
		mp_popup_timeout_set(popup, MP_POPUP_TIMEOUT);
		evas_object_show(ad->win_main);
		evas_object_show(popup);
		return false;
	}
	TA_E(2,"mp_check_battery_available");

	TA_S(2, "mp_check_mass_storage_mode");
	if (mp_check_mass_storage_mode())
	{
		Evas_Object *popup = mp_popup_create(ad->win_main, MP_POPUP_NORMAL, NULL, ad, _mp_main_exit_cb, ad);
		elm_object_text_set(popup, GET_SYS_STR("IDS_COM_POP_UNABLE_TO_USE_DURING_MASS_STORAGE_MODE"));
		mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_SK_OK"), MP_POPUP_YES);
		mp_popup_timeout_set(popup, MP_POPUP_TIMEOUT);
		evas_object_show(ad->win_main);
		evas_object_show(popup);
		return false;
	}
	TA_E(2,"mp_check_mass_storage_mode");
	return true;
}

#ifndef MP_SOUND_PLAYER
void
_mp_main_create_view_by_item(struct appdata *ad, const char *argv1)
{
	mp_retm_if(argv1 == NULL, "argv1 is NULL");
	mp_retm_if(ad == NULL, "ad is NULL");

	if (argv1)
	{
		if (strstr(argv1, MP_SHORTCUT_PLAYLIST) == argv1)
		{
			int playlist_id = -1;
			char *id;
			id = strtok((char *)argv1, MP_FUNC_ADD_TO_HOME_SEPARATION);
			id = strtok(NULL, MP_FUNC_ADD_TO_HOME_SEPARATION);
			if(id)
			{
				playlist_id = atoi(id);
				DEBUG_TRACE("Playlist id is %d", playlist_id);
				ad->launch_type = MP_LAUNCH_ADD_TO_HOME;
				ad->loadtype = LOAD_PLAYLIST;
				ad->request_play_id = playlist_id;

				mp_library_load(ad);
			}
		}
		else if (strstr(argv1, MP_SHORTCUT_GROUP) == argv1)
		{
			char *id;
			id = strtok((char *)argv1, MP_FUNC_ADD_TO_HOME_SEPARATION);
			id = strtok(NULL, MP_FUNC_ADD_TO_HOME_SEPARATION);
			if (id)
			{
				ad->request_group_type = (atoi(id));
				mp_debug("view_type is %d", ad->request_group_type);

				id = strtok(NULL, MP_FUNC_ADD_TO_HOME_SEPARATION);

				if (id)
				{
					IF_FREE(ad->request_group_name);
					ad->request_group_name = g_strdup(id);

					DEBUG_TRACE("Reqest Album name is %s", ad->request_group_name);
					ad->launch_type = MP_LAUNCH_ADD_TO_HOME;
					ad->loadtype = LOAD_GROUP;
					mp_library_load(ad);
				}
			}
		}
		else if (strstr(argv1, MP_SHORTCUT_SONG) == argv1)
		{
			char current_player_path[255] = { 0, };
			char temp[255] = { 0, };
			strncpy(temp, argv1, 255);

			int i = 0;
			for (i = 0; i < strlen(temp); i++)
			{
				if (temp[i] == '/')
				{
					int j = i + 1;
					while (temp[j])
					{
						current_player_path[j - (i + 1)] = temp[j];
						j++;
					}
					DEBUG_TRACE("current_player_path is %s", current_player_path);
					break;
				}
			}

			ad->launch_type = MP_LAUNCH_ADD_TO_HOME;
			ad->loadtype = LOAD_TRACK;

			IF_FREE(ad->request_playing_path);
			ad->request_playing_path = g_strdup(current_player_path);

		}
	}
}
#endif

static Eina_Bool _mp_main_win_visibility_change_cb(void *data, int type, void *event)
{
	struct appdata *ad = (struct appdata *)data;
	mp_retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "ad is null");

	Ecore_X_Event_Window_Visibility_Change* ev = (Ecore_X_Event_Window_Visibility_Change *)event;
	mp_debug("## Type=[%d], win=[%d], fully_obscured=[%d] ##", type, ev->win, ev->fully_obscured);

	if (ev->win == ad->xwin) {
		/* main window */
		if (ev->fully_obscured == 1) {
			mp_debug("hide main window");
			mp_player_mgr_vol_type_unset();

			ad->app_is_foreground = false;
		} else {
			mp_debug("show main window");
			mp_player_mgr_vol_type_set();
			ad->app_is_foreground = true;
		}
	}

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _mp_main_win_focus_in_cb(void *data, int type, void *event)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	mp_retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "ad is null");


	Ecore_X_Event_Window_Focus_In *ev = (Ecore_X_Event_Window_Focus_In *)event;
	if (ev->win != ad->xwin)
		return ECORE_CALLBACK_PASS_ON;

	ad->is_focus_out = false;
	if (ad->win_minicon && ad->b_minicontroller_show)
		mp_minicontroller_destroy(ad);

#ifndef MP_SOUND_PLAYER
	int db_state = VCONFKEY_FILEMANAGER_DB_UPDATED;
	vconf_get_int(VCONFKEY_FILEMANAGER_DB_STATUS, &db_state);
	if(db_state == VCONFKEY_FILEMANAGER_DB_UPDATING)
	{
		DEBUG_TRACE("update list");
		mp_view_manager_update_list_contents(mp_util_get_view_data(ad), true);
	}
#endif

	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_WINDOW_FOCUS, true);

	return ECORE_CALLBACK_PASS_ON;
}

static void _show_minicontroller(struct appdata *ad)
{
	ad->is_focus_out = true;
	if (ad->player_state == PLAY_STATE_PAUSED || ad->player_state == PLAY_STATE_PLAYING)
	{
		if (!ad->win_minicon)
			mp_minicontroller_create(ad);
		else
			mp_minicontroller_show(ad);
	}
}

static Eina_Bool _mp_main_win_focus_out_cb(void *data, int type, void *event)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	mp_retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "ad is null");


	Ecore_X_Event_Window_Focus_Out *ev = (Ecore_X_Event_Window_Focus_Out *)event;
	if (ev->win != ad->xwin)
		return ECORE_CALLBACK_PASS_ON;

		/* Testing Code. If a track is getting played or paused,
		   the MiniController should be displayed as soon as the main window goes to back ground.
		   When again the Music ICon in Main menu is pressed, the mini controller will be hidden and
		   The Main Screen of the Music Application will be displayed. */
	_show_minicontroller(ad);

	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_WINDOW_FOCUS, false);

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _mp_main_client_message_cb(void *data, int type, void *event)
{
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);

	Ecore_X_Event_Client_Message *ev =
	    (Ecore_X_Event_Client_Message *) event;
	int new_angle = 0;

	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE)
	{
		new_angle = ev->data.l[0];
		DEBUG_TRACE("ROTATION: %d", new_angle);
		mp_minicontroller_rotate(ad, new_angle);
	}
	else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE)
	{
		bool visible = false;
		if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON)
		{
			mp_debug("quickpanel show");
			visible = true;
		}
		else
		{
			mp_debug("quickpanel hide");
			visible = false;
		}
		mp_minicontroller_visible_set(ad, visible);
	}

	return ECORE_CALLBACK_PASS_ON;
}

static void __mp_main_lcd_state_changed_cb(power_state_e state, void *user_data)
{
	DEBUG_TRACE("power_state: %d", state);

	struct appdata *ad = user_data;
	MP_CHECK(ad);

	if(state == POWER_STATE_SCREEN_OFF){
		ad->is_lcd_off = true;
#ifdef MP_SOUND_PLAYER
		mp_play_view_progress_timer_freeze(ad);
#else
		mp_view_manager_freeze_progress_timer(ad);
#endif
	}else if(state == POWER_STATE_NORMAL){
		ad->is_lcd_off = false;
		/* for refresh progressbar */
		ad->music_pos = mp_player_mgr_get_position() / 1000.0;
		ad->music_length = mp_player_mgr_get_duration() / 1000.0;
#ifdef MP_SOUND_PLAYER
		mp_play_view_progress_timer_thaw(ad);
#else
		mp_view_manager_thaw_progress_timer(ad);
#endif
	}
}


Eina_Bool __mp_main_app_prop_change(void *data, int ev_type, void *ev)
{
	startfunc;
	Ecore_X_Event_Window_Property *event = ev;

	if (event->win != ecore_x_window_root_first_get())
		return ECORE_CALLBACK_PASS_ON;

	if (event->atom != ecore_x_atom_get("FONT_TYPE_change"))
		return ECORE_CALLBACK_PASS_ON;

	DEBUG_TRACE("Font is changed!(FONT_TYPE_change)\n");

	//We have a config(font) changed property. Here you can apply to new fonts

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_mp_main_app_init_idler_cb(void *data)
{
	startfunc;
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);

	if (!mp_app_noti_init(ad))
	{
		ERROR_TRACE("Error when noti init");
	}

	ad->key_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, mp_app_key_down_cb, ad);
	ad->key_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, mp_app_key_up_cb, ad);
	ad->mouse_button_down = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, mp_app_mouse_event_cb, ad);
	ad->mouse_button_up = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, mp_app_mouse_event_cb, ad);
	ad->mouse_move = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, mp_app_mouse_event_cb, ad);
	/* window visibility change event */
	ad->visibility_change = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE, _mp_main_win_visibility_change_cb, ad);
	/* window focus in/out event */
	ad->client_msg = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _mp_main_client_message_cb, ad);
	ad->property = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, __mp_main_app_prop_change, ad);

#ifdef MP_ENABLE_INOTIFY
	mp_app_inotify_init(ad);
#endif

	ad->app_init_idler = NULL;

	power_set_changed_cb(__mp_main_lcd_state_changed_cb, ad);




	return ECORE_CALLBACK_CANCEL;
}

static bool __mp_main_service_extra_data_cb(service_h service, const char *key, void *user_data)
{
	MP_CHECK_FALSE(service);
	char * value = NULL;
	service_get_extra_data(service, key, &value);
	DEBUG_TRACE("key: %s, value: %s", key, value);
	IF_FREE(value);

	return true;
}


#ifdef MP_FEATURE_EXIT_ON_BACK
static Eina_Bool
_mp_main_caller_win_destroy_cb(void *data, int type, void *event)
{
	startfunc;
	Ecore_X_Event_Window_Hide *ev;
	struct appdata *ad = data;
	MP_CHECK_VAL(ad, ECORE_CALLBACK_RENEW);

        ev = event;
	if (ev == NULL) {
		DEBUG_TRACE("ev is NULL");
		return ECORE_CALLBACK_RENEW;
	}
	//DEBUG_TRACE("win: %d, caller_win: %d", ev->win ,ad->caller_win_id);
        if(ev->win == ad->caller_win_id) {
		elm_exit();
        }

        return ECORE_CALLBACK_RENEW;
}
#endif


static int
_mp_main_parse_service(struct appdata *ad, service_h service, char **request_title, char **path, int *lunching_by_menu_icon, bool *activate_window)
{
	int ret = 0;
	MP_CHECK_VAL(service, -1);

	service_foreach_extra_data(service, __mp_main_service_extra_data_cb, NULL);
	char *value = NULL;
#ifdef MP_SOUND_PLAYER
	if (!service_get_extra_data(service, MP_REQ_TYPE, &value))
	{
		DEBUG_TRACE("launch by S Voice app. req tyep: %s", value);
	}
	else if(mp_util_get_uri_from_app_svc(service, ad, path))
	{
		DEBUG_TRACE("uri: %s", *path);
	}
	else
		ERROR_TRACE("No uri...");

#ifdef MP_FEATURE_EXIT_ON_BACK
	if(ad->caller_win_id)
	{
		DEBUG_TRACE("unset transient for win: 0x%x", ad->caller_win_id);
		ecore_x_icccm_transient_for_unset(elm_win_xwindow_get(ad->win_main));
		ecore_event_handler_del(ad->callerWinEventHandler);
		ad->caller_win_id = 0;
		ad->callerWinEventHandler = NULL;
	}

	unsigned int id = 0;
	service_get_window(service, &id);
	service_get_extra_data(service, MP_EXIT_ON_BACK, &value);

	DEBUG_TRACE("Caller window id: 0x%x, ExitOnBack: %s", id, value);
	if(id && value && !strcasecmp(value, "true"))
	{
		ecore_x_icccm_transient_for_set(elm_win_xwindow_get(ad->win_main), id);
		ecore_x_window_client_manage(id);

		ad->callerWinEventHandler =
			ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY,
						_mp_main_caller_win_destroy_cb, ad);
		ad->caller_win_id = id;
	}
#endif

#else
	char *uri = NULL;

	*activate_window = true;

	if(!service_get_extra_data(service, "shortcut", &uri))
	{
		if (uri)
		{
			mp_debug("uri = %s", uri);
			*lunching_by_menu_icon = 1;
			*path = uri;
		}
	}
	else if (!service_get_extra_data(service, MP_REQ_TYPE, &value))
	{
		IF_FREE(value);
	}

	else
	{
		if(!service_get_extra_data(service, MP_MM_KEY, &value))
		{
			DEBUG_TRACE("mm key event, ad->player_state : %d", ad->player_state);
			*activate_window = false;
			if(ad->player_state == PLAY_STATE_PAUSED)
			{
				ad->launch_type = MP_LAUNCH_DEFAULT;
				ad->loadtype = LOAD_DEFAULT;
				mp_play_control_play_pause(ad, true);
			}
			else
			{
				ad->launch_type = MP_LAUNCH_PLAY_RECENT;
				ad->loadtype = LOAD_MM_KEY;

				int count;
				mp_media_list_h media = NULL;

				*activate_window = false;
				mp_media_info_list_count(MP_TRACK_BY_PLAYED_TIME, NULL, NULL, NULL, 0, &count);
				if(!count)
				{
					mp_media_info_list_count(MP_TRACK_ALL, NULL, NULL, NULL, 0, &count);
					mp_media_info_list_create(&media, MP_TRACK_ALL, NULL, NULL, NULL, 0, 0, count);
				}
				else
					mp_media_info_list_create(&media, MP_TRACK_BY_PLAYED_TIME, NULL, NULL, NULL, 0, 0, count);

				mp_util_append_media_list_item_to_playlist(ad->playlist_mgr, media, count, 0, NULL);
				mp_media_info_list_destroy(media);
			}
			IF_FREE(value);
		}
	}

#endif
	return ret;
}


static void
_mp_atexit_cb(void)
{
	ERROR_TRACE("%%%%%%%%%%%%%%%%%%%%%");
	ERROR_TRACE("#exit() invoked. music-player is exiting");
	ERROR_TRACE("%%%%%%%%%%%%%%%%%%%%%");
}

static void
_mp_playlist_item_change_callback(mp_plst_item *item, void *userdata)
{
	struct appdata *ad = userdata;
	MP_CHECK(ad);

	if(ad->current_track_info)
	{
		mp_util_free_track_info(ad->current_track_info);
		ad->current_track_info = NULL;
	}

	if(item)
	{
		mp_util_load_track_info(ad, item, &ad->current_track_info);
#ifdef MP_FEATURE_AVRCP_13
		mp_avrcp_noti_track(ad->current_track_info->title,
			ad->current_track_info->artist, ad->current_track_info->album,
			ad->current_track_info->genre, ad->current_track_info->duration);
#endif
	}

}

/**< Called before main loop */
static bool
mp_create(void *data)
{
	struct appdata *ad = data;

	atexit(_mp_atexit_cb);

	MP_CHECK_VAL(ad, EINA_FALSE);

	TA_S(0, "mp_create");

	TA_S(1, "elm_theme_extension_add");
	/* do extension add before add elm object.*/
	elm_theme_extension_add(NULL, THEME_NAME);
	TA_E(1,"elm_theme_extension_add");

	TA_S(1, "bindtextdomain");
	bindtextdomain(DOMAIN_NAME, LOCALE_DIR);
	DEBUG_TRACE("DOMAIN_NAME: %s, LOCALE_DIR: %s", DOMAIN_NAME, LOCALE_DIR);
	TA_E(1,"bindtextdomain");

	TA_S(1, "mp_create_win");
	ad->win_main = mp_create_win(GET_SYS_STR("IDS_COM_BODY_MUSIC"));
	mp_retv_if(ad->win_main == NULL, EINA_FALSE);
	elm_win_indicator_mode_set(ad->win_main, ELM_WIN_INDICATOR_SHOW);
	evas_event_callback_add(evas_object_evas_get(ad->win_main), EVAS_CALLBACK_RENDER_FLUSH_POST,
				_mp_evas_flush_post, ad);
	ad->xwin = elm_win_xwindow_get(ad->win_main);
	mp_volume_key_grab_set_window(ad->xwin);
	TA_E(1,"mp_create_win");

	TA_S(1, "mp_widget_create_bgimg");
	ad->bgimage = mp_widget_create_bgimg(ad->win_main);
	ad->evas = evas_object_evas_get(ad->win_main);
	TA_E(1,"mp_widget_create_bgimg");

	TA_S(1, "mp_player_mgr_session_init");
	/*initialize session type */
	if (!mp_player_mgr_session_init())
	{
		ERROR_TRACE("Error when set session");
		return EINA_FALSE;
	}
	TA_E(1,"mp_player_mgr_session_init");

	TA_S(1, "mp_setting_init");
	mp_setting_init(ad);
	TA_E(1,"mp_setting_init");

	TA_S(1, "_mp_main_init");
	if (!_mp_main_init(ad))
	{
		ERROR_TRACE("Fail when init music");
		return EINA_FALSE;
	}
	TA_E(1,"_mp_main_init");


#ifdef ENABLE_RICHINFO
	xmlInitParser();
#endif

	TA_S(1, "mp_http_mgr_create");
	mp_http_mgr_create(ad);
	TA_E(1,"mp_http_mgr_create");


	TA_S(1, "mp_playlist_mgr_create");
	ad->playlist_mgr = mp_playlist_mgr_create();
	mp_playlist_mgr_set_item_change_callback(ad->playlist_mgr, _mp_playlist_item_change_callback, ad);
	int val = 0;
	mp_setting_get_shuffle_state(&val);
	mp_playlist_mgr_set_shuffle(ad->playlist_mgr, val);
#ifdef MP_SOUND_PLAYER
	val = MP_SETTING_REP_NON;
#else
	mp_setting_get_repeat_state(&val);
#endif
	mp_playlist_mgr_set_repeat(ad->playlist_mgr, val);
	TA_E(1,"mp_playlist_mgr_create");

	TA_S(1, "mp_language_mgr_create");
	mp_language_mgr_create();
	TA_E(1,"mp_language_mgr_create");

	TA_S(1, "mp_conformant_add");
	Evas_Object *conformant = elm_conformant_add(ad->win_main);
	MP_CHECK_FALSE(conformant);
	evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(conformant);
	elm_win_resize_object_add(ad->win_main, conformant);
	ad->conformant = conformant;
	TA_E(1,"mp_conformant_add");

	TA_E(0, "mp_create");
	return EINA_TRUE;
}

/**< Called after main loop */
static void
mp_terminate(void *data)
{
	struct appdata *ad = data;
	DEBUG_TRACE_FUNC();
	MP_CHECK(ad);

	mp_language_mgr_destroy();
	mp_ecore_idler_del(ad->mss_startup_idler);
	mp_ecore_idler_del(ad->bt_pause_idler);
	mp_ecore_idler_del(ad->app_init_idler);
	mp_ecore_idler_del(ad->popup_del_idler);
	mp_ecore_timer_del(ad->longpress_timer);

	mp_app_ungrab_mm_keys(ad);

	if (ad->key_down)
		ecore_event_handler_del(ad->key_down);
	if (ad->key_up)
		ecore_event_handler_del(ad->key_up);
	if (ad->mouse_button_down)
		ecore_event_handler_del(ad->mouse_button_down);
	if (ad->visibility_change) {
		ecore_event_handler_del(ad->visibility_change);
		ad->visibility_change = NULL;
	}
	if (ad->focus_in) {
		ecore_event_handler_del(ad->focus_in);
		ad->focus_in = NULL;
	}
	if (ad->focus_out) {
		ecore_event_handler_del(ad->focus_out);
		ad->focus_out = NULL;
	}

	if (ad->player_state != PLAY_STATE_NONE)
	{
		mp_player_mgr_stop(ad);
		mp_player_mgr_destroy(ad);
	}

	if (!mp_util_is_other_player_playing())
	{
		vconf_set_int(VCONFKEY_MUSIC_STATE, VCONFKEY_MUSIC_OFF);
	}
	mp_minicontroller_destroy(ad);
	mp_player_mgr_vol_type_unset();
	if (!mp_player_mgr_session_finish())
		ERROR_TRACE("Error when set session");
#ifdef MP_ENABLE_INOTIFY
	mp_app_inotify_finalize(ad);
#endif
	mp_http_mgr_destory(ad);

	mp_media_info_disconnect();

#ifdef ENABLE_RICHINFO
	xmlCleanupParser();
#endif
	mp_setting_deinit(ad);
	if (!mp_app_noti_ignore())
		ERROR_TRACE("Error when ignore noti");


#ifdef MP_SOUND_PLAYER
	sp_view_mgr_destroy(ad->view_mgr);
	ad->view_mgr = NULL;
#endif


	MP_TA_ACUM_ITEM_SHOW_RESULT_TO(MP_TA_SHOW_FILE);
	MP_TA_RELEASE();

	mp_lyric_mgr_destory(ad);





	return;
}

/**< Called when every window goes back */
static void
mp_pause(void *data)
{
	DEBUG_TRACE_FUNC();

	return;
}

/**< Called when any window comes on top */
static void
mp_resume(void *data)
{
	DEBUG_TRACE_FUNC();

	return;
}

/**< Called at the first idler and relaunched by AUL*/
static void
mp_service(service_h service, void *data)
{
	startfunc;

	struct appdata *ad = data;
	mp_ret_if(ad == NULL);

	if(ad->app_is_foreground)
	{
		DEBUG_TRACE("relaunch is not allowed");
		elm_win_activate(ad->win_main);
		return;
	}
	else
		ad->app_is_foreground = true;

	TA_S(0, "mp_service");

	TA_S(1, "_mp_main_is_launching_available");
	if (!_mp_main_is_launching_available(ad))
		return;
	TA_E(1,"_mp_main_is_launching_available");

	char *request_title = NULL;
	int lunching_by_menu_icon = 0;
	char *path = NULL;
	bool activate_window = true;

	ad->launch_type = MP_LAUNCH_DEFAULT;
	ad->loadtype = LOAD_DEFAULT;

	TA_S(1, "_mp_main_parse_service");
	if(_mp_main_parse_service(ad, service, &request_title, &path, &lunching_by_menu_icon, &activate_window))
	{
		ERROR_TRACE("Error: _mp_main_parse_service");
		elm_exit();
		return;
	}
	TA_E(1,"_mp_main_parse_service");

#ifdef MP_SOUND_PLAYER

DEBUG_TRACE("%s:1:path=%s\n", __func__, path);
	if (path) {
		_mp_main_set_current_playing_item(ad, path);
		free(path);
	}
	else {
		mp_error("no path");
		mp_app_exit(ad);
		return;
	}

	IF_FREE(ad->latest_playing_key_id);
	if (!ad->base_layout_main) {

		ad->base_layout_main = mp_widget_create_layout_main(ad->conformant);
		elm_object_content_set(ad->conformant, ad->base_layout_main);

		ad->loadtype = LOAD_TRACK;
		/* create base navi bar */
		Evas_Object *navibar = mp_widget_navigation_new(ad->base_layout_main, ad);
		elm_object_part_content_set(ad->base_layout_main, "elm.swallow.content", navibar);

		ad->view_mgr = sp_view_mgr_create(navibar);
		ad->naviframe = navibar;
	}

	mp_evas_object_del(ad->ctx_popup);
	mp_play_destory(ad);
	mp_play_view_load(ad);
	ad->paused_by_user = FALSE;

	mp_play_new_file(data, TRUE);
#else
DEBUG_TRACE("%s:1:path=%s\n", __func__, path);
	if (path)
	{
		if (lunching_by_menu_icon)
			_mp_main_create_view_by_item(ad, path);
		else
		{
			_mp_main_set_current_playing_item(ad, path);
			IF_FREE(ad->request_playing_path);
			ad->request_playing_path = g_strdup(path);
		}
		free(path);
	}

	DEBUG_TRACE("ad->launch_type:%d, ad->loadtype:%d", ad->launch_type, ad->loadtype);

	if (ad->launch_type != MP_LAUNCH_DEFAULT)
	{
		//destory popup
		mp_popup_destroy(ad);

		//destroy info view..
		mp_view_manager_pop_info_view(ad);

		//destroy ug.
		mp_ug_destory_all(ad);

	}

	TA_S(1, "mp_library_create");
	if (ad->base_layout_main == NULL)
		mp_library_create(ad);
	TA_E(1, "mp_library_create");

	if (ad->launch_type != MP_LAUNCH_DEFAULT &&
		(ad->loadtype == LOAD_TRACK))
	{
		IF_FREE(ad->latest_playing_key_id);
		ad->load_play_view = true;

		ad->b_add_tracks = 0;
		elm_toolbar_item_selected_set(ad->library->ctltab_songs, EINA_TRUE);
		Evas_Object *layout = mp_view_manager_get_last_view_layout(ad);
		mp_view_layout_set_edit_mode(mp_util_get_layout_data(layout), false);

		if(ad->loadtype == LOAD_TRACK)
		{
			mp_play_view_load_by_path(ad, ad->request_playing_path);
		}
		else
		{
			mp_play_view_load_and_play(ad, NULL, FALSE);
		}

		IF_FREE(ad->request_playing_path);

		ad->paused_by_user = FALSE;

	}
	else if(ad->loadtype == LOAD_MM_KEY)
	{
		ad->launch_type = MP_LAUNCH_DEFAULT;
		ad->loadtype= LOAD_DEFAULT;
		mp_play_new_file(ad, TRUE);
		_show_minicontroller(ad);
	}
#endif


	TA_S(1, "evas_object_show");
	evas_object_show(ad->win_main);
	TA_E(1,"evas_object_show");

	DEBUG_TRACE("activate window");
	TA_S(1, "elm_win_activate");
	elm_win_activate(ad->win_main);
	TA_E(1,"elm_win_activate");

	TA_S(1, "mp_player_mgr_vol_type_set");
	mp_player_mgr_vol_type_set();
	TA_E(1,"mp_player_mgr_vol_type_set");

	TA_E(0,"MP-LAUNCH_TIME");
	TA_E(0,"mp_service");

	endfunc;

	return;
}

static void
mp_low_battery(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	if (mp_check_battery_available())
	{
		mp_app_exit(ad);
	}
	return;
}

void
mp_device_orientation_cb(app_device_orientation_e orientation, void *user_data)
{
	struct appdata *ad = user_data;
	MP_CHECK(ad);

}

static void
__mp_language_changed_cb(void *user_data)
{
	DEBUG_TRACE("");
	struct appdata *ad = user_data;
	mp_popup_destroy(ad);
#ifndef MP_SOUND_PLAYER
	mp_view_manager_pop_info_view(ad);
#endif
	mp_ug_destory_all(ad);
	mp_language_mgr_update();
}

int
main(int argc, char *argv[])
{
	struct appdata ad;
	app_event_callback_s event_callbacks;

	event_callbacks.create = mp_create;
	event_callbacks.terminate = mp_terminate;
	event_callbacks.pause = mp_pause;
	event_callbacks.resume = mp_resume;
	event_callbacks.service = mp_service;
	event_callbacks.low_memory = NULL;
	event_callbacks.low_battery = mp_low_battery;
	event_callbacks.device_orientation = mp_device_orientation_cb;
	event_callbacks.language_changed = __mp_language_changed_cb;
	event_callbacks.region_format_changed = NULL;

	DEBUG_TRACE(" starting music main");

	MP_TA_INIT();
	TA_S(0, "MP-LAUNCH_TIME");
	memset(&ad, 0x0, sizeof(struct appdata));
	return app_efl_main(&argc, &argv, &event_callbacks, &ad);
}
