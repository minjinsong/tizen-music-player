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

#include "mp-widget.h"
#include "mp-player-debug.h"
#include "mp-util.h"
#include "mp-popup.h"
#ifndef MP_SOUND_PLAYER
#include "mp-common.h"
#endif

#define MAX_LEN_VIB_DURATION 0.5

static void _mp_widget_genlist_flick_left_cb(void *data, Evas_Object * obj, void *event_info);
static void _mp_widget_genlist_flick_right_cb(void *data, Evas_Object * obj, void *event_info);
static void _mp_widget_genlist_flick_stop_cb(void *data, Evas_Object * obj, void *event_info);
static void _mp_widget_realize_genlist_cb(void *data, Evas_Object * obj, void *event_info);


static void
_mp_widget_genlist_flick_left_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	if(ad->vertical_scroll)
		return;
	elm_object_scroll_freeze_push(obj);
}

static void
_mp_widget_genlist_flick_right_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	if(ad->vertical_scroll)
		return;
	elm_object_scroll_freeze_push(obj);
}

static void
_mp_widget_genlist_flick_stop_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	ad->vertical_scroll = false;
	elm_object_scroll_freeze_pop(obj);
}

static void
_mp_widget_realize_genlist_cb(void *data, Evas_Object * obj, void *event_info)
{

}

Evas_Object *
mp_widget_navigation_new(Evas_Object * parent, struct appdata *ad)
{
	Evas_Object *navi_bar;
	mp_retv_if(parent == NULL, NULL);
	navi_bar = elm_naviframe_add(parent);
	mp_retvm_if(navi_bar == NULL, NULL, "Fail to create navigation bar");
#ifndef MP_SOUND_PLAYER
	evas_object_smart_callback_add(navi_bar, "transition,finished", mp_common_navigationbar_finish_effect, ad);
#endif
	evas_object_show(navi_bar);
	return navi_bar;
}

static void
_mp_widget_gl_mode_right(void *data, Evas_Object * obj, void *event_info)
{
	MP_CHECK(obj);
	MP_CHECK(event_info);

	/* reset old sweep item mode */
	Elm_Object_Item *it = (Elm_Object_Item *)elm_genlist_decorated_item_get(obj);
	if (it && (it != event_info)) {
		elm_genlist_item_decorate_mode_set(it, "slide", EINA_FALSE);
		elm_genlist_item_select_mode_set(it, ELM_OBJECT_SELECT_MODE_DEFAULT);
	}

	// disable sweep if edit mode.
	MP_CHECK(elm_genlist_decorate_mode_get(obj) == EINA_FALSE);
	MP_CHECK(elm_genlist_item_flip_get(event_info) == EINA_FALSE);
	// Start genlist sweep
	elm_genlist_item_decorate_mode_set(event_info, "slide", EINA_TRUE);
	elm_genlist_item_select_mode_set(event_info, ELM_OBJECT_SELECT_MODE_NONE);
}

static void
_mp_widget_gl_mode_left(void *data, Evas_Object * obj, void *event_info)
{
	MP_CHECK(obj);
	MP_CHECK(event_info);
	// disable sweep if edit mode.
	MP_CHECK(elm_genlist_decorate_mode_get(obj) == EINA_FALSE);
	MP_CHECK(elm_genlist_item_flip_get(event_info) == EINA_FALSE);
	// Finish genlist sweep
	elm_genlist_item_decorate_mode_set(event_info, "slide", EINA_FALSE);
	elm_genlist_item_select_mode_set(event_info, ELM_OBJECT_SELECT_MODE_DEFAULT);
}

static void
_mp_widget_gl_mode_cancel(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	MP_CHECK(obj);

	mp_util_reset_genlist_mode_item(obj);

	ad->vertical_scroll = true;
}

Evas_Object *
mp_widget_genlist_create(struct appdata *ad, Evas_Object * parent, bool homogeneous, bool sweep_flag)
{
	Evas_Object *list = NULL;

	list = elm_genlist_add(parent);
	CHECK(list);
	evas_object_data_set(list, "ap", ad);
	evas_object_show(list);
	elm_genlist_homogeneous_set(list, homogeneous);

	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(list, "realized", _mp_widget_realize_genlist_cb, ad);

	if (sweep_flag) {
		evas_object_smart_callback_add(list, "drag,start,left", _mp_widget_genlist_flick_left_cb, ad);
		evas_object_smart_callback_add(list, "drag,start,right", _mp_widget_genlist_flick_right_cb, ad);
		evas_object_smart_callback_add(list, "drag,stop", _mp_widget_genlist_flick_stop_cb, ad);

		evas_object_smart_callback_add(list, "drag,start,right", _mp_widget_gl_mode_right, NULL);
		evas_object_smart_callback_add(list, "drag,start,left", _mp_widget_gl_mode_left, NULL);
		evas_object_smart_callback_add(list, "drag,start,up", _mp_widget_gl_mode_cancel, ad);
		evas_object_smart_callback_add(list, "drag,start,down", _mp_widget_gl_mode_cancel, ad);
	}
	return list;
}

static void
_mp_widget_lowbattery_del_cb(void *data, Evas * e, Evas_Object * eo, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	ad->popup[MP_POPUP_NORMAL] = NULL;
}

static void
_mp_widget_lowbattery_res_cb(void *data, Evas_Object * obj, void *event_info)
{
	mp_evas_object_del(obj);
}

bool
mp_widget_check_lowbattery(void *data, Evas_Object * parent)
{
	struct appdata *ad = (struct appdata *)data;

	if (mp_check_battery_available())
	{
		Evas_Object *popup = mp_popup_create(ad->win_main, MP_POPUP_NORMAL, NULL, NULL, _mp_widget_lowbattery_res_cb, ad);
		elm_object_text_set(popup, GET_SYS_STR("IDS_COM_BODY_LOW_BATTERY"));
		mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_SK_OK"), 0);
		evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _mp_widget_lowbattery_del_cb, ad);
		evas_object_show(popup);

		return FALSE;
	}

	return TRUE;
}

static void
_mp_widget_text_popup_timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	startfunc;
	void (*cb)(void *data) = evas_object_data_get(obj, "timeout_callback");
	mp_evas_object_del(obj);

	struct appdata *ad = data;
	MP_CHECK(ad);
	ad->notify_delete = NULL;

	if(cb)
		cb(ad);
}

Evas_Object *
mp_widget_text_popup(void *data, const char *message)
{
	struct appdata *ad = data;
	Evas_Object *popup = NULL;
	popup = mp_popup_create(ad->win_main, MP_POPUP_NOTIFY, NULL, ad, _mp_widget_text_popup_timeout_cb, ad);
	ad->notify_delete = popup;

	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(popup, message);
	mp_popup_timeout_set(popup, MP_POPUP_TIMEOUT);
	evas_object_show(popup);
	return popup;
}

int
mp_widget_text_popup_with_cb(void *data, const char *message, void (*timeout_cb)(void *data))
{
	Evas_Object *popup = NULL;
	popup = mp_widget_text_popup(data, message);
	evas_object_data_set(popup, "timeout_callback", timeout_cb);
	return 0;
}

Evas_Object *
mp_widget_create_icon(Evas_Object * obj, const char *path, int w, int h)
{
	Evas_Object *thumbnail = elm_bg_add(obj);
	elm_bg_load_size_set(thumbnail, w, h);

	Evas *evas = evas_object_evas_get(obj);
	MP_CHECK_NULL(evas);
	if (mp_util_is_image_valid(evas, path))
	{
		elm_bg_file_set(thumbnail, path, NULL);
	}
	else
	{
		elm_bg_file_set(thumbnail, DEFAULT_THUMBNAIL, NULL);
	}

	evas_object_size_hint_aspect_set(thumbnail, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	return thumbnail;
}

Evas_Object *
mp_widget_create_bgimg(Evas_Object * parent)
{
	Evas_Object *bg;

	mp_retvm_if(parent == NULL, NULL, "parent is NULL");

	DEBUG_TRACE_FUNC();

	bg = evas_object_rectangle_add(evas_object_evas_get(parent));
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, bg);
	evas_object_show(bg);

	return bg;
}


Evas_Object *
mp_widget_create_button(Evas_Object * parent, char *style, char *caption, Evas_Object * icon,
			void (*func) (void *, Evas_Object *, void *), void *data)
{
	if (!parent)
		return NULL;

	Evas_Object *btn;

	btn = elm_button_add(parent);

	if (style)
		elm_object_style_set(btn, style);

	if (caption)
		elm_object_text_set(btn, caption);

	if (icon)
		elm_object_content_set(btn, icon);

	elm_object_focus_allow_set(btn, EINA_FALSE);
	evas_object_propagate_events_set(btn, EINA_FALSE);

	evas_object_smart_callback_add(btn, "clicked", func, (void *)data);

	return btn;
}

static void
_mp_widget_entry_maxlength_reached_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = data;
	MP_CHECK(ad);
	mp_popup_tickernoti_show(GET_SYS_STR("IDS_COM_POP_MAXIMUM_NUMBER_OF_CHARACTERS_REACHED"), false, false);
}

static void
_mp_widget_entry_changed_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = data;
	MP_CHECK(ad);

	Evas_Object *editfield = data;
	MP_CHECK(editfield);

	Evas_Object *entry = obj;
	MP_CHECK(entry);

	Eina_Bool entry_empty = elm_entry_is_empty(entry);
	const char *eraser_signal = NULL;
	const char *guidetext_signal = NULL;
	if (entry_empty) {
		eraser_signal =	"elm,state,eraser,hide";
		guidetext_signal = "elm,state,guidetext,show";
	} else {
		eraser_signal =	"elm,state,eraser,show";
		guidetext_signal = "elm,state,guidetext,hide";
	}
	elm_object_signal_emit(editfield, eraser_signal, "elm");
	elm_object_signal_emit(editfield, guidetext_signal, "elm");
}

static void
_mp_widget_entry_eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source) // When X marked button is clicked, empty entry's contents.
{
	Evas_Object *entry = data;
	MP_CHECK(entry);

	elm_entry_entry_set(entry, "");
}


Evas_Object *
mp_widget_create_editfield(Evas_Object * parent, int limit_size, char *guide_txt, struct appdata *ad)
{
	startfunc;
	Evas_Object *editfield = NULL;
	Evas_Object *entry = NULL;
	editfield = elm_layout_add(parent);
	elm_layout_theme_set(editfield, "layout", "editfield", "default");
	MP_CHECK_NULL(editfield);
	evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL,
					EVAS_HINT_FILL);

	entry = elm_entry_add(editfield);
	MP_CHECK_NULL(entry);
	elm_object_style_set(entry, "default");
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_NONE);
	elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);

	evas_object_data_set(editfield, "entry", entry);
	elm_object_part_content_set(editfield, "elm.swallow.content", entry);

	evas_object_smart_callback_add(entry, "changed", _mp_widget_entry_changed_cb, editfield);
	elm_object_signal_callback_add(editfield, "elm,eraser,clicked", "elm", _mp_widget_entry_eraser_clicked_cb, entry);

	if(limit_size > 0)
	{
		static Elm_Entry_Filter_Limit_Size limit_filter_data;

		limit_filter_data.max_char_count = 0;
		limit_filter_data.max_byte_count = limit_size;
		elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);
		evas_object_smart_callback_add(entry, "maxlength,reached", _mp_widget_entry_maxlength_reached_cb,
					       ad);
	}

	if(guide_txt)
		elm_object_part_text_set(editfield, "elm.guidetext", guide_txt);

	return editfield;

}

Evas_Object *
mp_widget_editfield_entry_get(Evas_Object *editfield)
{
	MP_CHECK_NULL(editfield);

	Evas_Object *entry = evas_object_data_get(editfield, "entry");

	return entry;
}

Evas_Object *
mp_widget_create_title_btn(Evas_Object *parent, const char *text, const char * icon_path, Evas_Smart_Cb func, void *data)
{
	Evas_Object *btn = elm_button_add(parent);
	Evas_Object * icon = NULL;
	MP_CHECK_NULL(btn);

	if(text)
		elm_object_text_set(btn, text);
	else if(icon_path)
	{
		icon = elm_icon_add(btn);
		MP_CHECK_NULL(icon);
		elm_image_file_set(icon, IMAGE_EDJ_NAME, icon_path);
		elm_object_content_set(btn, icon);
		elm_object_style_set(btn, "title_button");
	}
	evas_object_smart_callback_add(btn, "clicked", func, data);
	return btn;
}

Evas_Object *
mp_widget_create_layout_main(Evas_Object * parent)
{
	Evas_Object *layout;

	mp_retv_if(parent == NULL, NULL);
	layout = elm_layout_add(parent);
	mp_retvm_if(layout == NULL, NULL, "Failed elm_layout_add.\n");

	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, layout);

	evas_object_show(layout);

	return layout;
}

inline Evas_Object *
mp_common_load_edj(Evas_Object * parent, const char *file, const char *group)
{
	Evas_Object *eo = NULL;
	int r = -1;

	eo = elm_layout_add(parent);
	if (eo)
	{
		r = elm_layout_file_set(eo, file, group);
		if (!r)
		{
			evas_object_del(eo);
			return NULL;
		}
		evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(eo);
	}

	return eo;
}

static void
_mp_common_win_del(void *data, Evas_Object * obj, void *event)
{
	elm_exit();
}

Evas_Object *
mp_create_win(const char *name)
{
	Evas_Object *eo;
	int w, h;

	TA_S(2, "elm_win_add");
	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	TA_E(2, "elm_win_add");
	if (eo)
	{
		elm_win_title_set(eo, name);
		evas_object_smart_callback_add(eo, "delete,request", _mp_common_win_del, NULL);
		ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
		evas_object_resize(eo, w, h);
		TA_S(2, "elm_win_conformant_set");
		elm_win_conformant_set(eo, EINA_TRUE);
		TA_E(2, "elm_win_conformant_set");
	}
	return eo;
}

Evas_Object *
mp_common_create_naviframe_title_button(Evas_Object *parent, const char * text_id, void *save_cb, void *user_data)
{
	Evas_Object *btn_save = NULL;
	btn_save = elm_button_add(parent);
	elm_object_style_set(btn_save, "naviframe/title1/default");
        evas_object_size_hint_align_set(btn_save, EVAS_HINT_FILL, EVAS_HINT_FILL);

        evas_object_smart_callback_add(btn_save, "clicked", save_cb, user_data);

        elm_object_text_set(btn_save, GET_SYS_STR(text_id));
	mp_language_mgr_register_object(btn_save, OBJ_TYPE_ELM_OBJECT, NULL, text_id);
        evas_object_show(btn_save);

        return btn_save;

}


/*type
*	"naviframe/toolbar/default"
*	"naviframe/more/default"
*/

Evas_Object *mp_widget_create_toolbar_btn(Evas_Object *parent, const char *style, const char *text, Evas_Smart_Cb func, void *data)
{
	Evas_Object *btn = elm_button_add(parent);
	MP_CHECK_NULL(btn);

	elm_object_style_set(btn, style);
	elm_object_text_set(btn, text);
	evas_object_smart_callback_add(btn, "clicked", func, data);
	return btn;
}


