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

#include <libintl.h>

#include "mp-language-mgr.h"
#include "glib.h"
#include "mp-player-debug.h"

typedef struct
{
	Evas_Object *obj;
	obj_type type;
	const char *text_id;
	const char *part;
}obj_data;

typedef struct
{
	Elm_Object_Item *obj_item;
	const char *text_id;
}obj_item_data;

typedef struct _lang_mgr *lang_mgr;
struct  _lang_mgr{
	GList *objs;
	GList *obj_items;
	GList *glist_items;
	GList *grid_items;
}_lang_mgr;


static lang_mgr g_lang_mgr;

#define G_LIST_FOREACH(list, l, data) \
  for (l = list,                         \
       data = g_list_nth_data(l, 0);     \
       l;                                \
       l = g_list_next(l),            \
       data = g_list_nth_data(l, 0))

static void __glist_free(void *data)
{
	obj_data *item = data;
	MP_CHECK(item);
	free(item);
}

static char *__get_text(const char *ID)
{
	MP_CHECK_NULL(ID);
	char *str;

	if(strstr(ID, "IDS_COM"))
		str = dgettext("sys_string", ID);
	else
		str = gettext(ID);

	return str;
}

static void __update_obj(void *data, void *userdata)
{
	char *text;
	obj_data *item = data;
	MP_CHECK(item);
	DEBUG_TRACE("handle: 0x%x, ID:%s",item->obj, item->text_id);

	text = __get_text(item->text_id);

	if(item->type == OBJ_TYPE_ELM_OBJECT)
		elm_object_text_set(item->obj, text);
	else if(item->type == OBJ_TYPE_EDJE_OBJECT)
		edje_object_part_text_set(elm_layout_edje_get(item->obj), item->part, text);
	else
		WARN_TRACE("Unhandled case");
}

static void __update_obj_item(void *data, void *userdata)
{
	char *text;
	obj_item_data *item_data = data;
	MP_CHECK(item_data);
	DEBUG_TRACE("handle: 0x%x, ID:%s",item_data->obj_item, item_data->text_id);
	text = __get_text(item_data->text_id);
	elm_object_item_text_set(item_data->obj_item, text);
}

static void __update_list(void *data, void *userdata)
{
	Elm_Object_Item *item = data;
	MP_CHECK(item);
	DEBUG_TRACE("handle: 0x%x",item);
	elm_genlist_item_update(item);
}

static void __update_grid(void *data, void *userdata)
{
	Elm_Object_Item *item = data;
	MP_CHECK(item);
	DEBUG_TRACE("handle: 0x%x",item);
	elm_gengrid_item_update(item);
}

static void __obj_del_cb(void *data, Evas * e, Evas_Object * eo, void *event_info)
{
	obj_data *item = data;
	MP_CHECK(g_lang_mgr);
	MP_CHECK(item);

	g_lang_mgr->objs =
		g_list_delete_link(g_lang_mgr->objs, g_list_find(g_lang_mgr->objs, item));

	free(item);
}

int mp_language_mgr_create(void)
{
	DEBUG_TRACE("");
	if(!g_lang_mgr)
	{
		lang_mgr mgr = calloc(1, sizeof(_lang_mgr));
		if(!mgr)
		{
			WARN_TRACE("Error: calloc");
			return -1;
		}
		g_lang_mgr = mgr;
	}
	return 0;
}

int mp_language_mgr_destroy(void)
{
	MP_CHECK_VAL(g_lang_mgr, -1);
	g_list_free_full(g_lang_mgr->objs, __glist_free);
	g_lang_mgr->objs = NULL;

	g_list_free(g_lang_mgr->glist_items);
	g_lang_mgr->glist_items = NULL;

	g_list_free(g_lang_mgr->grid_items);
	g_lang_mgr->grid_items = NULL;

	free(g_lang_mgr);
	g_lang_mgr = NULL;

	return 0;
}

void mp_language_mgr_register_object(Evas_Object *obj, obj_type type, const char *part, const char *text_id)
{
	MP_CHECK(g_lang_mgr);
	obj_data *item = calloc(1, sizeof(obj_data));
	MP_CHECK(item);

	item->type = type;
	item->part = part;
	item->text_id = text_id;
	item->obj = obj;

	evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, __obj_del_cb, item);

	g_lang_mgr->objs = g_list_append(g_lang_mgr->objs, item);
}

void mp_language_mgr_register_object_item(Elm_Object_Item *object_item, const char *text_ID)
{
	MP_CHECK(g_lang_mgr);
	obj_item_data *data = calloc(1, sizeof(obj_item_data));
	MP_CHECK(data);

	data->obj_item = object_item;
	data->text_id = text_ID;

	g_lang_mgr->obj_items= g_list_append(g_lang_mgr->obj_items, data);
}

void mp_language_mgr_unregister_object_item(Elm_Object_Item *object_item)
{
	GList *l;
	obj_item_data *data;

	G_LIST_FOREACH(g_lang_mgr->obj_items, l, data)
	{
		if(data && data->obj_item == object_item)
		{
			g_lang_mgr->obj_items = g_list_delete_link(g_lang_mgr->obj_items, l);
			if(data)
				free(data);
			break;
		}
	}
}

void mp_language_mgr_object_item_text_ID_set(Elm_Object_Item *object_item, const char *text_ID)
{
	MP_CHECK(g_lang_mgr);
	GList *l;
	obj_item_data *data;

	G_LIST_FOREACH(g_lang_mgr->obj_items, l, data)
	{
		if(data->obj_item == object_item)
		{
			DEBUG_TRACE("set ID: %s", text_ID);
			data->text_id = text_ID;
			break;
		}
	}

}

void mp_language_mgr_register_genlist_item(Elm_Object_Item *item)
{
	MP_CHECK(g_lang_mgr);
	g_lang_mgr->glist_items =
		g_list_append(g_lang_mgr->glist_items, item);
}

void mp_language_mgr_unregister_genlist_item(Elm_Object_Item *item)
{
	MP_CHECK(g_lang_mgr);
	g_lang_mgr->glist_items =
		g_list_delete_link(g_lang_mgr->glist_items, g_list_find(g_lang_mgr->glist_items, item));
}

void mp_language_mgr_register_gengrid_item(Elm_Object_Item *item)
{
	MP_CHECK(g_lang_mgr);
	g_lang_mgr->grid_items =
		g_list_append(g_lang_mgr->grid_items, item);
}

void mp_language_mgr_unregister_gengrid_item(Elm_Object_Item *item)
{
	MP_CHECK(g_lang_mgr);
	g_lang_mgr->grid_items =
		g_list_delete_link(g_lang_mgr->grid_items, g_list_find(g_lang_mgr->grid_items, item));
}

void mp_language_mgr_update()
{
	DEBUG_TRACE("language changed. update text");
	MP_CHECK(g_lang_mgr);
	g_list_foreach(g_lang_mgr->objs, __update_obj, NULL);
	g_list_foreach(g_lang_mgr->obj_items, __update_obj_item, NULL);
	g_list_foreach(g_lang_mgr->glist_items, __update_list, NULL);
	g_list_foreach(g_lang_mgr->grid_items, __update_grid, NULL);
}
