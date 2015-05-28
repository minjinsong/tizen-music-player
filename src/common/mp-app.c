/*
 * Copyright 2012	Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "mp-ta.h"
#include "music.h"
#include "mp-setting-ctrl.h"
#include "mp-item.h"
#include "mp-player-control.h"
#include "mp-play-view.h"
#include "mp-playlist-mgr.h"

#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include "mp-player-mgr.h"
#include "mp-player-debug.h"
#include <syspopup_caller.h>
#include <pthread.h>
#include <media_key.h>
#include <utilX.h>
#include "mp-minicontroller.h"
#include "mp-play.h"
#include "mp-app.h"
#include "mp-ug-launch.h"
#include "ui-gadget.h"
#include "mp-widget.h"
#include "mp-util.h"
#include "mp-edit-view.h"
#include "mp-volume.h"

#ifndef MP_SOUND_PLAYER
#include "mp-library.h"
#include "mp-common.h"
#include "mp-group-view.h"
#endif



#ifdef MP_FEATURE_AVRCP_13
#include "mp-avrcp.h"
#endif

static Ecore_Pipe *gNotiPipe;
typedef enum {
	MP_APP_PIPE_CB_AVAILABLE_ROUTE_CHANGED,
	MP_APP_PIPE_CB_ACTIVE_DEVICE_CHANGED,
} mp_app_pipe_cb_type_e;

typedef struct {
	mp_app_pipe_cb_type_e type;
	void *user_data;
	sound_device_out_e out;
} mp_app_pipe_data_s;

#ifdef MP_ENABLE_INOTIFY
static void _mp_add_inofity_refresh_watch(struct appdata *ad);
#endif

static Eina_Bool
_mp_app_ear_key_timer_cb(void *data)
{
	DEBUG_TRACE("");
	struct appdata *ad = (struct appdata *)data;
	if (ad->ear_key_press_cnt == 1)
	{
		DEBUG_TRACE("play/pause ctrl");
		if (ad->player_state == PLAY_STATE_PLAYING)
		{
			ad->paused_by_user = TRUE;
			mp_player_mgr_pause(ad);
		}
		else if (ad->player_state == PLAY_STATE_PAUSED)
		{
			ad->paused_by_user = FALSE;
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
			ad->paused_by_user = FALSE;
			mp_play_current_file(ad);
		}
		else
		{
			ad->paused_by_user = FALSE;
			mp_play_new_file(ad, TRUE);
		}
	}
	else if (ad->ear_key_press_cnt == 2)
	{
		DEBUG_TRACE("next ctrl");
		mp_play_next_file(data, TRUE);
	}
	else
	{
		DEBUG_TRACE("prev ctrl");
		mp_play_prev_file(data);
	}

	ad->ear_key_press_cnt = 0;
	ecore_timer_del(ad->ear_key_timer);
	ad->ear_key_timer = NULL;
	return EINA_FALSE;
}

void
_mp_app_noti_changed_cb(keynode_t * node, void *data)
{
	struct appdata *ad = (struct appdata *)data;

	char *keyname = vconf_keynode_get_name(node);

	DEBUG_TRACE("key changed: %s", keyname);

	if (strcmp(keyname, VCONFKEY_FILEMANAGER_DB_STATUS) == 0)
	{
		int db_state = vconf_keynode_get_int(node);
		if(db_state == VCONFKEY_FILEMANAGER_DB_UPDATED)
		{
			if (mp_playlist_mgr_get_current(ad->playlist_mgr)
			    && !ecore_file_exists(mp_playlist_mgr_get_current(ad->playlist_mgr)->uri))
				mp_play_stop_and_updateview(ad, TRUE);
#ifndef MP_SOUND_PLAYER
			else
				mp_library_update_view(ad);
#endif

#ifdef MP_ENABLE_INOTIFY
			_mp_add_inofity_refresh_watch(ad);
#endif
		}
	}
	else if (strcmp(keyname, VCONFKEY_SETAPPL_SOUND_STATUS_BOOL) == 0)
	{
		bool profile = vconf_keynode_get_bool(node);
		DEBUG_TRACE("profile changed: %d", profile);
		if (profile == false) {
			mp_player_mgr_pause(ad);
		}
	}
	else if (strcmp(keyname, MP_VCONFKEY_PLAYING_PID) == 0)
	{
		int playing_pid = vconf_keynode_get_int(node);
		if (playing_pid != getpid())
		{
			DEBUG_TRACE("other player activated : [pid:%d]", playing_pid);
			if (ad->player_state == PLAY_STATE_PLAYING) {
				ad->paused_by_other_player = TRUE;
				mp_play_control_play_pause(ad, false);
			}

			//mp_minicontroller_destroy(ad);
		}
	}
}

static void
_mp_add_available_route_changed_cb(sound_route_e route, bool available, void *user_data)
{
	DEBUG_TRACE("route: %d, available: %d", route, available);
	MP_CHECK(gNotiPipe);

	mp_app_pipe_data_s pipe_data;
	memset(&pipe_data, 0, sizeof(mp_app_pipe_data_s));
	pipe_data.type = MP_APP_PIPE_CB_AVAILABLE_ROUTE_CHANGED;
	pipe_data.user_data = user_data;

	ecore_pipe_write(gNotiPipe, &pipe_data, sizeof(mp_app_pipe_data_s));
}

static void
_mp_app_active_device_chaged_cb(sound_device_in_e in, sound_device_out_e out, void *user_data)
{
	DEBUG_TRACE("input=[%d], output=[%d]", in, out);
	MP_CHECK(gNotiPipe);

	mp_app_pipe_data_s pipe_data;
	memset(&pipe_data, 0, sizeof(mp_app_pipe_data_s));
	pipe_data.type = MP_APP_PIPE_CB_ACTIVE_DEVICE_CHANGED;
	pipe_data.out = out;
	pipe_data.user_data = user_data;

	ecore_pipe_write(gNotiPipe, &pipe_data, sizeof(mp_app_pipe_data_s));
}

#ifdef MP_FEATURE_AVRCP_13
static void
_mp_app_set_avrcp_mode(struct appdata *ad)
{
	if(mp_playlist_mgr_is_shuffle(ad->playlist_mgr))
		mp_avrcp_noti_shuffle_mode(MP_AVRCP_SHUFFLE_ON);
	else
		mp_avrcp_noti_shuffle_mode(MP_AVRCP_SHUFFLE_OFF);

	int repeat = mp_playlist_mgr_get_repeat(ad->playlist_mgr);
	if(repeat == MP_PLST_REPEAT_ALL)
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_ALL);
	else 	if(repeat == MP_PLST_REPEAT_ONE)
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_ONE);
	else
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_OFF);

	{
		mp_avrcp_noti_eq_state(MP_AVRCP_EQ_OFF);
	}
}

static void
_mp_avrcp_connection_state_changed_cb(bool connected, const char *remote_address, void *user_data)
{
	DEBUG_TRACE("connected: %d, remote_addr: %s", connected, remote_address);
	struct appdata *ad = user_data;
	MP_CHECK(ad);

	if(ad->current_track_info)
	{
		mp_track_info_t *info = ad->current_track_info;
		mp_avrcp_noti_track(info->title, info->artist, info->album, info->genre, info->duration);
	}
	else
		mp_avrcp_noti_track(NULL, NULL, NULL, NULL, 0);

	if(ad->player_state == PLAY_STATE_PLAYING)
		mp_avrcp_noti_player_state(MP_AVRCP_STATE_PLAYING);
	else if(ad->player_state == PLAY_STATE_PAUSED)
		mp_avrcp_noti_player_state(MP_AVRCP_STATE_PAUSED);
	else
		mp_avrcp_noti_player_state(MP_AVRCP_STATE_STOPPED);

	_mp_app_set_avrcp_mode(ad);

}

static void _mp_app_avrcp_shuffle_changed_cb(mp_avrcp_shuffle_mode_e mode, void *user_data)
{
	startfunc;
	struct appdata *ad = user_data;
	MP_CHECK(ad);

	if(mode == MP_AVRCP_SHUFFLE_OFF)
	{
		if (!mp_setting_set_shuffle_state(FALSE))
		{
			mp_play_control_shuffle_set(ad, FALSE);
		}
		mp_playlist_mgr_set_shuffle(ad->playlist_mgr, FALSE);
		mp_avrcp_noti_shuffle_mode(MP_AVRCP_SHUFFLE_OFF);
	}
	else
	{
		if (!mp_setting_set_shuffle_state(TRUE))
		{
			mp_play_control_shuffle_set(ad, TRUE);
		}
		mp_playlist_mgr_set_shuffle(ad->playlist_mgr, TRUE);
		mp_avrcp_noti_shuffle_mode(MP_AVRCP_SHUFFLE_ON);
	}
}
static void _mp_app_repeat_changed_cb(mp_avrcp_repeat_mode_e mode, void *user_data)
{
	startfunc;
	struct appdata *ad = user_data;
	MP_CHECK(ad);

	if(mode == MP_AVRCP_REPEAT_ONE)
	{
		if (!mp_setting_set_repeat_state(MP_SETTING_REP_1))
			mp_play_control_repeat_set(ad, MP_SETTING_REP_1);

		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_ONE);
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_ONE);
	}
	else if(mode == MP_AVRCP_REPEAT_OFF)
	{
		if (!mp_setting_set_repeat_state(MP_SETTING_REP_NON))
			mp_play_control_repeat_set(ad, MP_SETTING_REP_NON);

		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_NONE);
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_OFF);
	}
	else
	{
		if (!mp_setting_set_repeat_state(MP_SETTING_REP_ALL))
			mp_play_control_repeat_set(ad, MP_SETTING_REP_ALL);

		mp_playlist_mgr_set_repeat(ad->playlist_mgr, MP_PLST_REPEAT_ALL);
		mp_avrcp_noti_repeat_mode(MP_AVRCP_REPEAT_ALL);
	}

}
static void _mp_app_avrcp_eq_changed_cb(mp_avrcp_eq_state_e state, void *user_data)
{
	startfunc;
	struct appdata *ad = user_data;
	MP_CHECK(ad);

	if(state == MP_AVRCP_EQ_ON)
	{
		if (vconf_set_int(VCONFKEY_MUSIC_SOUND_ALIVE_VAL, VCONFKEY_MUSIC_SOUND_ALIVE_AUTO))
		{
			WARN_TRACE("Fail to set VCONFKEY_MUSIC_SOUND_ALIVE_VAL");
		}
	}
	else
	{
		if (vconf_set_int(VCONFKEY_MUSIC_SOUND_ALIVE_VAL, VCONFKEY_MUSIC_SOUND_ALIVE_NORMAL))
		{
			WARN_TRACE("Fail to set VCONFKEY_MUSIC_SOUND_ALIVE_VAL");
		}
	}

	if (vconf_set_bool(VCONFKEY_MUSIC_SE_CHANGE, true))
	{
		WARN_TRACE("Fail to set VCONFKEY_MUSIC_SE_CHANGE");
	}
}

#endif

static void
_mp_app_noti_pipe_handler(void *data, void *buffer, unsigned int nbyte)
{
	struct appdata *ad = data;
	MP_CHECK(ad);

	mp_app_pipe_data_s *pipe_data = buffer;
	MP_CHECK(pipe_data);

	switch (pipe_data->type) {
	case MP_APP_PIPE_CB_AVAILABLE_ROUTE_CHANGED:
		if(ad->playing_view)
			mp_play_view_update_snd_path(ad);
		break;

	case MP_APP_PIPE_CB_ACTIVE_DEVICE_CHANGED:
		mp_setting_update_active_device();
		break;

	default:
		WARN_TRACE("Not defined.. [%d]", pipe_data->type);
	}

}

bool
mp_app_noti_init(void *data)
{
	startfunc;
	struct appdata *ad = data;
	bool res = TRUE;
	if (vconf_notify_key_changed(VCONFKEY_FILEMANAGER_DB_STATUS, _mp_app_noti_changed_cb, ad) < 0)
	{
		ERROR_TRACE("Error when register callback\n");
		res = FALSE;
	}
	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, _mp_app_noti_changed_cb, ad) < 0)
	{
		ERROR_TRACE("Fail to register VCONFKEY_MUSIC_MENU_CHANGE key callback");
		res = FALSE;
	}
	if (vconf_notify_key_changed(MP_VCONFKEY_PLAYING_PID, _mp_app_noti_changed_cb, ad) < 0)
	{
		ERROR_TRACE("Fail to register MP_VCONFKEY_PLAYING_PID key callback");
		res = FALSE;
	}

	gNotiPipe = ecore_pipe_add(_mp_app_noti_pipe_handler, ad);
#ifdef MP_FEATURE_AVRCP_13
	if(!mp_avrcp_target_initialize(_mp_avrcp_connection_state_changed_cb, ad))
	{
		_mp_app_set_avrcp_mode(ad);
		if(ad->current_track_info)
		{
			mp_track_info_t *info = ad->current_track_info;
			mp_avrcp_noti_track(info->title, info->artist, info->album, info->genre, info->duration);
		}
	}

	mp_avrcp_set_mode_change_cb(_mp_app_avrcp_shuffle_changed_cb,
		_mp_app_repeat_changed_cb, _mp_app_avrcp_eq_changed_cb, ad);
#endif

	DEBUG_TRACE("Enter sound_manager_set_available_route_changed_cb");
	int ret = sound_manager_set_available_route_changed_cb(_mp_add_available_route_changed_cb, ad);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		ERROR_TRACE("sound_manager_set_available_route_changed_cb().. [0x%x]", ret);
		res = FALSE;
	}
	DEBUG_TRACE("Leave sound_manager_set_available_route_changed_cb");

	DEBUG_TRACE("Enter sound_manager_set_active_device_changed_cb");
	ret = sound_manager_set_active_device_changed_cb(_mp_app_active_device_chaged_cb, ad);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		ERROR_TRACE("sound_manager_set_active_device_changed_cb().. [0x%x]", ret);
		res = FALSE;
	}
	DEBUG_TRACE("Leave sound_manager_set_active_device_changed_cb");

	return res;
}

bool
mp_app_noti_ignore(void)
{
	sound_manager_unset_available_route_changed_cb();
	sound_manager_unset_active_device_changed_cb();

	if (vconf_ignore_key_changed(VCONFKEY_FILEMANAGER_DB_STATUS, _mp_app_noti_changed_cb) < 0)
	{
		ERROR_TRACE("Error when ignore callback\n");
		return FALSE;
	}
	if (vconf_ignore_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, _mp_app_noti_changed_cb) < 0)
	{
		ERROR_TRACE("Error when ignore callback\n");
		return FALSE;
	}

	if (vconf_ignore_key_changed(VCONFKEY_SYSMAN_EARJACKKEY, _mp_app_noti_changed_cb) < 0)
	{
		ERROR_TRACE("Error when ignore callback\n");
		return FALSE;
	}

	if (vconf_ignore_key_changed(VCONFKEY_SYSMAN_MMC_STATUS, _mp_app_noti_changed_cb) < 0)
	{
		ERROR_TRACE("Error when ignore callback\n");
		return FALSE;
	}

	if (vconf_ignore_key_changed(MP_VCONFKEY_PLAYING_PID, _mp_app_noti_changed_cb) < 0)
	{
		ERROR_TRACE("Error when ignore callback\n");
		return FALSE;
	}

#ifdef MP_FEATURE_AVRCP_13
	mp_avrcp_target_finalize();
#endif

	if(gNotiPipe)
	{
		ecore_pipe_del(gNotiPipe);
		gNotiPipe = NULL;
	}
	return TRUE;
}

#ifdef MP_ENABLE_INOTIFY
//#define MP_WATCH_FLAGS IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MODIFY
#define MP_WATCH_FLAGS IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_CLOSE_WRITE | IN_MOVED_TO

#define MP_EVENT_SIZE  (sizeof (struct inotify_event))
/** reasonable guess as to size of 1024 events */
#define MP_EVENT_BUF_LEN (1024 * (MP_EVENT_SIZE + 16))
#define INOTI_FOLDER_COUNT_MAX 1024

typedef struct _mp_inotify_t
{
	int fd;
	GList *wd_list;
	unsigned int prev_event;
	pthread_t monitor;
	mp_inotify_cb callback;
	void *u_data;
} mp_inotify_t;

static pthread_mutex_t mp_noti_lock;
static mp_inotify_t *g_handle;

static void
_mp_app_inotify_handle_free(void)
{
	pthread_mutex_destroy(&mp_noti_lock);

	if (g_handle)
	{
		if (g_handle->fd >= 0)
		{
			close(g_handle->fd);
			g_handle->fd = -1;
		}
		g_free(g_handle);
		g_handle = NULL;
	}

	return;
}

static mp_inotify_t *
_mp_app_inotify_handle_init(void)
{
	_mp_app_inotify_handle_free();
	g_handle = g_new0(mp_inotify_t, 1);

	if (g_handle)
	{
		g_handle->fd = -1;
		pthread_mutex_init(&mp_noti_lock, NULL);
	}

	return g_handle;
}

static void
_mp_app_inotify_thread_clean_up(void *data)
{
	pthread_mutex_t *lock = (pthread_mutex_t *) data;
	DEBUG_TRACE("Thread cancel Clean_up function");
	if (lock)
	{
		pthread_mutex_unlock(lock);
	}
	return;
}


static gpointer
_mp_app_inotify_watch_thread(gpointer user_data)
{
	mp_inotify_t *handle = (mp_inotify_t *) user_data;
	int oldtype = 0;

	mp_retvm_if(handle == NULL, NULL, "handle is NULL");
	DEBUG_TRACE("Create _mp_app_inotify_watch_thread!!! ");

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	while (1)
	{
		ssize_t len = 0;
		ssize_t i = 0;
		char event_buff[MP_EVENT_BUF_LEN] = { 0, };

		if (handle->fd < 0)
		{
			ERROR_TRACE("fd is not a vaild one");
			pthread_exit(NULL);
		}

		len = read(handle->fd, event_buff, sizeof(event_buff) - 1);
		if (len <= 0 || len > sizeof(event_buff) - 1)
		{

		}

		while (i < len)
		{
			struct inotify_event *pevent = (struct inotify_event *)&event_buff[i];
			mp_inotify_event s_event = MP_INOTI_NONE;

			if (pevent->len && strncmp(pevent->name, ".", 1) == 0)
			{
				s_event = MP_INOTI_NONE;
			}
			else if (pevent->mask & IN_ISDIR)	//directory
			{
				/*
				   if (pevent->mask & IN_DELETE_SELF)
				   s_event = MP_INOTI_DELETE_SELF;

				   if (pevent->mask & IN_MOVE_SELF)
				   s_event = MP_INOTI_MOVE_SELF;

				   if (pevent->mask & IN_CREATE)
				   s_event = MP_INOTI_CREATE;

				   if (pevent->mask & IN_DELETE)
				   s_event = MP_INOTI_DELETE;

				   if (pevent->mask & IN_MOVED_FROM)
				   s_event = MP_INOTI_MOVE_OUT;

				   if (pevent->mask & IN_MOVED_TO)
				   s_event = MP_INOTI_MOVE_IN;
				 */
			}
			else	//file
			{
				if (pevent->mask & IN_CREATE)
				{
					s_event = MP_INOTI_NONE;
					handle->prev_event = IN_CREATE;
				}

				if (pevent->mask & IN_CLOSE_WRITE)
				{
					if (handle->prev_event == IN_CREATE)
					{
						s_event = MP_INOTI_CREATE;
					}
					handle->prev_event = MP_INOTI_NONE;
				}

				if (pevent->mask & IN_DELETE)
					s_event = MP_INOTI_DELETE;

				if (pevent->mask & IN_MODIFY)
				{
					s_event = MP_INOTI_MODIFY;
				}

				if (pevent->mask & IN_MOVED_TO)
				{
					s_event = MP_INOTI_MOVE_OUT;
				}
			}

			if (s_event != MP_INOTI_NONE)
			{
				pthread_cleanup_push(_mp_app_inotify_thread_clean_up, (void *)&mp_noti_lock);
				pthread_mutex_lock(&mp_noti_lock);
				if (handle->callback)
				{
					handle->callback(s_event, (pevent->len) ? pevent->name : NULL, handle->u_data);
				}
				pthread_mutex_unlock(&mp_noti_lock);
				pthread_cleanup_pop(0);
			}

			i += sizeof(struct inotify_event) + pevent->len;

			if (i >= MP_EVENT_BUF_LEN)
				break;
		}
	}

	DEBUG_TRACE("end _mp_app_inotify_watch_thread!!! ");

	return NULL;
}

Ecore_Timer *_g_inotyfy_timer = NULL;

static Eina_Bool
_mp_app_inotify_timer_cb(void *data)
{
	bool b_invalid_playing_file = false;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);

	if (ad->edit_in_progress)
	{
		DEBUG_TRACE("editing in progress. not refresh list...");
		return false;
	}

	DEBUG_TRACE("update view");

	mp_plst_item * current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);

	if (current_item)
	{
		if (mp_util_check_uri_available(current_item->uri))
		{
			mp_debug("http uri path");
		}
		else	if (!g_file_test(current_item->uri, G_FILE_TEST_EXISTS))
		{
			mp_play_stop_and_updateview(ad, FALSE);
			b_invalid_playing_file = true;
		}
	}


#ifndef MP_SOUND_PLAYER
	mp_library_update_view(ad);
#endif
	_g_inotyfy_timer = NULL;
	return EINA_FALSE;
}

static void
_mp_app_inotify_cb(mp_inotify_event event, char *path, void *data)
{
	DEBUG_TRACE("file operation occured...");

	struct appdata *ad = (struct appdata *)data;

	MP_CHECK(path);

	ecore_pipe_write(ad->inotify_pipe, path, strlen(path));
}

void
_mp_app_inotify_add_recursive_watch(const char *path, void *ad)
{

	DIR *dp = NULL;
	struct dirent *entry = NULL;
	char *sub_path = NULL;
	sub_path = strdup(path);
	if (mp_app_inotify_add_watch(sub_path, _mp_app_inotify_cb, ad) < 0)
	{
		IF_FREE(sub_path);
		return;
	}

	dp = opendir(sub_path);
	if (dp == NULL)
		return;

	while ((entry = (struct dirent *)readdir(dp)) != NULL)
	{
		if (entry->d_name[0] == '.')
			continue;

		IF_FREE(sub_path);
		sub_path = g_strdup_printf("%s/%s", path, entry->d_name);
		if (entry->d_type == DT_DIR)
			_mp_app_inotify_add_recursive_watch(sub_path, ad);
	}
	IF_FREE(sub_path);

	closedir(dp);

}

static void
_mp_app_pipe_cb(void *data, void *path, unsigned int nbyte)
{
	struct appdata *ad = (struct appdata *)data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	DEBUG_TRACE("%s modified..", path);
	mp_retm_if(ad->app_is_foreground, "Do not refresh list");

	if (_g_inotyfy_timer)
		ecore_timer_del(_g_inotyfy_timer);
	_g_inotyfy_timer = ecore_timer_add(0.5, _mp_app_inotify_timer_cb, data);


}

static void
_mp_add_inofity_refresh_watch(struct appdata *ad)
{
	mp_inotify_t *handle = NULL;
	handle = g_handle;

	MP_CHECK(handle);

	GList *wd_list = handle->wd_list;
	while(wd_list)
	{
		if(wd_list->data >=0)
			mp_app_inotify_rm_watch((int)wd_list->data);
		wd_list = g_list_delete_link(wd_list, wd_list);
	}

	_mp_app_inotify_add_recursive_watch(MP_MMC_ROOT_PATH, ad);
	_mp_app_inotify_add_recursive_watch(MP_PHONE_ROOT_PATH, ad);

}

int
mp_app_inotify_init(void *data)
{

	struct appdata *ad = data;

	mp_inotify_t *handle = NULL;
	handle = _mp_app_inotify_handle_init();
	mp_retvm_if(handle == NULL, -1, "fail to _mp_app_inotify_handle_init()");

	handle->fd = inotify_init();

	if (handle->fd < 0)
	{
		switch (errno)
		{
		case EMFILE:
			ERROR_TRACE("The user limit on the total number of inotify instances has been reached.\n");
			break;
		case ENFILE:
			ERROR_TRACE("The system limit on the total number of file descriptors has been reached.\n");
			break;
		case ENOMEM:
			ERROR_TRACE("Insufficient kernel memory is available.\n");
			break;
		default:
			ERROR_TRACE("Fail to inotify_init(), Unknown error.\n");
			break;
		}
		return -1;
	}
	pthread_create(&handle->monitor, NULL, _mp_app_inotify_watch_thread, handle);

	_mp_app_inotify_add_recursive_watch(MP_MMC_ROOT_PATH, ad);
	_mp_app_inotify_add_recursive_watch(MP_PHONE_ROOT_PATH, ad);

	ad->inotify_pipe = ecore_pipe_add(_mp_app_pipe_cb, (const void *)ad);

	return 0;
}

int
mp_app_inotify_add_watch(const char *path, mp_inotify_cb callback, void *user_data)
{
	mp_inotify_t *handle = NULL;
	GList *wd_list;
	int wd;

	handle = g_handle;
	MP_CHECK_VAL(handle, -1);

	pthread_mutex_lock(&mp_noti_lock);

	wd_list = handle->wd_list;
	wd = inotify_add_watch(handle->fd, path, MP_WATCH_FLAGS);
	if (wd < 0)
	{
		switch (errno)
		{
		case EACCES:
			ERROR_TRACE("Read access to the given file is not permitted.\n");
			break;
		case EBADF:
			ERROR_TRACE("The given file descriptor is not valid.\n");
			handle->fd = -1;
			break;
		case EFAULT:
			ERROR_TRACE("pathname points outside of the process's accessible address space.\n");
			break;
		case EINVAL:
			ERROR_TRACE
				("The given event mask contains no legal events; or fd is not an inotify file descriptor.\n");
			break;
		case ENOMEM:
			ERROR_TRACE("Insufficient kernel memory is available.\n");
			break;
		case ENOSPC:
			ERROR_TRACE
				("The user limit on the total number of inotify watches was reached or the kernel failed to allocate a needed resource.\n");
			break;
		default:
			ERROR_TRACE("Fail to mp_inotify_add_watch(), Unknown error.\n");
			break;
		}
		pthread_mutex_unlock(&mp_noti_lock);
		return -1;
	}

	wd_list = g_list_append(wd_list, (gpointer)wd);
	if(!wd_list) {
		DEBUG_TRACE("g_list_append failed");
		pthread_mutex_unlock(&mp_noti_lock);
		return -1;
	}

	handle->callback = callback;
	handle->u_data = user_data;
	pthread_mutex_unlock(&mp_noti_lock);

	return 0;
}

int
mp_app_inotify_rm_watch(int wd)
{
	int ret = -1;
	mp_inotify_t *handle = NULL;

	handle = g_handle;
	mp_retvm_if(handle == NULL, -1, "handle is NULL");

	if (handle->fd < 0 || wd < 0)
	{
		WARN_TRACE
			("inotify is not initialized or has no watching dir - fd [%d] wd [%d]",
			 handle->fd, wd);
		return 0;
	}

	pthread_mutex_lock(&mp_noti_lock);

	ret = inotify_rm_watch(handle->fd, wd);
	if (ret < 0)
	{
		switch (errno)
		{
		case EBADF:
			ERROR_TRACE("fd is not a valid file descriptor\n");
			handle->fd = -1;
			break;
		case EINVAL:
			ERROR_TRACE("The watch descriptor wd is not valid; or fd is not an inotify file descriptor.\n");
			break;
		default:
			ERROR_TRACE("Fail to mp_inotify_add_watch(), Unknown error.\n");
			break;
		}
		pthread_mutex_unlock(&mp_noti_lock);
		return -1;
	}
	pthread_mutex_unlock(&mp_noti_lock);

	return 0;
}

static void
_mp_app_inotify_wd_list_destroy(gpointer data)
{
	mp_app_inotify_rm_watch((int)data);
}

void
mp_app_inotify_finalize(struct appdata *ad)
{
	mp_inotify_t *handle = NULL;
	handle = g_handle;

	mp_retm_if(handle == NULL, "handle is NULL");

	if (ad->inotify_pipe)
	{
		ecore_pipe_del(ad->inotify_pipe);
		ad->inotify_pipe = NULL;
	}

	g_list_free_full(handle->wd_list, _mp_app_inotify_wd_list_destroy);
	handle->wd_list = NULL;

	pthread_cancel(handle->monitor);
	pthread_join(handle->monitor, NULL);

	_mp_app_inotify_handle_free();

	return;
}
#endif


Eina_Bool
mp_app_key_down_cb(void *data, int type, void *event)
{
	struct appdata *ad = data;
	MP_CHECK_VAL(ad, ECORE_CALLBACK_PASS_ON);

	Ecore_Event_Key *key = event;
	MP_CHECK_VAL(key, ECORE_CALLBACK_PASS_ON);

	if (!g_strcmp0(key->keyname, KEY_VOLUMEUP)) {
		mp_volume_key_event_send(MP_VOLUME_KEY_UP, false);
	}
	else if (!g_strcmp0(key->keyname, KEY_VOLUMEDOWN)) {
		mp_volume_key_event_send(MP_VOLUME_KEY_DOWN, false);
	}

	return ECORE_CALLBACK_PASS_ON;
}

Eina_Bool
mp_app_key_up_cb(void *data, int type, void *event)
{
	struct appdata *ad = data;
	MP_CHECK_VAL(ad, ECORE_CALLBACK_PASS_ON);

	Ecore_Event_Key *key = event;
	MP_CHECK_VAL(key, ECORE_CALLBACK_PASS_ON);

	if (!g_strcmp0(key->keyname, KEY_VOLUMEUP)) {
		mp_volume_key_event_send(MP_VOLUME_KEY_UP, true);
	}
	else if (!g_strcmp0(key->keyname, KEY_VOLUMEDOWN)) {
		mp_volume_key_event_send(MP_VOLUME_KEY_DOWN, true);
	}
	else if (!g_strcmp0(key->keyname, KEY_MEDIA)) {
		DEBUG_TRACE("Key pressed");
		if (ad->ear_key_press_cnt > 3)
		{
			DEBUG_TRACE("pressed more than 3times");
			return ECORE_CALLBACK_PASS_ON;
		}
		if (ad->ear_key_timer)
		{
			ecore_timer_del(ad->ear_key_timer);
			ad->ear_key_timer = NULL;
		}
		ad->ear_key_timer = ecore_timer_add(0.5, _mp_app_ear_key_timer_cb, ad);
		ad->ear_key_press_cnt++;
	}

	return ECORE_CALLBACK_PASS_ON;
}


Eina_Bool
mp_app_mouse_event_cb(void *data, int type, void *event)
{
	struct appdata *ad = data;

	static unsigned int buttons = 0;

	if (type == ECORE_EVENT_MOUSE_BUTTON_DOWN) {
		Ecore_Event_Mouse_Button *ev = event;
		if (!ad->mouse.downed) {
			ad->mouse.downed = TRUE;
			ad->mouse.sx = ev->root.x;
			ad->mouse.sy = ev->root.y;
			buttons = ev->buttons;
		}
	}
	else if (type == ECORE_EVENT_MOUSE_BUTTON_UP) {
		ad->mouse.sx = 0;
		ad->mouse.sy = 0;
		ad->mouse.downed = FALSE;
		ad->mouse.moving = FALSE;
	}
	else if (type == ECORE_EVENT_MOUSE_MOVE) {
	}

	return ECORE_CALLBACK_PASS_ON;
}

void
mp_app_exit(void *data)
{
	struct appdata *ad = data;
	mp_retm_if(ad == NULL, "appdata is NULL");

	DEBUG_TRACE("player_state [%d]\n", ad->player_state);

	elm_exit();
}

#define CTR_EDJ_SIG_SRC "ctrl_edj"

void
_mp_app_media_key_event_cb(media_key_e key, media_key_event_e event, void *user_data)
{
	struct appdata *ad = (struct appdata *)user_data;
	MP_CHECK(ad);

	mp_debug("key [%d], event [%d]", key, event);
	bool released = false;
	if (event == MEDIA_KEY_STATUS_RELEASED)
		released = true;

	if (event == MEDIA_KEY_STATUS_UNKNOWN) {
		mp_debug("unknown key status");
		return;
	}

	const char *signal = NULL;

	switch (key) {
	case MEDIA_KEY_PLAY:
		if (released) {
			if (ad->player_state != PLAY_STATE_PLAYING)
				mp_play_control_play_pause(ad, true);
			else
				DEBUG_TRACE("Already playing state. skip event");
		}
		break;
	case MEDIA_KEY_PAUSE:
		if (released) {
			if (ad->player_state == PLAY_STATE_PLAYING)
				mp_play_control_play_pause(ad, false);
			else
				DEBUG_TRACE("Already pause state. skip event");
		}
		break;
	case MEDIA_KEY_PREVIOUS:
	case MEDIA_KEY_REWIND:
		signal = (released) ? "rew_btn_up" : "rew_btn_down";
		mp_play_control_rew_cb(ad, NULL, signal, CTR_EDJ_SIG_SRC);
		break;

	case MEDIA_KEY_NEXT:
	case MEDIA_KEY_FASTFORWARD:
		signal = (released) ? "ff_btn_up" : "ff_btn_down";
		mp_play_control_ff_cb(ad, NULL, signal, CTR_EDJ_SIG_SRC);
		break;

	case MEDIA_KEY_STOP:
		mp_player_control_stop(ad);
		break;
	default:
		mp_debug("Undefined key");
		break;
	}
}

bool
mp_app_grab_mm_keys(struct appdata *ad)
{
	utilx_grab_key(ecore_x_display_get(), ad->xwin, KEY_MEDIA, OR_EXCLUSIVE_GRAB);
	int err = media_key_reserve(_mp_app_media_key_event_cb, ad);
	if (err != MEDIA_KEY_ERROR_NONE) {
		mp_error("media_key_reserve().. [0x%x]", err);
		return false;
	}

	return true;
}

void
mp_app_ungrab_mm_keys(struct appdata *ad)
{
	media_key_release();
	utilx_ungrab_key(ecore_x_display_get(), ad->xwin, KEY_MEDIA);
}

