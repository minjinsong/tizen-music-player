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

#include "music.h"
#include "mp-search.h"
#include "mp-player-debug.h"
#include "mp-view-layout.h"

#define MP_SEARCHBAR_W 400*elm_config_scale_get()

void
_mp_search_view_activated_cb(void *data, Evas_Object * obj, void *event_info)
{
	MP_CHECK(data);
	mp_search_hide_imf_pannel(data);
}

static void
_mp_search_entry_changed_cb(void *data, Evas_Object * obj, void *event_info)
{
	Evas_Object *searchbar = data;
	MP_CHECK(searchbar);
	Evas_Object *entry = obj;
	MP_CHECK(entry);

	const char *signal = NULL;
	if (elm_entry_is_empty(entry))
		signal = "elm,state,eraser,hide";
	else
		signal = "elm,state,eraser,show";

	elm_object_signal_emit(searchbar, signal, "elm");
}

static void
_mp_search_eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *entry = data;
	MP_CHECK(entry);

	elm_entry_entry_set(entry, "");
}


Evas_Object *
mp_search_create_new(Evas_Object * parent, Evas_Smart_Cb change_cb, void *change_cb_data, Evas_Smart_Cb cancel_cb, void *cancel_cb_data)
{
	startfunc;
	Evas_Object *sb = NULL;
	Evas_Object *en = NULL;

	sb = elm_layout_add(parent);
	MP_CHECK_NULL(sb);

	const char *style = (cancel_cb) ? "cancel_button" : "default";
	elm_layout_theme_set(sb, "layout", "searchbar", style);

	if (cancel_cb) {
		Evas_Object *cancel_btn = elm_button_add(sb);
		elm_object_style_set(cancel_btn, "searchbar/default");
		elm_object_text_set(cancel_btn, GET_SYS_STR("IDS_COM_SK_CANCEL"));
		mp_language_mgr_register_object(cancel_btn, OBJ_TYPE_ELM_OBJECT, NULL, "IDS_COM_SK_CANCEL");
		evas_object_smart_callback_add(cancel_btn, "clicked", cancel_cb, cancel_cb_data);

		elm_object_part_content_set(sb, "button_cancel", cancel_btn);
		elm_object_signal_emit(sb, "cancel,show", "");
	}

	en = elm_entry_add(sb);
	elm_entry_scrollable_set(en, EINA_TRUE);
	elm_entry_single_line_set(en, EINA_TRUE);
	elm_entry_cnp_mode_set(en, ELM_CNP_MODE_PLAINTEXT);
	elm_object_part_content_set(sb, "elm.swallow.content", en);
	evas_object_data_set(sb, "entry", en);

	elm_entry_input_panel_layout_set(en, ELM_INPUT_PANEL_LAYOUT_NORMAL);

	evas_object_size_hint_weight_set(sb, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_align_set(sb, EVAS_HINT_FILL, 0.0);

	evas_object_smart_callback_add(en, "changed", _mp_search_entry_changed_cb, sb);
	elm_object_signal_callback_add(sb, "elm,eraser,clicked", "elm", _mp_search_eraser_clicked_cb, en);

	evas_object_smart_callback_add(en, "changed", change_cb, change_cb_data);
	evas_object_smart_callback_add(en, "activated", _mp_search_view_activated_cb, sb);

	static Elm_Entry_Filter_Limit_Size limit_filter_data;
	limit_filter_data.max_char_count = 0;
	limit_filter_data.max_byte_count = MP_METADATA_LEN_MAX;
	elm_entry_markup_filter_append(en, elm_entry_filter_limit_size, &limit_filter_data);

	evas_object_show(sb);

	return sb;
}

void
mp_search_hide_imf_pannel(Evas_Object * search)
{
	MP_CHECK(search);
	Evas_Object *en = mp_search_entry_get(search);
	Ecore_IMF_Context *imf_context = elm_entry_imf_context_get(en);

	if (imf_context)
	{
		ecore_imf_context_input_panel_hide(imf_context);
	}
}

void
mp_search_show_imf_pannel(Evas_Object * search)
{
	MP_CHECK(search);
	Evas_Object *en = mp_search_entry_get(search);
	Ecore_IMF_Context *imf_context = elm_entry_imf_context_get(en);

	if (imf_context)
	{
		ecore_imf_context_input_panel_show(imf_context);
	}
}

Evas_Object *
mp_search_entry_get(Evas_Object *search)
{
	MP_CHECK_NULL(search);

	Evas_Object *entry = evas_object_data_get(search, "entry");

	return entry;
}

const char *
mp_search_text_get(Evas_Object *search)
{
	MP_CHECK_NULL(search);
	Evas_Object *entry = evas_object_data_get(search, "entry");
	MP_CHECK_NULL(entry);

	const char *text = elm_object_text_get(entry);
	return text;
}

void
mp_search_text_set(Evas_Object *search, const char *text)
{
	MP_CHECK(search);
	Evas_Object *entry = evas_object_data_get(search, "entry");
	MP_CHECK(entry);

	if (text == NULL)
		text = "";

	elm_object_text_set(entry, text);
}

