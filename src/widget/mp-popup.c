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
#include <bundle.h>
#include <stdio.h>
#include "music.h"
#include "mp-menu.h"
#include "mp-popup.h"
#include "mp-item.h"
#include "mp-player-debug.h"
#include "mp-view-layout.h"
#include "mp-playlist-mgr.h"
#include "mp-common.h"
#include <sound_manager.h>
#include "mp-util.h"

#include "mp-widget.h"

static Elm_Genlist_Item_Class itc;

typedef struct {
	struct appdata *ad;
	mp_popup_type type;

	Evas_Smart_Cb response_cb;
	void *cb_data;
} Popup_Data;

#define mp_popup_set_popup_data(obj, data) evas_object_data_set((obj), "popup_data", (data))
#define mp_popup_get_popup_data(obj) evas_object_data_get((obj), "popup_data")

static char *
_mp_popup_gl_label_get(void *data, Evas_Object * obj, const char *part)
{
	char *label = (char *)data;
	DEBUG_TRACE("%s", label);
	return strdup(label);
}

static Evas_Object *
_mp_popup_gl_icon_get(void *data, Evas_Object * obj, const char *part)
{
	DEBUG_TRACE("");
	MP_CHECK_NULL(data);

	struct appdata *ad = evas_object_data_get(obj, "ad");
	MP_CHECK_NULL(ad);

	Evas_Object *radio = elm_radio_add(obj);
	elm_radio_group_add(radio, ad->radio_group);

	if (!strcmp(GET_SYS_STR("IDS_COM_OPT_HEADPHONES_ABB"), data))
	{
		elm_radio_state_value_set(radio, MP_SND_PATH_EARPHONE);
		evas_object_data_set(radio, "idx", (void *)(MP_SND_PATH_EARPHONE));
	}
	else if (!strcmp(GET_STR("Speaker"), data))
	{
		elm_radio_state_value_set(radio, MP_SND_PATH_SPEAKER);
		evas_object_data_set(radio, "idx", (void *)(MP_SND_PATH_SPEAKER));
	}
	else
	{
		elm_radio_state_value_set(radio, MP_SND_PATH_BT);
		evas_object_data_set(radio, "idx", (void *)(MP_SND_PATH_BT));
	}

	evas_object_show(radio);

	elm_radio_value_set(ad->radio_group, ad->snd_path);

	return radio;
}

static void
mp_popup_set_min_size(Evas_Object *box, int cnt)
{
	int min_h = 0;
	MP_CHECK(box);
	if(cnt > 3)
		min_h = MP_POPUP_GENLIST_ITEM_H_MAX;
	else
		min_h = MP_POPUP_GENLIST_ITEM_H*cnt + cnt -1;

	evas_object_size_hint_min_set(box,
			MP_POPUP_GENLIST_ITEM_W * elm_config_scale_get(), min_h * elm_config_scale_get());
}

static void
_mp_popup_cancel_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	evas_object_del(obj);
}

static Evas_Object *
_mp_popup_create_min_style_popup(Evas_Object * parent, char *title, int cnt,
				void *user_data, Evas_Smart_Cb cb, struct appdata *ad)
{
	Evas_Object *genlist = NULL;
	Evas_Object *box = NULL;
	Evas_Object *popup = mp_popup_create(parent, MP_POPUP_GENLIST, title, user_data, cb, ad);
	MP_CHECK_NULL(popup);

	box = elm_box_add(popup);
	MP_CHECK_NULL(box);

	mp_popup_set_min_size(box, cnt);

	genlist = elm_genlist_add(box);
	MP_CHECK_NULL(genlist);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	evas_object_data_set(popup, "genlist", genlist);

	evas_object_show(genlist);
	elm_box_pack_end(box, genlist);
	elm_object_content_set(popup, box);
	evas_object_show(box);

	evas_object_show(popup);

	return popup;
}

static Evas_Object *
_mp_popup_pv_set_as_create(Evas_Object * parent, void *user_data, bool add_to_home, struct appdata *ad)
{
	DEBUG_TRACE("");
	MP_CHECK_NULL(ad);
	Evas_Object *popup = NULL;
	Evas_Object *genlist;

	popup = _mp_popup_create_min_style_popup(parent, GET_SYS_STR("IDS_COM_SK_SET"), add_to_home ? 3:2, NULL, _mp_popup_cancel_button_cb, ad);
	MP_CHECK_NULL(popup);

	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_POP_CANCEL"), MP_POPUP_NO);

	itc.item_style = "1text";
	itc.func.text_get = _mp_popup_gl_label_get;
	itc.func.content_get = NULL;
	itc.func.state_get = NULL;
	itc.func.del = NULL;

	genlist = evas_object_data_get(popup, "genlist");
	MP_CHECK_NULL(genlist);


	elm_genlist_item_append(genlist, &itc, GET_STR(CALL_RINGTONE), NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_set_as_select_cb, user_data);

	elm_genlist_item_append(genlist, &itc, GET_STR(CALLER_RINGTONE), NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_set_as_select_cb, user_data);

	return popup;
}

static Evas_Object *
_mp_popup_pv_share_create(Evas_Object * parent, void *user_data, struct appdata *ad)
{
	DEBUG_TRACE("");
	MP_CHECK_NULL(ad);
	Evas_Object *popup = NULL;
	Evas_Object *genlist;
	int option_count = 3;
#ifdef MP_FEATURE_WIFI_SHARE
	++option_count;
#endif

	popup = _mp_popup_create_min_style_popup(parent, GET_SYS_STR("IDS_COM_BUTTON_SHARE"), option_count, NULL, _mp_popup_cancel_button_cb, ad);

	MP_CHECK_NULL(popup);
	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_POP_CANCEL"), MP_POPUP_NO);

	itc.item_style = "1text";
	itc.func.text_get = _mp_popup_gl_label_get;
	itc.func.content_get = NULL;
	itc.func.state_get = NULL;
	itc.func.del = NULL;
	genlist = evas_object_data_get(popup, "genlist");
	MP_CHECK_NULL(genlist);

	elm_genlist_item_append(genlist, &itc, BLUETOOTH_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_select_cb, user_data);

	elm_genlist_item_append(genlist, &itc, EMAIL_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_select_cb, user_data);
	elm_genlist_item_append(genlist, &itc, MESSAGE_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_select_cb, user_data);
#ifdef MP_FEATURE_WIFI_SHARE
	elm_genlist_item_append(genlist, &itc, WIFI_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_select_cb, user_data);
#endif

	return popup;
}

static Evas_Object *
_mp_popup_list_share_create(Evas_Object * parent, void *user_data, struct appdata *ad)
{
	DEBUG_TRACE("");
	MP_CHECK_NULL(ad);
	Evas_Object *popup = NULL;
	Evas_Object *genlist;
	int option_count = 3;
#ifdef MP_FEATURE_WIFI_SHARE
	++option_count;
#endif


	popup = _mp_popup_create_min_style_popup(parent, GET_SYS_STR("IDS_COM_BUTTON_SHARE"), option_count, NULL, _mp_popup_cancel_button_cb, ad);
	MP_CHECK_NULL(popup);
	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_POP_CANCEL"), MP_POPUP_NO);

	itc.item_style = "1text";
	itc.func.text_get = _mp_popup_gl_label_get;
	itc.func.content_get = NULL;
	itc.func.state_get = NULL;
	itc.func.del = NULL;

	genlist = evas_object_data_get(popup, "genlist");
	MP_CHECK_NULL(genlist);

	elm_genlist_item_append(genlist, &itc, BLUETOOTH_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_list_select_cb, user_data);
	elm_genlist_item_append(genlist, &itc, EMAIL_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_list_select_cb, user_data);

	elm_genlist_item_append(genlist, &itc, MESSAGE_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_list_select_cb, user_data);
#ifdef MP_FEATURE_WIFI_SHARE
	elm_genlist_item_append(genlist, &itc, WIFI_SYS, NULL,
				       ELM_GENLIST_ITEM_NONE, mp_menu_share_list_select_cb, user_data);
#endif

	return popup;
}

static Evas_Object *
_mp_popup_sound_path_create(Evas_Object * parent, void *data, struct appdata *ad)
{
	DEBUG_TRACE_FUNC();
	MP_CHECK_NULL(ad);

	Evas_Object *popup = NULL;
	bool bt_connected = false;
	int earjack = 0;
	int ret = 0;
	char *bt_name = NULL;
	int cnt = 1;

	ret = sound_manager_get_a2dp_status(&bt_connected, &bt_name);
	if (ret != SOUND_MANAGER_ERROR_NONE)
	{
		WARN_TRACE("Fail to sound_manager_get_a2dp_status ret = [%d]", ret);
	}
	IF_FREE(bt_name);
	if(bt_connected)
		cnt++;

	if (vconf_get_int(VCONFKEY_SYSMAN_EARJACK, &earjack))
		WARN_TRACE("Earjack state get Fail...");
	if(earjack)
		cnt++;

	popup = _mp_popup_create_min_style_popup(parent, GET_SYS_STR("IDS_COM_HEADER_AUDIO_DEVICE_ABB"), cnt, NULL, _mp_popup_cancel_button_cb, ad);
	MP_CHECK_NULL(popup);
	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_POP_CANCEL"), MP_POPUP_NO);

	return popup;
}

static Evas_Object *
_mp_popup_add_to_playlist_create(Evas_Object * parent, void *data, struct appdata *ad)
{
	DEBUG_TRACE_FUNC();
	MP_CHECK_NULL(ad);
	Evas_Object *popup = NULL;
	int ret;
	int count = 0;

	ret = mp_media_info_group_list_count(MP_GROUP_BY_PLAYLIST, NULL, NULL, &count);
	DEBUG_TRACE("count,%d", count);

	popup = _mp_popup_create_min_style_popup(parent, GET_STR("IDS_MUSIC_BODY_ADD_TO_PLAYLIST"), count+1, NULL, _mp_popup_cancel_button_cb, ad);
	MP_CHECK_NULL(popup);
	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_POP_CANCEL"), MP_POPUP_NO);

	return popup;
}



static void
_mp_popup_del_cb(void *data, Evas * e, Evas_Object * eo, void *event_info)
{
	DEBUG_TRACE("");
	struct appdata *ad = (struct appdata *)data;
	int type = (int)evas_object_data_get(eo, "type");
	DEBUG_TRACE("type: %d", type);
	if (type >= MP_POPUP_MAX)
	{
		ERROR_TRACE("Never should be here!!!");
		return;
	}
	ad->popup[type] = NULL;
}

static bool
_mp_popup_popup_exist(struct appdata *ad, mp_popup_t type)
{
	MP_CHECK_FALSE(ad);
	if (ad->popup[type])
		return TRUE;
	return FALSE;
}

Elm_Object_Item *
mp_genlist_popup_item_append(Evas_Object * popup, char *label, Evas_Object * icon, void *cb, void *data)
{
	MP_CHECK_NULL(popup);
	MP_CHECK_NULL(label);

	Evas_Object *genlist = evas_object_data_get(popup, "genlist");
	MP_CHECK_NULL(genlist);

	Elm_Object_Item *item = NULL;

	if (!icon)
	{
		itc.item_style = "1text";
		itc.func.text_get = _mp_popup_gl_label_get;
		itc.func.content_get = NULL;
		itc.func.state_get = NULL;
		itc.func.del = NULL;
	}
	else
	{
		itc.item_style = "1text.1icon.3";
		itc.func.text_get = _mp_popup_gl_label_get;
		itc.func.content_get = _mp_popup_gl_icon_get;
		itc.func.state_get = NULL;
		itc.func.del = NULL;
	}

	item = elm_genlist_item_append(genlist, &itc, label, NULL, ELM_GENLIST_ITEM_NONE, cb, data);

	return item;

}

Evas_Object *
mp_genlist_popup_create(Evas_Object * parent, mp_popup_t type, void *user_data, struct appdata * ad)
{
	mp_retvm_if(parent == NULL, NULL, "parent is NULL");
	MP_CHECK_NULL(ad);

	if (_mp_popup_popup_exist(ad, MP_POPUP_GENLIST))
	{
		DEBUG_TRACE("popup already exist...");
		return NULL;
	}

	Evas_Object *popup = NULL;

	switch (type)
	{
	case MP_POPUP_PV_SET_AS:
		popup = _mp_popup_pv_set_as_create(parent, user_data, false, ad);
		break;
	case MP_POPUP_PV_SET_AS_INCLUDE_ADD_TO_HOME:
		popup = _mp_popup_pv_set_as_create(parent, user_data, false, ad);
		break;
	case MP_POPUP_PV_SHARE:
		popup = _mp_popup_pv_share_create(parent, user_data, ad);
		break;
	case MP_POPUP_LIST_SHARE:
		popup = _mp_popup_list_share_create(parent, user_data, ad);
		break;
	case MP_POPUP_ADD_TO_PLST:
		popup = _mp_popup_add_to_playlist_create(parent, user_data, ad);
		break;
	case MP_POPUP_SOUND_PATH:
		popup = _mp_popup_sound_path_create(parent, user_data, ad);
		evas_object_data_set(popup, "sound_path", (char *)1);
		break;

	default:
		break;
	}

	if (popup)
	{
		evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _mp_popup_del_cb, ad);
		evas_object_data_set(popup, "type", (void *)MP_POPUP_GENLIST);
		ad->popup[MP_POPUP_GENLIST] = popup;
	}

	return popup;
}

static Eina_Bool
_mp_popup_genlist_popup_del_idler(void *data)
{
	Evas_Object *genlist_popup = data;
	MP_CHECK_VAL(genlist_popup, ECORE_CALLBACK_CANCEL);

	struct appdata *ad = evas_object_data_get(genlist_popup, "appdata");
	evas_object_del(genlist_popup);

	if (ad)
		ad->popup_del_idler = NULL;

	return ECORE_CALLBACK_DONE;
}

void
mp_popup_destroy(struct appdata *ad)
{
	MP_CHECK(ad);
	int i = 0;
	for(i=0; i < MP_POPUP_MAX; i++)
	{
		if (ad->popup[i] && i != MP_POPUP_NOTIFY)
		{
			if (i == MP_POPUP_GENLIST) {
				/* do NOT destroy genlist in genlst select callback function */
				evas_object_data_set(ad->popup[i], "appdata", ad);
				evas_object_hide(ad->popup[i]);
				ad->popup_del_idler = ecore_idler_add(_mp_popup_genlist_popup_del_idler, ad->popup[i]);
			} else {
				mp_evas_object_del(ad->popup[i]);
			}
			ad->popup[i] = NULL;
		}
	}
}

static void
_mp_popup_destroy_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Popup_Data *popup_data = data;
	MP_CHECK(popup_data);

	if (popup_data->type < MP_POPUP_MAX && popup_data->ad)
	{
		popup_data->ad->popup[popup_data->type] = NULL;
	}
	mp_popup_set_popup_data(obj, NULL);

	SAFE_FREE(popup_data);
}


Evas_Object *
mp_popup_create(Evas_Object * parent, mp_popup_type type, char *title, void *user_data, Evas_Smart_Cb response_cb,
		struct appdata *ad)
{
	Evas_Object *popup = NULL;
	Evas_Object *progressbar = NULL;

	MP_CHECK_NULL(parent);
	MP_CHECK_NULL(ad);

	if (_mp_popup_popup_exist(ad, type))
	{
		DEBUG_TRACE("popup already exist...");
		return NULL;
	}

	popup = elm_popup_add(ad->win_main);
	MP_CHECK_NULL(popup);

	Popup_Data *popup_data = (Popup_Data *)calloc(1, sizeof(Popup_Data));
	mp_assert(popup_data);
	ERROR_TRACE("popup_data: %x", popup_data);

	popup_data->ad = ad;
	popup_data->type = type;
	popup_data->response_cb = response_cb;
	popup_data->cb_data = user_data;
	mp_popup_set_popup_data(popup, popup_data);

	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _mp_popup_destroy_cb, popup_data);

	if (title)
		elm_object_part_text_set(popup, "title,text", title);

	switch (type)
	{
	case MP_POPUP_NORMAL:
		DEBUG_TRACE("MP_POPUP_NORMAL");
		break;

	case MP_POPUP_GENLIST:
		DEBUG_TRACE("MP_POPUP_GENLIST");
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_object_style_set(popup, "min_menustyle");
		break;

	case MP_POPUP_PROGRESS:
		progressbar = elm_progressbar_add(popup);
		MP_CHECK_NULL(progressbar);

		elm_object_style_set(progressbar, "list_process");
		evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, 0.5);
		evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(progressbar);
		elm_progressbar_pulse(progressbar, EINA_TRUE);

		elm_object_content_set(popup, progressbar);

		mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_POP_CANCEL"), MP_POPUP_NO);
		break;

	case MP_POPUP_NOTIFY:
		DEBUG_TRACE("MP_POPUP_NOTIFY");
		break;

	default:
		DEBUG_TRACE("Unsupported type: %d", type);
	}

	evas_object_show(popup);

	ad->popup[type] = popup;

	return popup;

}

void
mp_popup_response(Evas_Object *popup, int response)
{
	startfunc;
	MP_CHECK(popup);

	Popup_Data *popup_data = mp_popup_get_popup_data(popup);
	MP_CHECK(popup_data);

	if (popup_data->response_cb)
		popup_data->response_cb(popup_data->cb_data, popup, (void *)response);
	else
		mp_evas_object_del(popup);
}


static void
_mp_popup_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	Evas_Object *popup = data;
	MP_CHECK(popup);

	Popup_Data *popup_data = mp_popup_get_popup_data(popup);
	MP_CHECK(popup_data);

	int response = mp_evas_object_response_get(obj);
	mp_popup_response(popup, response);
}

bool
mp_popup_button_set(Evas_Object *popup, popup_button_t btn_index, const char *text, int response)
{
	MP_CHECK_FALSE(popup);
	MP_CHECK_FALSE(text);
	if (btn_index == MP_POPUP_BTN_MAX) {
		mp_error("invalid button type");
		return FALSE;
	}

	bool ret = FALSE;

	static char *part[MP_POPUP_BTN_MAX] = {
		"button1",
		"button2",
		"button3",
	};

	Evas_Object *button = mp_widget_create_button(popup, "popup_button/default", (char *)text, NULL, _mp_popup_button_clicked_cb, popup);
	if (button) {
		elm_object_part_content_set(popup, part[btn_index], button);
		mp_evas_object_response_set(button, response);
		ret = TRUE;
	}

	return ret;
}

static void
__mp_popup_timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	int response = (int)data;
	mp_popup_response(obj, response);
}

void
mp_popup_timeout_set(Evas_Object *popup, double timeout)
{
	startfunc;
	MP_CHECK(popup);

	elm_popup_timeout_set(popup, timeout);
	evas_object_smart_callback_add(popup, "timeout", __mp_popup_timeout_cb, (void *)MP_POPUP_NO);
	evas_object_smart_callback_add(popup, "block,clicked", __mp_popup_timeout_cb, (void *)MP_POPUP_NO);
}


bool
mp_popup_tickernoti_show(const char *text, bool info_style, bool bottom)
{
	MP_CHECK_FALSE(text);

	/*************************************************************************************
	 *	bundle infomaiton
	 *	Key	Value				Description
	 *	0	"default", "info" 	Tickernoti style, (default: "default")
	 *	1	"text"				Text to use for the tickernoti description
	 *	2	"0", "1"			Orientation of tickernoti. (0: top, 1:bottom, default: top)
	 *	3	"1", "2", ¡¦ ,"999"	Timeout (1: 1 second, default: -1 means infinite)
	 *************************************************************************************/

	bundle *b = NULL;

	b = bundle_create();
	MP_CHECK_FALSE(b);
	if (info_style)
		bundle_add(b, "0", "info");
	bundle_add(b, "1", text);
	if (bottom)
		bundle_add(b, "2", "1");  // bottom orientation
	bundle_add(b, "3", "2");  // terminated after 2 seconds

	int ret = syspopup_launch("tickernoti-syspopup", b);
	bundle_free(b);

	if (ret == 0)
		mp_debug("# tickernoti [%s] #", text);
	else {
		mp_error("syspopup_launch().. [0x%0x]", ret);
		return false;
	}

	return true;
}

