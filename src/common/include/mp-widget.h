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

#ifndef __MP_WIDGET_H_
#define __MP_WIDGET_H_

#include "music.h"

#define _EDJ(obj) elm_layout_edje_get(obj) /**< get evas object from elm layout */

Evas_Object *mp_widget_navigation_new(Evas_Object * parent, struct appdata *ad);
Evas_Object *mp_widget_genlist_create(struct appdata *ad, Evas_Object * parent, bool homogeneous, bool sweep_flag);
bool mp_widget_check_lowbattery(void *data, Evas_Object * parent);
Evas_Object *mp_widget_text_popup(void *data, const char *message);
Evas_Object *mp_widget_create_icon(Evas_Object * obj, const char *path, int w, int h);
Evas_Object *mp_widget_create_bgimg(Evas_Object * parent);
Evas_Object *mp_widget_create_button(Evas_Object * parent, char *style, char *caption, Evas_Object * icon,
				     void (*func) (void *, Evas_Object *, void *), void *data);
Evas_Object * mp_widget_create_editfield(Evas_Object * parent, int limit_size, char *guide_txt, struct appdata *ad);
Evas_Object * mp_widget_editfield_entry_get(Evas_Object *editfield);
Evas_Object * mp_widget_create_title_btn(Evas_Object *parent, const char *text, const char * icon_path, Evas_Smart_Cb func, void *data);
Evas_Object * mp_common_create_naviframe_title_button(Evas_Object *parent, const char * text_id, void *save_cb, void *user_data);
Evas_Object * mp_widget_create_layout_main(Evas_Object * parent);
inline Evas_Object *mp_common_load_edj(Evas_Object * parent, const char *file, const char *group);
Evas_Object *mp_create_win(const char *name);

#define MP_TOOLBAR_BTN_DEFULTL	"naviframe/toolbar/default"
#define MP_TOOLBAR_BTN_LEFT	"naviframe/toolbar/left"
#define MP_TOOLBAR_BTN_RIGHT	"naviframe/toolbar/right"
#define MP_TOOLBAR_BTN_MORE	"naviframe/more/default"
Evas_Object *mp_widget_create_toolbar_btn(Evas_Object *parent, const char *style, const char *text, Evas_Smart_Cb func, void *data);

#endif
