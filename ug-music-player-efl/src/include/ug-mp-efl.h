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

#ifndef __msUG_efl_H__
#define __msUG_efl_H__

#include <Elementary.h>
#include <glib.h>
#include <libintl.h>
#include <ui-gadget.h>
#include <ui-gadget-module.h>
#include "ug-mp-debug.h"
#include "mp-common-defs.h"
#include "ug-mp-text.h"
#include "ug-mp-widget.h"

#define ICON_SIZE 64*elm_config_scale_get()
#define DEFAULT_THUMBNAIL RESDIR"/images/music_player/34_thumb_07.png"

#define PKGNAME "ug-music-player-efl"
#define DOMAIN_NAME "music-player"
#define LOCALE_DIR PREFIX"/res/locale"

#define _EDJ(o)			elm_layout_edje_get(o)
#define GET_STR(s)			dgettext(DOMAIN_NAME, s)
#define dgettext_noop(s)	(s)
#define N_(s)			dgettext_noop(s)
#define GET_SYS_STR(str) dgettext("sys_string", str)

#define IF_FREE(p) ({if(p){free(p);p=NULL;}})

#define UG_MP_OPERATION_PICK 	"http://tizen.org/appcontrol/operation/pick"
#define UG_MP_SELECT_MODE_KEY	"http://tizen.org/appcontrol/data/selection_mode"
#define UG_MP_SELECT_MULTIPLE		"multiple"
#define UG_MP_OUTPUT_KEY		"http://tizen.org/appcontrol/data/selected"

typedef enum{
	UG_MP_SELECT_SINGLE,
	UG_MP_SELECT_MULTI,
}ug_type;

struct ug_data
{
	Evas_Object *base_layout;
	Evas_Object *navi_bar;

	ug_type select_type;

	ui_gadget_h ug;
};

#endif /* __msUG_efl_H__ */
