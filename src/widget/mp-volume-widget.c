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

#include "mp-volume-widget.h"
#include "mp-player-debug.h"
#include "mp-define.h"
#include "mp-volume.h"

typedef struct {
	bool dragging;

	Mp_Volume_Widget_Cb event_cb;
	void *user_data;

} Volume_Widget_Data;

static void
_mp_volume_widget_drag_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	Volume_Widget_Data *wd = data;
	MP_CHECK(wd);

	if (wd->event_cb)
		wd->event_cb(wd->user_data, obj, VOLUME_WIDGET_EVENT_DRAG_START);
}

static void
_mp_volume_widget_drag_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	Volume_Widget_Data *wd = data;
	MP_CHECK(wd);

	if (wd->event_cb)
		wd->event_cb(wd->user_data, obj, VOLUME_WIDGET_EVENT_DRAG_STOP);
}

static void
_mp_volume_widget_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	double val = elm_slider_value_get(obj);
	int incator_val = (int)(val + 0.5);
	if (incator_val != mp_volume_get_current())
		mp_volume_set(incator_val);
}

static void
_mp_volume_widget_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Volume_Widget_Data *wd = data;
	SAFE_FREE(wd);
}


Evas_Object *
mp_volume_widget_add(Evas_Object *parent)
{
	MP_CHECK_NULL(parent);

	Evas_Object *slider = elm_slider_add(parent);
	elm_object_style_set(slider, "music-player/default");
	elm_slider_indicator_show_set(slider, EINA_TRUE);
	evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_slider_indicator_format_set(slider, "%1.0f");

	Evas_Object *icon = elm_icon_add(slider);
	elm_image_file_set(icon, IMAGE_EDJ_NAME, MP_ICON_VOLUME_MIN);
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	Evas_Object *icon_end = elm_icon_add(slider);
	elm_image_file_set(icon_end, IMAGE_EDJ_NAME, MP_ICON_VOLUME_MAX);
	evas_object_size_hint_aspect_set(icon_end, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	elm_object_content_set(slider,icon);
	elm_object_part_content_set(slider, "end", icon_end);

	elm_slider_min_max_set(slider, 0, (double)mp_volume_get_max());
	elm_slider_value_set(slider, (double)mp_volume_get_current());

	Volume_Widget_Data *wd = calloc(1, sizeof(Volume_Widget_Data));
	mp_assert(wd);
	evas_object_data_set(slider, "widget_data", wd);

	evas_object_smart_callback_add(slider, "slider,drag,start", _mp_volume_widget_drag_start_cb, wd);
	evas_object_smart_callback_add(slider, "changed", _mp_volume_widget_changed_cb, wd);
	evas_object_smart_callback_add(slider, "slider,drag,stop", _mp_volume_widget_drag_stop_cb, wd);

	evas_object_event_callback_add(slider, EVAS_CALLBACK_DEL, _mp_volume_widget_del_cb, wd);

	return slider;
}

void
mp_volume_widget_event_callback_add(Evas_Object *obj, Mp_Volume_Widget_Cb event_cb, void *user_data)
{
	MP_CHECK(obj);
	Volume_Widget_Data *wd = evas_object_data_get(obj, "widget_data");
	MP_CHECK(wd);

	wd->event_cb = event_cb;
	wd->user_data = user_data;
}

void
mp_volume_widget_volume_up(Evas_Object *obj)
{
	double max = 0.0;
	elm_slider_min_max_get(obj, NULL, &max);

	double current = elm_slider_value_get(obj);

	if (current < max)
		elm_slider_value_set(obj, (current + 1.0));

	evas_object_smart_callback_call(obj, "changed", NULL);
}

void
mp_volume_widget_volume_down(Evas_Object *obj)
{
	double min = 0.0;
	elm_slider_min_max_get(obj, &min, NULL);

	double current = elm_slider_value_get(obj);

	if (current > min)
		elm_slider_value_set(obj, (current - 1.0));

	evas_object_smart_callback_call(obj, "changed", NULL);
}

