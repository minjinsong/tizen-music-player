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

#include "mp-ctxpopup.h"
#include <syspopup_caller.h>
#include <bundle.h>
#include <stdio.h>
#include "music.h"
#include "mp-menu.h"
#include "mp-item.h"
#include "mp-player-debug.h"
#include "mp-view-layout.h"
#include "mp-playlist-mgr.h"
#include "mp-common.h"
#include <sound_manager.h>
#include "mp-util.h"

#include "mp-widget.h"

static void _mp_ctxpopup_item_del_cb(void *data, Evas_Object *obj, void *event_info)
{
	DEBUG_TRACE("");

	IF_FREE(data);
}

static void _dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	DEBUG_TRACE("");

	MP_CHECK(data);

	evas_object_del(data);
	evas_object_smart_callback_del(data,"dismissed", _dismissed_cb);
}

static void _move_ctxpopup(Evas_Object *ctxpopup, Evas_Object *btn)
{
	DEBUG_TRACE("");

	Evas_Coord x, y, w , h;
	evas_object_geometry_get(btn, &x, &y, &w, &h);
    evas_object_move(ctxpopup, x + (w / 2), y + (h /2));
}

Evas_Object *
_mp_ctxpopup_pv_set_as_create(Evas_Object *parent, void *user_data, struct appdata *ad)
{
	DEBUG_TRACE("");

	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(parent);

	Evas_Object *popup = elm_ctxpopup_add(ad->naviframe);
	evas_object_smart_callback_add(popup,"dismissed", _dismissed_cb, popup);
	elm_ctxpopup_direction_priority_set(popup,
						     ELM_CTXPOPUP_DIRECTION_DOWN,
						     ELM_CTXPOPUP_DIRECTION_LEFT,
						     ELM_CTXPOPUP_DIRECTION_UP,
						     ELM_CTXPOPUP_DIRECTION_RIGHT );

	mp_ctxpopup_item_append(popup, GET_STR(CALL_RINGTONE), NULL, mp_menu_set_as_select_cb, user_data);
	mp_ctxpopup_item_append(popup, GET_STR(CALLER_RINGTONE), NULL, mp_menu_set_as_select_cb, user_data);

	_move_ctxpopup(popup, parent);
	evas_object_show(popup);

	return popup;
}

Evas_Object *
_mp_ctxpopup_pv_share_create(Evas_Object *parent, void *user_data, struct appdata *ad)
{
	DEBUG_TRACE("");

	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(parent);

	Evas_Object *popup = elm_ctxpopup_add(ad->naviframe);
	evas_object_smart_callback_add(popup,"dismissed", _dismissed_cb, popup);
	elm_ctxpopup_direction_priority_set(popup,
						     ELM_CTXPOPUP_DIRECTION_DOWN,
						     ELM_CTXPOPUP_DIRECTION_LEFT,
						     ELM_CTXPOPUP_DIRECTION_UP,
						     ELM_CTXPOPUP_DIRECTION_RIGHT );

	mp_ctxpopup_item_append(popup, BLUETOOTH_SYS, NULL, mp_menu_share_select_cb, user_data);
	mp_ctxpopup_item_append(popup, EMAIL_SYS, NULL, mp_menu_share_select_cb, user_data);
	mp_ctxpopup_item_append(popup, MESSAGE_SYS, NULL, mp_menu_share_select_cb, user_data);

#ifdef MP_FEATURE_WIFI_SHARE
	mp_ctxpopup_item_append(popup, WIFI_SYS, NULL, mp_menu_share_select_cb, user_data);
#endif

	_move_ctxpopup(popup, parent);
	evas_object_show(popup);

	return popup;
}

Evas_Object *
_mp_ctxpopup_list_share_create(Evas_Object *parent, void *user_data, struct appdata *ad)
{
	DEBUG_TRACE("");

	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(parent);

	Evas_Object *popup = elm_ctxpopup_add(ad->naviframe);
	evas_object_smart_callback_add(popup,"dismissed", _dismissed_cb, popup);
	elm_ctxpopup_direction_priority_set(popup,
						     ELM_CTXPOPUP_DIRECTION_DOWN,
						     ELM_CTXPOPUP_DIRECTION_LEFT,
						     ELM_CTXPOPUP_DIRECTION_UP,
						     ELM_CTXPOPUP_DIRECTION_RIGHT );

	mp_ctxpopup_item_append(popup, BLUETOOTH_SYS, NULL, mp_menu_share_list_select_cb, user_data);
	mp_ctxpopup_item_append(popup, EMAIL_SYS, NULL, mp_menu_share_list_select_cb, user_data);
	mp_ctxpopup_item_append(popup, MESSAGE_SYS, NULL, mp_menu_share_list_select_cb, user_data);

#ifdef MP_FEATURE_WIFI_SHARE
	mp_ctxpopup_item_append(popup, WIFI_SYS, NULL, mp_menu_share_list_select_cb, user_data);
#endif

	_move_ctxpopup(popup, parent);
	evas_object_show(popup);

	return popup;
}


void
mp_ctxpopup_destroy(Evas_Object * popup)
{
	DEBUG_TRACE("");

	MP_CHECK(popup);
	mp_evas_object_del(popup);
}

void
mp_ctxpopup_clear(Evas_Object * popup)
{
	DEBUG_TRACE("");

	MP_CHECK(popup);
	elm_ctxpopup_clear(popup);
}

Elm_Object_Item *
mp_ctxpopup_item_append(Evas_Object *popup, char *label, Evas_Object *icon, void *cb, void *data)
{
	DEBUG_TRACE("");

	MP_CHECK_NULL(popup);
	//MP_CHECK_NULL(label);

	Elm_Object_Item *item = NULL;

	CtxPopup_Data *popup_data = NULL;
	popup_data = (CtxPopup_Data *)calloc(1, sizeof(CtxPopup_Data));
	mp_assert(popup_data);
	ERROR_TRACE("popup_data: %x", popup_data);

	popup_data->popup = popup;
	popup_data->label = label;
	popup_data->user_data = data;

	item = elm_ctxpopup_item_append(popup, label, icon, cb, popup_data);
	elm_object_item_del_cb_set(item, _mp_ctxpopup_item_del_cb);

	return item;
}

Evas_Object *
mp_ctxpopup_create(Evas_Object *parent, int type, void *user_data, void *ad)
{
	DEBUG_TRACE("");

	MP_CHECK_NULL(parent);
	MP_CHECK_NULL(ad);

	Evas_Object *popup = NULL;

	switch (type)
	{
	case MP_CTXPOPUP_PV_SET_AS:
		popup = _mp_ctxpopup_pv_set_as_create(parent, user_data, ad);
		break;
	case MP_CTXPOPUP_PV_SET_AS_INCLUDE_ADD_TO_HOME:
		popup = _mp_ctxpopup_pv_set_as_create(parent, user_data, ad);
		break;
	case MP_CTXPOPUP_PV_SHARE:
		popup = _mp_ctxpopup_pv_share_create(parent, user_data, ad);
		break;
	case MP_CTXPOPUP_LIST_SHARE:
		popup = _mp_ctxpopup_list_share_create(parent, user_data, ad);
		break;
	default:
		break;
	}

	return popup;
}
