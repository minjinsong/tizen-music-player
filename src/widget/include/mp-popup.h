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

#ifndef __MP_popup_H_
#define __MP_popup_H_

#include <Elementary.h>

typedef enum _mp_popup_t
{
	MP_POPUP_PV_SET_AS = 0,
	MP_POPUP_PV_SET_AS_INCLUDE_ADD_TO_HOME,
	MP_POPUP_PV_SHARE,
	MP_POPUP_LIST_SHARE,
	MP_POPUP_SOUND_PATH,
	MP_POPUP_ADD_TO_PLST,
} mp_popup_t;

typedef enum {
	MP_POPUP_BTN_1,
	MP_POPUP_BTN_2,
	MP_POPUP_BTN_3,
	MP_POPUP_BTN_MAX,
} popup_button_t;

Elm_Object_Item *mp_genlist_popup_item_append(Evas_Object * popup, char *label, Evas_Object * icon, void *cb,
					       void *data);
Evas_Object *mp_genlist_popup_create(Evas_Object * parent, mp_popup_t type, void *user_data, struct appdata *ad);
Evas_Object *mp_popup_create(Evas_Object * parent, mp_popup_type type, char *title, void *user_data, Evas_Smart_Cb response_cb,
			     struct appdata *ad);
void mp_popup_destroy(struct appdata *ad);
void mp_popup_response(Evas_Object *popup, int response);
bool mp_popup_button_set(Evas_Object *popup, popup_button_t btn_index, const char *text, int response);
void mp_popup_timeout_set(Evas_Object *popup, double timeout);
bool mp_popup_tickernoti_show(const char *text, bool info_style, bool bottom);

#endif // __MP_contextpopup_H_
