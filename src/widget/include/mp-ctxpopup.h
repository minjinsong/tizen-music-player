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

#ifndef __MP_ctxpopup_H_
#define __MP_ctxpopup_H_

#include <Elementary.h>

typedef enum _mp_ctxpopup_t
{
	MP_CTXPOPUP_PV_SET_AS = 0,
	MP_CTXPOPUP_PV_SET_AS_INCLUDE_ADD_TO_HOME,
	MP_CTXPOPUP_PV_SHARE,
	MP_CTXPOPUP_LIST_SHARE,
	MP_CTXPOPUP_CHANGE_AXIS,
} mp_ctxpopup_t;

typedef struct {
	Evas_Object *popup;
	char *label;
	void *user_data;
} CtxPopup_Data;

Elm_Object_Item *
mp_ctxpopup_item_append(Evas_Object *popup, char *label, Evas_Object *icon, void *cb, void *data);

Evas_Object *
mp_ctxpopup_create(Evas_Object *parent, int type, void *user_data, void *ad);
void mp_ctxpopup_clear(Evas_Object * popup);
void mp_ctxpopup_destroy(Evas_Object * popup);

#endif // __MP_ctxpopup_H_