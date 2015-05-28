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

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <ui-gadget.h>

#include <sys/time.h>
#include <vconf.h>
#include <glib.h>
#include <fcntl.h>
#include <app.h>

#include "music.h"
#include "mp-item.h"
#include "mp-menu.h"
#include "mp-ug-launch.h"
#include "mp-define.h"

#include "music.h"
#include "mp-item.h"
#include "mp-player-debug.h"
#include "mp-play-view.h"
#include "mp-widget.h"
#include "mp-volume.h"

#ifdef MP_SOUND_PLAYER
#include "sp-view-manager.h"
#else
#include "mp-common.h"
#include "mp-group-view.h"
#include "mp-view-manager.h"
#endif

#define UG_EMAIL_NAME "email-composer-efl"
#define UG_BT_NAME "setting-bluetooth-efl"
#define UG_MUSIC_INFO "music-info-efl"
#define UG_MSG_NAME "msg-composer-efl"
#ifdef MP_FEATURE_WIFI_SHARE
#define UG_FTM_NAME "fileshare-efl"
#endif
#define UG_MUSIC_SETTINGS	"setting-music-player-efl"

#define MP_UG_INFO_PATH "path"
#define MP_UG_INFO_ALBUMART "albumart"
#define MP_UG_INFO_ARTIST "artist"
#define MP_UG_INFO_ID "id"
#define MP_UG_INFO_DESTROY "destroy"
#define MP_UG_INFO_BACK "back"
#define MP_UG_INFO_LOAD "load"
#define MP_UG_INFO_ALBUMART_CLICKED "albumart_clicked"
#define MP_UG_INFO_MEDIA_SVC_HANDLE	"media_service_handle"

/* for contact ug */
#define CT_UG_REQUEST_SAVE_RINGTONE 42
#define CT_UG_BUNDLE_TYPE "type"
#define CT_UG_BUNDLE_PATH "ct_path"
#define UG_CONTACTS_LIST "contacts-list-efl"

static void
_mp_ug_layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	startfunc;
	Evas_Object *base, *win;

	MP_CHECK(priv);
	MP_CHECK(ug);

	base = ug_get_layout(ug);
	if (!base)
		return;

	win = ug_get_window();

	switch (mode)
	{
	case UG_MODE_FULLVIEW:
		evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(win, base);
		evas_object_show(base);
		break;
	default:
		break;
	}

	struct appdata *ad = priv;
	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, false);

	endfunc;
}

static void
_mp_ug_info_destroy_cb(ui_gadget_h ug, void *priv)
{
	startfunc;
	if (!ug || !priv)
		return;

	struct appdata *ad = priv;
	ad->info_ug = NULL;
	if (ad->playing_view)
		mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, true);
	return;
}

static void
_mp_ug_info_base_del_cb(void *data, Evas * e, Evas_Object * eo, void *event_info)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	ad->info_ug_base = NULL;
	ad->info_back_play_view_flag = FALSE;
}

static void
_mp_ug_info_layout_resize_cb(void *data, Evas * e, Evas_Object * eo, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);

	int x, y, w, h;
	static int b_x, b_y, b_w, b_h;
	evas_object_geometry_get(eo, &x, &y, &w, &h);
	DEBUG_TRACE("x: %d, y: %d, w: %d, h: %d", x, y, w, h);
	if(eo == ad->info_ug_base && y > 0)
	{
		DEBUG_TRACE("info_ug_base");
		b_x = x;
		b_y = y;
		b_w = w;
		b_h = h;
		return;
	}
	if( y == 0 && ad->info_ug_base)
	{
		DEBUG_TRACE("move layout...");
		evas_object_move(eo, b_x, b_y);
		evas_object_resize(eo, b_w, b_h);
	}
}

static void
_mp_ug_info_push_layout(struct appdata *ad , ui_gadget_h ug)
{
	startfunc;

	Evas_Object *parent = NULL;
#ifdef MP_SOUND_PLAYER
	parent = sp_view_mgr_get_naviframe(ad->view_mgr);
#else
	MP_CHECK(ad->library);
	parent = ad->naviframe;
#endif

	ad->info_ug_base = mp_common_load_edj(parent, PLAY_VIEW_EDJ_NAME, "richinfo/base");
	MP_CHECK(ad->info_ug_base);

	edje_object_part_swallow(_EDJ(ad->info_ug_base), "swallow", ad->info_ug_layout);
	evas_object_event_callback_add(ad->info_ug_base, EVAS_CALLBACK_RESIZE, _mp_ug_info_layout_resize_cb, ad);
#ifdef MP_SOUND_PLAYER
	sp_view_mgr_push_view_content(ad->view_mgr, ad->info_ug_base, SP_VIEW_TYPE_INFO);
	sp_view_mgr_set_title_label(ad->view_mgr, GET_STR("IDS_MUSIC_BODY_MEDIA_INFO"));
	sp_view_mgr_set_back_button(ad->view_mgr, mp_play_view_info_back_cb, ad);
#else
	view_data_t *view_data = evas_object_data_get(ad->naviframe, "view_data");
	mp_view_manager_push_view_content(view_data, ad->info_ug_base, MP_VIEW_CONTENT_INFO);
	mp_view_manager_set_title_and_buttons(view_data, "IDS_MUSIC_BODY_MEDIA_INFO", ad);
#endif
	evas_object_event_callback_add(ad->info_ug_base, EVAS_CALLBACK_DEL, _mp_ug_info_base_del_cb, ad);

	mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, false);
}

static void
_mp_ug_info_layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	startfunc;
	Evas_Object *base, *win;

	MP_CHECK(ug);
	MP_CHECK(priv);

	struct appdata *ad = (struct appdata *)priv;

	base = ug_get_layout(ug);
	MP_CHECK(base);
	ad->info_ug_layout = base;
	evas_object_event_callback_add(ad->info_ug_layout, EVAS_CALLBACK_RESIZE, _mp_ug_info_layout_resize_cb, ad);

	win = ug_get_window();
	MP_CHECK(win);

	switch (mode)
	{
	case UG_MODE_FULLVIEW:
		evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(win, base);
		ug_disable_effect(ug);
		evas_object_show(base);
		break;
	default:
		break;
	}

	_mp_ug_info_push_layout(ad, ug);
	endfunc;
}

void _mp_ug_info_result_cb(ui_gadget_h ug, service_h result, void *priv)
{
	startfunc;
	struct appdata *ad = (struct appdata *)priv;
	if(!result)
		_mp_ug_info_push_layout(ad, ug);
	else
	{
		if(service_remove_extra_data(result, MP_UG_INFO_ALBUMART_CLICKED) == SERVICE_ERROR_NONE)
			mp_play_view_info_back_cb(ad, NULL, NULL);
	}
	endfunc;

}

static ui_gadget_h
_mp_ug_create_ug_layout_only(struct appdata *ad, ui_gadget_h parent, const char *name, enum ug_mode mode, service_h  service,
		 struct ug_cbs *cbs)
{
	startfunc;
	ui_gadget_h ug = NULL;

	ug = ug_create(parent, name, mode, service, cbs);
	if (!ug)
	{
		ERROR_TRACE("unable to create ug %s", name);
	}
	endfunc;
	return ug;
}

static void
_mp_ug_destroy(struct appdata *ad)
{
	startfunc;

	if (ad->ug) {
		ug_destroy(ad->ug);
		ad->ug = NULL;
	}

}

static void
_mp_ug_destroy_cb(ui_gadget_h ug, void *priv)
{
	startfunc;
	if (!ug || !priv)
		return;

	struct appdata *ad = priv;

	_mp_ug_destroy(ad);

	if (ad->playing_view) {
		mp_volume_key_grab_condition_set(MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE, true);
	}

	return;
}

static ui_gadget_h
_mp_ug_create_ug(struct appdata *ad, ui_gadget_h parent, const char *name, enum ug_mode mode, service_h  service,
		 struct ug_cbs *cbs)
{
	startfunc;
	MP_CHECK_NULL(ad);

	ui_gadget_h ug = NULL;

	ug = ug_create(parent, name, mode, service, cbs);

	if (!ug)
	{
		ERROR_TRACE("unable to create ug %s", name);
		mp_widget_text_popup(ad, GET_SYS_STR("IDS_COM_BODY_APPLICATION_NOT_INSTALLED"));
	}
	endfunc;
	return ug;
}

int
mp_ug_email_attatch_file(const char *filepath, void *user_data)
{
#if 1
	bool ret = mp_send_via_appcontrol(user_data, MP_SEND_TYPE_EMAIL, filepath);
	return (ret) ? 0 : -1;
#else
	startfunc;
	struct appdata *ad = NULL;
	service_h service = NULL;
	struct ug_cbs cbs = { 0, };
	int option = 0;

	mp_retvm_if(filepath == NULL, -1, "file path is NULL");
	mp_retvm_if(user_data == NULL, -1, "appdata is NULL");

	ad = (struct appdata *)user_data;
	MP_CHECK_VAL(ad, -1);

	if (ad->ug)
	{
		ERROR_TRACE("Destory previous ui-gadget first !!!!");
		return -1;
	}
	option = UG_OPT_INDICATOR_ENABLE;

	UG_INIT_EFL(ad->win_main, option);

	if(service_create(&service) != SERVICE_ERROR_NONE)
	{
		ERROR_TRACE("Error: service_create");
		return -1;
	}

	service_add_extra_data(service, "RUN_TYPE", "5");
	service_add_extra_data(service, "ATTACHMENT", filepath);

	cbs.priv = user_data;
	cbs.layout_cb = _mp_ug_layout_cb;
	cbs.result_cb = NULL;
	cbs.destroy_cb = _mp_ug_destroy_cb;


	ad->ug = _mp_ug_create_ug(ad, NULL, UG_EMAIL_NAME, UG_MODE_FULLVIEW, service, &cbs);

	service_destroy(service);
	endfunc;
	return 0;
#endif
}

int
mp_ug_message_attatch_file(const char *filepath, void *user_data)
{
	startfunc;
	struct appdata *ad = NULL;
	service_h service = NULL;
	struct ug_cbs cbs = { 0, };
	int option = 0;

	mp_retvm_if(filepath == NULL, -1, "file path is NULL");
	mp_retvm_if(user_data == NULL, -1, "appdata is NULL");

	ad = (struct appdata *)user_data;
	MP_CHECK_VAL(ad, -1);

	if (ad->ug)
	{
		ERROR_TRACE("Destory previous ui-gadget first !!!!");
		return -1;
	}
	option = UG_OPT_INDICATOR_ENABLE;

	UG_INIT_EFL(ad->win_main, option);

	if(service_create(&service) != SERVICE_ERROR_NONE)
	{
		ERROR_TRACE("Error: service_create");
		return -1;
	}
	service_add_extra_data(service, "ATTACHFILE", filepath);

	cbs.priv = user_data;
	cbs.layout_cb = _mp_ug_layout_cb;
	cbs.result_cb = NULL;
	cbs.destroy_cb = _mp_ug_destroy_cb;

	ad->ug = _mp_ug_create_ug(ad, NULL, UG_MSG_NAME, UG_MODE_FULLVIEW, service, &cbs);
	service_destroy(service);
	return 0;
}

int
mp_ug_bt_attatch_file(const char *filepath, int count, void *user_data)
{
	startfunc;
	struct appdata *ad = NULL;
	service_h service = NULL;
	struct ug_cbs cbs = { 0, };
	int option = 0;
	char *file_count = NULL;

	mp_retvm_if(filepath == NULL, -1, "file path is NULL");
	mp_retvm_if(user_data == NULL, -1, "appdata is NULL");

	ad = user_data;
	if (ad->ug)
	{
		ERROR_TRACE("Destory previous ui-gadget first !!!!");
		return -1;
	}

	option = UG_OPT_INDICATOR_ENABLE;

	UG_INIT_EFL(ad->win_main, option);

	file_count = g_strdup_printf("%d", count);

	if(service_create(&service) != SERVICE_ERROR_NONE)
	{
		ERROR_TRACE("Error: service_create");
		return -1;
	}
	service_add_extra_data(service, "launch-type", "send");
	service_add_extra_data(service, "filecount", file_count);
	service_add_extra_data(service, "files", filepath);

	cbs.priv = user_data;
	cbs.layout_cb = _mp_ug_layout_cb;
	cbs.result_cb = NULL;
	cbs.destroy_cb = _mp_ug_destroy_cb;

	ad->ug = _mp_ug_create_ug(ad, NULL, UG_BT_NAME, UG_MODE_FULLVIEW, service, &cbs);

	service_destroy(service);
	IF_FREE(file_count);

	return 0;
}
#ifdef MP_FEATURE_WIFI_SHARE
int
mp_ug_wifi_attatch_file(const char *filepath, int count, void *user_data)
{
	startfunc;
	struct appdata *ad = NULL;
	service_h service = NULL;
	struct ug_cbs cbs = { 0, };
	int option = 0;
	char *file_count = NULL;

	mp_retvm_if(filepath == NULL, -1, "file path is NULL");
	mp_retvm_if(user_data == NULL, -1, "appdata is NULL");

	ad = user_data;
	if (ad->ug)
	{
		ERROR_TRACE("Destory previous ui-gadget first !!!!");
		return -1;
	}
	option = UG_OPT_INDICATOR_ENABLE;
	UG_INIT_EFL(ad->win_main, option);
	file_count = g_strdup_printf("%d", count);
	if(service_create(&service) != SERVICE_ERROR_NONE)
	{
		ERROR_TRACE("Error: service_create");
		return -1;
	}
	service_add_extra_data(service, "filecount", file_count);
	service_add_extra_data(service, "files", filepath);
	cbs.priv = user_data;
	cbs.layout_cb = _mp_ug_layout_cb;
	cbs.result_cb = NULL;
	cbs.destroy_cb = _mp_ug_destroy_cb;
	ad->ug = _mp_ug_create_ug(ad, NULL, UG_FTM_NAME, UG_MODE_FULLVIEW, service, &cbs);
	service_destroy(service);
	IF_FREE(file_count);
	return 0;
}
#endif


int
mp_ug_contact_user_sel(const char *filepath, void *user_data)
{
	startfunc;
	struct appdata *ad = NULL;
	service_h service = NULL;
	struct ug_cbs cbs = { 0, };
	int option = 0;

	mp_retvm_if(filepath == NULL, -1, "file path is NULL");
	mp_retvm_if(user_data == NULL, -1, "appdata is NULL");
	ad = user_data;

	if (ad->ug)
	{
		ERROR_TRACE("Destory previous ui-gadget first !!!!");
		return -1;
	}

	option = UG_OPT_INDICATOR_ENABLE;

	UG_INIT_EFL(ad->win_main, option);

	if(service_create(&service) != SERVICE_ERROR_NONE)
	{
		ERROR_TRACE("Error: service_create");
		return -1;
	}

	char buf[16];

	snprintf(buf, sizeof(buf), "%d", CT_UG_REQUEST_SAVE_RINGTONE);
	service_add_extra_data(service, CT_UG_BUNDLE_TYPE, buf);
	service_add_extra_data(service, CT_UG_BUNDLE_PATH, filepath);

	cbs.priv = ad;
	cbs.layout_cb = _mp_ug_layout_cb;
	cbs.result_cb = NULL;
	cbs.destroy_cb = _mp_ug_destroy_cb;

	ad->ug = _mp_ug_create_ug(ad, NULL, UG_CONTACTS_LIST, UG_MODE_FULLVIEW, service, &cbs);

	service_destroy(service);

	return 0;
}

int
mp_ug_show_info(struct appdata *ad)
{
	startfunc;
	service_h service = NULL;
	struct ug_cbs cbs = { 0, };
	int option = 0;

	MP_CHECK_VAL(ad, -1);
	if (ad->info_ug)
	{
		DEBUG_TRACE("info UG already created");
		mp_ug_send_message(ad, MP_UG_MESSAGE_LOAD);
		return 0;
	}

	option = UG_OPT_INDICATOR_ENABLE;

	UG_INIT_EFL(ad->win_main, option);

	if(service_create(&service) != SERVICE_ERROR_NONE)
	{
		ERROR_TRACE("Error: service_create");
		return -1;
	}
	MP_CHECK_EXCEP(service);

	mp_plst_item *item = mp_playlist_mgr_get_current(ad->playlist_mgr);
	MP_CHECK_EXCEP(item);

	mp_media_info_h media = NULL;
	char *val = NULL;
	mp_media_info_create_by_path(&media, item->uri);

	service_add_extra_data(service, MP_UG_INFO_PATH, item->uri);
	if(!mp_media_info_get_thumbnail_path(media, &val))
		service_add_extra_data(service, MP_UG_INFO_ALBUMART, val);
	if(!mp_media_info_get_artist(media, &val))
		service_add_extra_data(service, MP_UG_INFO_ARTIST, val);
	service_add_extra_data(service, MP_UG_INFO_ID, item->uid);

	mp_media_info_destroy(media);

	cbs.priv = ad;
	cbs.layout_cb = _mp_ug_info_layout_cb;
	cbs.result_cb = _mp_ug_info_result_cb;
	cbs.destroy_cb = _mp_ug_info_destroy_cb;

	//ad->info_ug = _mp_ug_create_ug_layout_only(ad, NULL, UG_MUSIC_INFO, UG_MODE_FULLVIEW, service, &cbs);
	ad->info_ug = _mp_ug_create_ug_layout_only(ad, NULL, UG_MUSIC_INFO, UG_MODE_FRAMEVIEW, service, &cbs);

	service_destroy(service);
	service = NULL;

	MP_CHECK_EXCEP(ad->info_ug);

	return 0;

	mp_exception:
	if(service)
		service_destroy(service);
	return -1;

}

void
mp_ug_send_message(struct appdata *ad, mp_ug_message_t message)
{
	startfunc;
	ui_gadget_h ug = NULL;
	MP_CHECK(ad);

	service_h service = NULL;

	if(service_create(&service) != SERVICE_ERROR_NONE)
	{
		ERROR_TRACE("Error: service_create");
		return;
	}
	MP_CHECK(service);

	switch(message)
	{
	case MP_UG_MESSAGE_DEL:
		ug = ad->info_ug;
		service_add_extra_data(service, MP_UG_INFO_DESTROY, MP_UG_INFO_DESTROY);
		break;
	case MP_UG_MESSAGE_BACK:
		ug = ad->info_ug;
		service_add_extra_data(service, MP_UG_INFO_BACK, MP_UG_INFO_BACK);
		break;
	case MP_UG_MESSAGE_LOAD:
	{
		ug = ad->info_ug;
		service_add_extra_data(service, MP_UG_INFO_LOAD, MP_UG_INFO_LOAD);
		mp_plst_item *item = mp_playlist_mgr_get_current(ad->playlist_mgr);
		MP_CHECK_EXCEP(item);

		mp_media_info_h media = NULL;
		char *val = NULL;
		mp_media_info_create_by_path(&media, item->uri);

		service_add_extra_data(service, MP_UG_INFO_PATH, item->uri);
		if(!mp_media_info_get_thumbnail_path(media, &val))
			service_add_extra_data(service, MP_UG_INFO_ALBUMART, val);
		if(!mp_media_info_get_artist(media, &val))
			service_add_extra_data(service, MP_UG_INFO_ARTIST, val);
		service_add_extra_data(service, MP_UG_INFO_ID, item->uid);

		mp_media_info_destroy(media);
		break;
	}
	default:
		service_destroy(service);
		return;
	}
	ug_send_message(ug, service);

	mp_exception:
	if(service)
		service_destroy(service);
}

void
mp_ug_destory_all(struct appdata *ad)
{
	startfunc;
	// ad->info_ug should not be destoryed!!
	MP_CHECK(ad);
	ug_destroy(ad->ug);
	ad->ug = NULL;
}

bool
mp_ug_active(struct appdata *ad)
{
	startfunc;
	MP_CHECK_FALSE(ad);

	if (ad->ug)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool
mp_send_via_appcontrol(struct appdata *ad, mp_send_type_e send_type, const char *files)
{
	startfunc;
	MP_CHECK_FALSE(ad);

	bool result = false;
	const char *ug_name = NULL;

	service_h service = NULL;
	int ret = service_create(&service);
	if (ret != SERVICE_ERROR_NONE) {
		mp_error("service_create()... [0x%x]", ret);
		goto END;
	}

	ret = service_set_operation(service, SERVICE_OPERATION_SEND);
	if (ret != SERVICE_ERROR_NONE) {
		mp_error("service_set_operation()... [0x%x]", ret);
		goto END;
	}

	ret = service_set_uri(service, files);
	if (ret != SERVICE_ERROR_NONE) {
		mp_error("service_set_uri()... [0x%x]", ret);
		goto END;
	}

	switch (send_type) {
	case MP_SEND_TYPE_EMAIL:
		ug_name = UG_EMAIL_NAME;
		ret = service_add_extra_data(service, "RUN_TYPE", "5");
		if (ret != SERVICE_ERROR_NONE) {
			mp_error("service_add_extra_data()... [0x%x]", ret);
			goto END;
		}
		break;

	default:
		WARN_TRACE("Not supported type.. [%d]", send_type);
		goto END;
	}

	/* appcontrol name */
	ret = service_set_app_id(service, ug_name);
	if (ret != SERVICE_ERROR_NONE) {
		mp_error("service_set_app_id()... [0x%x]", ret);
		goto END;
	}

	/* set window */
	ret = service_set_window(service, ad->xwin);
	if (ret != SERVICE_ERROR_NONE) {
		mp_error("service_set_window()... [0x%x]", ret);
		goto END;
	}

	ret = service_send_launch_request(service, NULL, NULL);
	if (ret != SERVICE_ERROR_NONE) {
		mp_error("service_send_launch_request()... [0x%x]", ret);
		goto END;
	}

	result = true;

END:
	if (service) {
		service_destroy(service);
		service = NULL;
	}

	return result;
}

