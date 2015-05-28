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

#include <Elementary.h>
#include "mp-smart-event-box.h"
#include "mp-player-debug.h"

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

typedef struct _MpSmartEventBoxObject
{
	Evas_Object *rect;
	Evas_Coord x, y, w, h;
	Evas_Coord down_x;
	Evas_Coord down_y;
} MpSmartEventBoxObject;

static Evas_Smart_Class _parent_sc = EVAS_SMART_CLASS_INIT_NULL;

static void
__mouse_down_cb(void *data, Evas * evas, Evas_Object * obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *) event_info;
	Evas_Object *box = (Evas_Object *) data;
	MpSmartEventBoxObject *box_d = evas_object_smart_data_get(box);
	MP_CHECK(box_d);

	box_d->down_x = ev->canvas.x;
	box_d->down_y = ev->canvas.y;

	evas_object_smart_callback_call((Evas_Object *) data, "mouse.down", NULL);
	return;
}

static void
__mouse_up_cb(void *data, Evas * evas, Evas_Object * obj, void *event_info)
{
	Evas_Coord minw = 0, minh = 0, diff_x = 0, diff_y = 0;
	Evas_Event_Mouse_Up *mu = (Evas_Event_Mouse_Up *) event_info;
	Evas_Object *box = (Evas_Object *) data;
	MpSmartEventBoxObject *box_d = evas_object_smart_data_get(box);
	MP_CHECK(box_d);

	elm_coords_finger_size_adjust(1, &minw, 1, &minh);

	diff_x = box_d->down_x - mu->canvas.x;
	diff_y = box_d->down_y - mu->canvas.y;

	if ((ABS(diff_x) > minw) || (ABS(diff_y) > minh))
	{			// dragging
		if (ABS(diff_y) > ABS(diff_x))
		{
			if (diff_y < 0)	//down
				goto flick_down;
			else	//up
				goto flick_up;
		}
		else
		{
			if (diff_x < 0)
			{	//right
				goto flick_right;
			}
			else
			{	//left
				goto flick_left;
			}
		}
	}

	evas_object_smart_callback_call((Evas_Object *) data, "mouse.clicked", NULL);
	return;

      flick_up:
	evas_object_smart_callback_call((Evas_Object *) data, "mouse.flick.up", NULL);
	return;

      flick_down:
	evas_object_smart_callback_call((Evas_Object *) data, "mouse.flick.down", NULL);
	return;

      flick_left:
	evas_object_smart_callback_call((Evas_Object *) data, "mouse.flick.left", NULL);
	return;

      flick_right:
	evas_object_smart_callback_call((Evas_Object *) data, "mouse.flick.right", NULL);
	return;
}

static void
_smart_reconfigure(MpSmartEventBoxObject * data)
{
	evas_object_move(data->rect, data->x, data->y);
	evas_object_resize(data->rect, data->w, data->h);
	return;
}


static void
_mp_smart_event_box_object_add(Evas_Object * obj)
{
	MpSmartEventBoxObject *data = NULL;

	data = (MpSmartEventBoxObject *) malloc(sizeof(MpSmartEventBoxObject));
	mp_assert(data);
	memset(data, 0, sizeof(MpSmartEventBoxObject));

	data->rect = evas_object_rectangle_add(evas_object_evas_get(obj));
	evas_object_size_hint_min_set(data->rect, 0, 15);
	evas_object_color_set(data->rect, 0, 0, 0, 0);
	evas_object_size_hint_fill_set(data->rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(data->rect, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(data->rect, EVAS_HINT_FILL, 0.0);

	evas_object_smart_member_add(data->rect, obj);

	evas_object_smart_data_set(obj, data);

	return;
}

static void
_mp_smart_event_box_object_del(Evas_Object * obj)
{
	MpSmartEventBoxObject *data = NULL;
	data = evas_object_smart_data_get(obj);
	MP_CHECK(data);
	if (data->rect != NULL)
	{
		evas_object_smart_member_del(data->rect);
		evas_object_del(data->rect);
		data->rect = NULL;
	}
	free(data);

	return;
}

static void
_mp_smart_event_box_object_show(Evas_Object * obj)
{
	MpSmartEventBoxObject *data = NULL;
	data = evas_object_smart_data_get(obj);
	MP_CHECK(data);
	if (data->rect != NULL)
	{
		evas_object_show(data->rect);
	}

	return;
}

static void
_mp_smart_event_box_object_hide(Evas_Object * obj)
{
	MpSmartEventBoxObject *data = NULL;
	data = evas_object_smart_data_get(obj);

	MP_CHECK(data);

	if (data->rect != NULL)
	{
		evas_object_hide(data->rect);
	}

	return;
}

static void
_mp_smart_event_box_object_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
	MpSmartEventBoxObject *data = NULL;
	data = evas_object_smart_data_get(obj);
	MP_CHECK(data);
	data->x = x;
	data->y = y;

	_smart_reconfigure(data);
}

static void
_mp_smart_event_box_object_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
	MpSmartEventBoxObject *data = NULL;
	data = evas_object_smart_data_get(obj);
	MP_CHECK(data);
	data->w = w;
	data->h = h;

	_smart_reconfigure(data);
}



static Evas_Smart *
_mp_smart_event_box_object_smart_get(void)
{
	static Evas_Smart_Class sc = EVAS_SMART_CLASS_INIT_NAME_VERSION("mp_smart_event_box_object");

	if (!_parent_sc.name)
	{
		evas_object_smart_clipped_smart_set(&sc);
		_parent_sc = sc;
		sc.add = _mp_smart_event_box_object_add;
		sc.del = _mp_smart_event_box_object_del;
		sc.show = _mp_smart_event_box_object_show;
		sc.hide = _mp_smart_event_box_object_hide;
		sc.move = _mp_smart_event_box_object_move;
		sc.resize = _mp_smart_event_box_object_resize;
	}

	return evas_smart_class_new(&sc);
}


static Evas_Object *
_mp_smart_event_box_object_new(Evas * e)
{
	Evas_Object *obj;
	obj = evas_object_smart_add(e, _mp_smart_event_box_object_smart_get());

	return obj;
}


Evas_Object *
mp_smart_event_box_add(Evas_Object * parent)
{
	Evas *e = NULL;
	Evas_Object *obj = NULL;
	MpSmartEventBoxObject *data = NULL;

	e = evas_object_evas_get(parent);
	if ((obj = _mp_smart_event_box_object_new(e)))
	{
		if ((data = evas_object_smart_data_get(obj)))
		{
			evas_object_event_callback_add(data->rect, EVAS_CALLBACK_MOUSE_DOWN, __mouse_down_cb, obj);
			evas_object_event_callback_add(data->rect, EVAS_CALLBACK_MOUSE_UP, __mouse_up_cb, obj);
		}
		else
		{
			evas_object_del(obj);
			obj = NULL;
		}
	}

	return obj;
}
