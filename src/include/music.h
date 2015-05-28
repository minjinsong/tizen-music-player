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


#ifndef __DEF_music_H_
#define __DEF_music_H_

#include <Elementary.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <glib.h>
#include <vconf.h>
#include <Ecore_IMF.h>
#include <Ecore_X.h>
#include <app.h>
#include <Edje.h>
#include <errno.h>
#include <libintl.h>
#include <ui-gadget.h>

#include <sys/times.h>

#include "mp-define.h"
#include "mp-player-debug.h"
#include "mp-player-drm.h"

#include "mp-define.h"
#include "mp-playlist-mgr.h"

#include "mp-http-mgr.h"
#include "mp-ta.h"
#include "mp-language-mgr.h"
#include "mp-media-info.h"

#include "mp-lyric-view.h"
#include "mp-lyric-mgr.h"
#include "mp-play-view.h"


#ifdef MP_SOUND_PLAYER
#include "sp-view-manager.h"
#endif

E_DBus_Connection *EDBusHandle;

typedef struct
{

	/* controlbar tab item */
	Elm_Object_Item *ctltab_songs;
	Elm_Object_Item *ctltab_plist;
	Elm_Object_Item *ctltab_album;
	Elm_Object_Item *ctltab_artist;
	Elm_Object_Item *ctltab_genres;
	Elm_Object_Item *ctltab_year;
	Elm_Object_Item *ctltab_composer;
	Elm_Object_Item *ctltab_folder;



	bool allshare;

	bool first_append;
} mp_library;


#define MP_VIEW_DATA_MAGIC	0x801211aa
#define MP_SET_VIEW_DATA_MAGIC(view_data)	((view_data_t *)view_data)->magic = MP_VIEW_DATA_MAGIC
#define MP_CHECK_VIEW_DATA(view_data)	\
do {                                                  \
	if (((view_data_t *)view_data)->magic != MP_VIEW_DATA_MAGIC) {        \
		ERROR_TRACE("\n###########      ERROR   CHECK  #############\nPARAM is not view_data\n###########      ERROR   CHECK  #############\n"); \
		mp_assert(FALSE);}            \
} while (0)


typedef struct
{
	Evas_Object *layout;
	void *EvasPlugin;
	Evas_Object *box;
	Evas_Object *dali_obj;

	Ecore_Timer *mouse_up_timer;
	Ecore_Timer *now_playing_timer;

	Evas_Object *track_list;
	Evas_Object *track_genlist;
	bool show_track_list;

	int        track_count;

	Evas_Object *now_playing;
	Evas_Object *now_playing_icon;
	Evas_Object *all_tracks;
	bool all_tracks_click;
	Evas_Object* ctxpopup;
	int all_tracks_type;

	Evas_Object *back_button;

	int now_playing_album_seq;

	char *cur_artist;
	char *cur_album;

	Ecore_Job *refresh_job;

	struct appdata *ad;
} mp_coverflow_view;

typedef struct
{
	char *uri;
	char *title;
	char *artist;
	char *album;
	char *genre;
	char *location;
	char *format;

	int duration;

	char *thumbnail_path;
	char *copyright;

	char *author;
	char *track_num;
	bool favorite;
}mp_track_info_t;

enum
{
	MP_CREATE_PLAYLIST_MODE_NONE,
	MP_CREATE_PLAYLIST_MODE_NEW,
	MP_CREATE_PLAYLIST_MODE_WITHMUSICS,
	MP_CREATE_PLAYLIST_MODE_SAVEAS,
	MP_CREATE_PLAYLIST_MODE_SWEEP
};

typedef enum
{
	MP_LAUNCH_DEFAULT = 0,	//normal case
	MP_LAUNCH_BY_PATH,		//ug case
	MP_LAUNCH_ADD_TO_HOME,	//add to home
	MP_LAUNCH_PLAY_RECENT,
	MP_LAUNCH_LIVE_BOX,


} mp_launch_type;

typedef enum
{
	LOAD_DEFAULT,
	LOAD_TRACK,		//load by path
	LOAD_GROUP,		//load by shortcut
	LOAD_PLAYLIST,		//load by shortcut
	LOAD_MM_KEY,

} mp_load_type;

typedef enum
{
	MP_SPLIT_VIEW_TYPE_NORMAL = 0,
	MP_SPLIT_VIEW_TYPE_FULL,
} mp_split_view_type;

typedef struct mp_split_view
{
	Evas_Object *layout;
	Evas_Object *left_layout;
	Evas_Object *right_layout;
	Evas_Object *list;
	Evas_Object *fast_index;
	mp_split_view_type current_split_view_type;
	Ecore_Timer *idle_timer;
	Ecore_Idler *idle_idler;
} mp_split_view;


typedef enum
{
	MP_POPUP_NORMAL = 0,
	MP_POPUP_GENLIST,
	MP_POPUP_CTX,
	MP_POPUP_PROGRESS,
	MP_POPUP_NOTIFY,	// NOT destroyed by mp_popup_destroy()
	MP_POPUP_MAX,
} mp_popup_type;

typedef enum
{
	PLAY_STATE_NONE,
	PLAY_STATE_CREATED,
	PLAY_STATE_PREPARING,
	PLAY_STATE_READY,
	PLAY_STATE_PLAYING,
	PLAY_STATE_PAUSED,
} mp_player_state;

typedef enum {
	MP_SEND_TYPE_MESSAGE,
	MP_SEND_TYPE_EMAIL,
	MP_SEND_TYPE_BLUETOOTH,
	MP_SEND_TYPE_WIFI,
	MP_SEND_TYPE_NFC,
	MP_SEND_TYPE_NUM,
} mp_send_type_e;

typedef enum {
	MP_MORE_BUTTON_TYPE_DEFAULT,
	MP_MORE_BUTTON_TYPE_TRACK_LIST,
	MP_MORE_BUTTON_TYPE_MAX,
} mp_more_button_type_e;

struct appdata
{

	Evas *evas;
	Evas_Object *win_main;
	Evas_Object *bg;
	Evas_Object *popup[MP_POPUP_MAX];
	Evas_Object *ctx_popup;
	Ecore_Idler *popup_del_idler;
	int win_angle;

	Ecore_X_Window xwin;

#ifdef MP_FEATURE_EXIT_ON_BACK
	Ecore_Event_Handler *callerWinEventHandler;
	unsigned int caller_win_id;
#endif
	/* Layout for each view */
	Evas_Object *conformant;
	Evas_Object *base_layout_main;		//layout for transparent indicator area
	Evas_Object *naviframe;
	Evas_Object *controlbar_layout;		//layout for show hide controlbar.

	bool show_optional_menu;
	double music_pos;
	double music_length;
	//int isDragging;
	Ecore_Timer *progressbar_timer;

	mp_playing_view *playing_view;


	// for Plalying Control
	bool can_play_drm_contents;
	bool show_now_playing;

	mp_player_state player_state;

	Evas_Object *bgimage;

	mp_drm drm_info;
	mp_constraints_info_s drm_constraints_info;
	char * latest_playing_key_id;

	mp_plst_mgr *playlist_mgr;

	mp_launch_type launch_type;	// Support Play from outside
	mp_load_type loadtype;	// Support Add to home
	mp_track_type_e track_type;	// Support voice ui
	mp_group_type_e group_type;	// Support voice ui

	char *request_group_name;
	char *request_playing_path;
	mp_group_type_e request_group_type;
	int request_play_id;
	// update default view layout if this flag set. set true when album/artist/genre short cut pressed.
	bool update_default_view;

	Ecore_IMF_Context *imf_context;
#ifdef MP_SOUND_PLAYER
	Sp_View_Manager *view_mgr;
#else
	mp_library *library;
	GList *view_history;
#endif
	Evas_Object *tabbar;
	Evas_Object *genlist_edit;

	Evas_Object *editfiled_new_playlist;
	Evas_Object *editfiled_entry;
	char *new_playlist_name;

	bool b_add_tracks;

	int new_playlist_id;

	struct {
		bool  downed;
        	bool  moving;
		Evas_Coord sx;
		Evas_Coord sy;
	} mouse;

	Evas_Object *radio_group;

	Evas_Object *popup_delete;
	Evas_Object *notify_delete;

	bool b_search_mode;
	Evas_Object *isf_entry;
	Evas_Object *editfield;

	bool freeze_indicator_icon;	//set it true to prevent flickering play icon of indicator.

	int ear_key_press_cnt;
	Ecore_Timer *ear_key_timer;

	Evas_Object *win_minicon;
	Evas_Object *minicontroller_layout;
	Evas_Object *minicon_icon;
	Ecore_Timer *minicon_timer;
	Ecore_Timer *minicon_progress_timer;
	bool b_minicontroller_show;
	bool minicon_visible;

	mp_playing_view *backup_playing_view;
	Evas_Object *backup_layout_play_view;	//Used to swap the Playing layout from Landscape and Portrait.
	mp_split_view *split_view;
	double latest_moved_left_size;

	int current_appcore_rm;
	mp_screen_mode screen_mode;

	int screen_height;	//current screen height
	int screen_width;	//current screen width

	Evas_Object *edit_ctrl_bar;

	Ecore_Pipe *inotify_pipe;

	// for add to playlist
	void *layout_data;
	mp_media_info_h group_item_handler;
	char *fid;

	bool paused_by_user;

	mp_http_mgr_t *http_mgr;

	bool navi_effect_in_progress;	// Use this not to excute button callbacks while transition effect

	bool app_is_foreground;	// relaunch only available when music is in pause state
	bool is_lcd_off;

	bool is_focus_out;	// update minicontroller in bgm mode.

	mp_snd_path snd_path;	// indicate sound path;

	Ecore_Event_Handler *key_down;
	Ecore_Event_Handler *key_up;
	Ecore_Event_Handler *mouse_button_down;
	Ecore_Event_Handler *focus_in;
	Ecore_Event_Handler *focus_out;
	Ecore_Event_Handler *visibility_change;
	Ecore_Event_Handler *client_msg;
	Ecore_Event_Handler *mouse_button_up;
	Ecore_Event_Handler *mouse_move;
	Ecore_Event_Handler *property;

	int motion_handle;

	Evas_Object *buffering_popup;

	ui_gadget_h ug;
	ui_gadget_h setting_ug;
	ui_gadget_h info_ug;	//info ug handle - Do not destory info ug.
	Evas_Object *info_ug_base;	//if thist is not null, info ug is visible. use this to determine info view is exist or not.
	Evas_Object *info_ug_layout; //do not del this object. if it is deleted, info ug layout will not be displayed properly.
	bool info_click_flag;     //flag for click info button in play view
	bool info_back_play_view_flag;     //flag for info view back  play view

	bool edit_in_progress;	// don't update view in inotify callback while delete operation.

	Ecore_Timer *volume_down_timer;
	Ecore_Timer *mute_popup_show_timer;	//timer for showing mute popup
	bool volume_long_pressed;
	bool mute_flag;		//flag for mute

	double ff_rew;		// for ff and rew when there is no play view

	bool music_setting_change_flag;	//true for change music setting data, false for not

	bool load_play_view; /*set it true if play view must be displayed directly when app is launching*/

	bool is_Longpress;
	bool is_ff;

	Ecore_Idler *mss_startup_idler;
	Ecore_Idler *bt_pause_idler;
	Ecore_Idler *setting_idler;
	Ecore_Idler *app_init_idler;
	Ecore_Idler *playview_show_idler;

	Ecore_Timer *longpress_timer;
	bool seek_off;

	mp_lyric_mgr_t *lyric_mgr;
	mp_lyric_view_t *lyric_view;
	bool b_show_lyric;
	bool vertical_scroll;

	char *shortcut_descrition;

	Ecore_Animator *minfo_ani;
	GList *minfo_list;
	Evas_Object *minfo_genlist;

	bool direct_win_minimize;

	int album_image_w;
	int album_image_h;

	bool paused_by_other_player;

	mp_track_info_t *current_track_info;

	Evas_Object *more_btn_popup;
	mp_more_button_type_e more_btn_type;

};


typedef void (*mpOptCallBack) (void *, Evas_Object *, void *);

typedef struct
{
	const char *name;
	mpOptCallBack cb;
} MpOptItemType;

typedef struct
{
	MpOptItemType *l_opt;
	MpOptItemType *m_opt;
	MpOptItemType *r_opt;
} MpOptGroupType;

#endif /* __DEF_music_H__ */
