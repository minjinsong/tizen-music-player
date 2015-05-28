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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#include <Elementary.h>
#include <ui-gadget-module.h>
#include <vconf.h>
#include <stdbool.h>
#include <glib.h>

#include "ug-mp-efl.h"
#include "ug-mp-library-view.h"
#include "mp-media-info.h"

static void
_parse_service(struct ug_data *ugd, service_h service)
{
	char *operation = NULL;
	service_get_operation(service, &operation);
	DEBUG_TRACE("operation: %s", operation);

	if(!g_strcmp0(operation, UG_MP_OPERATION_PICK))
	{
		char *value = NULL;
		service_get_extra_data(service, UG_MP_SELECT_MODE_KEY, &value);
		if(!g_strcmp0(value, UG_MP_SELECT_MULTIPLE))
		{
			ugd->select_type = UG_MP_SELECT_MULTI;
			goto END;
		}
		IF_FREE(value);
	}

	END:
	IF_FREE(operation);

}

static Evas_Object *
_ug_mp_create_fullview(Evas_Object * parent, struct ug_data *ugd)
{
	Evas_Object *base_layout;

	base_layout = elm_layout_add(parent);

	mp_retv_if(base_layout == NULL, NULL);

	elm_layout_theme_set(base_layout, "layout", "application", "default");

	return base_layout;
}

static Evas_Object *
_ug_mp_create_navigation_layout(Evas_Object * parent)
{
	Evas_Object *navi_bar;

	mp_retv_if(parent == NULL, NULL);

	navi_bar = elm_naviframe_add(parent);
	evas_object_show(navi_bar);

	return navi_bar;
}

static Evas_Object *
_ug_mp_crete_bg(Evas_Object *parent)
{
    Evas_Object *bg = elm_bg_add(parent);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_style_set(bg, "group_list");
    evas_object_show(bg);
    return bg;
}

static void
ug_mp_on_key_event(ui_gadget_h ug, enum ug_key_event event, service_h service, void *priv)
{
	if (!ug)
		return;

	struct ug_data *ugd = (struct ug_data *)priv;
	if (!ugd)
	{
		DEBUG_TRACE("ugd is null...");
		return;
	}
	switch (event)
	{
	case UG_KEY_EVENT_END:
		ug_destroy_me(ug);
		break;
	default:
		break;
	}
}

static void *
ug_mp_create(ui_gadget_h ug, enum ug_mode mode, service_h service, void *priv)
{
	DEBUG_TRACE("");

	Evas_Object *parent = NULL;
	struct ug_data *ugd = NULL;

	ugd = (struct ug_data *)priv;
	ugd->ug = ug;
	mp_retvm_if((!ug || !priv), NULL, "handle or ui_gadget pointer is NULL, check it !!");

	mp_media_info_connect();

	_parse_service(ugd, service);

	bindtextdomain(DOMAIN_NAME, LOCALE_DIR);

	parent = (Evas_Object *) ug_get_parent_layout(ug);
	mp_retvm_if(parent == NULL, NULL, "parent layout is NULL");

	elm_win_conformant_set(parent, EINA_TRUE);

	if (mode == UG_MODE_FULLVIEW)
	{
		DEBUG_TRACE("UG_MODE_FULLVIEW");

		ugd->base_layout = _ug_mp_create_fullview(parent, ugd);

		if (ugd->base_layout)
		{
			Evas_Object *bg = _ug_mp_crete_bg(ugd->base_layout);
			mp_retvm_if(bg == NULL, NULL, "bg layout is NULL");
			elm_win_resize_object_add(parent, bg);

			elm_object_part_content_set(ugd->base_layout, "elm.swallow.bg", bg);
			ugd->navi_bar = _ug_mp_create_navigation_layout(ugd->base_layout);
			elm_object_part_content_set(ugd->base_layout, "elm.swallow.content", ugd->navi_bar);
		}
		ug_mp_library_view_create(ugd);
	}
	else
		ERROR_TRACE("Not supported ug mode : %d", mode);

	return ugd->base_layout;
}

static void
ug_mp_start(ui_gadget_h ug, service_h service, void *priv)
{
	DEBUG_TRACE("");
}

static void
ug_mp_pause(ui_gadget_h ug, service_h service, void *priv)
{
	DEBUG_TRACE("");
}

static void
ug_mp_resume(ui_gadget_h ug, service_h service, void *priv)
{
	DEBUG_TRACE("");
}

static void
ug_mp_destroy(ui_gadget_h ug, service_h service, void *priv)
{
	DEBUG_TRACE("");
	struct ug_data *ugd;

	mp_media_info_disconnect();

	if (!ug || !priv)
		return;

	ugd = priv;

	if (ugd->base_layout)
	{
		evas_object_del(ugd->base_layout);
		ugd->base_layout = NULL;
	}
}

static void
ug_mp_message(ui_gadget_h ug, service_h msg, service_h service, void *priv)
{
	DEBUG_TRACE("");
}

static void
ug_mp_event(ui_gadget_h ug, enum ug_event event, service_h service, void *priv)
{
	DEBUG_TRACE("event: %d", event);
	switch (event)
	{
	case UG_EVENT_LOW_MEMORY:
		break;
	case UG_EVENT_LOW_BATTERY:
		break;
	case UG_EVENT_LANG_CHANGE:
		break;
	case UG_EVENT_ROTATE_PORTRAIT:
		break;
	case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
		break;
	case UG_EVENT_ROTATE_LANDSCAPE:
		break;
	case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		break;
	default:
		break;
	}
}

UG_MODULE_API int
UG_MODULE_INIT(struct ug_module_ops *ops)
{
	DEBUG_TRACE("");
	struct ug_data *ugd;

	if (!ops)
		return -1;

	ugd = calloc(1, sizeof(struct ug_data));
	if (!ugd)
		return -1;

	ops->create = ug_mp_create;
	ops->start = ug_mp_start;
	ops->pause = ug_mp_pause;
	ops->resume = ug_mp_resume;
	ops->destroy = ug_mp_destroy;
	ops->message = ug_mp_message;
	ops->key_event = ug_mp_on_key_event;
	ops->event = ug_mp_event;
	ops->priv = ugd;
	ops->opt = UG_OPT_INDICATOR_PORTRAIT_ONLY;

	return 0;
}

UG_MODULE_API void
UG_MODULE_EXIT(struct ug_module_ops *ops)
{
	DEBUG_TRACE("");
	struct ug_data *ugd;

	if (!ops)
		return;

	ugd = ops->priv;

	if (ugd)
		free(ugd);
}

