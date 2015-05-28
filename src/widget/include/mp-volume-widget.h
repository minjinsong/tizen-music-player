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

#ifndef __MP_VOLUME_WIDGET_H__
#define __MP_VOLUME_WIDGET_H__

#include <Elementary.h>

typedef enum {
	VOLUME_WIDGET_EVENT_DRAG_START,
	VOLUME_WIDGET_EVENT_DRAG_STOP,
} volume_widget_event_e;

typedef void (*Mp_Volume_Widget_Cb)(void *user_data, Evas_Object *obj, volume_widget_event_e event);

#ifdef __cplusplus
extern "C" {
#endif

Evas_Object *mp_volume_widget_add(Evas_Object *parent);
void mp_volume_widget_event_callback_add(Evas_Object *obj, Mp_Volume_Widget_Cb event_cb, void *user_data);
void mp_volume_widget_volume_up(Evas_Object *obj);
void mp_volume_widget_volume_down(Evas_Object *obj);


#ifdef __cplusplus
}
#endif


#endif /* __MP_VOLUME_WIDGET_H__ */

