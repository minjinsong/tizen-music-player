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

#ifndef __SP_VIEW_MANAGER_H__
#define __SP_VIEW_MANAGER_H__

#include <Elementary.h>

typedef struct _Sp_View_Manager Sp_View_Manager;

typedef enum {
	SP_VIEW_TYPE_DEFAULT,
	SP_VIEW_TYPE_PLAY,
	SP_VIEW_TYPE_INFO,
	SP_VIEW_TYPE_NUM,
} Sp_View_Type;

Sp_View_Manager* sp_view_mgr_create(Evas_Object *navi);
void sp_view_mgr_destroy(Sp_View_Manager* view_mgr);
Evas_Object* sp_view_mgr_get_naviframe(Sp_View_Manager *view_mgr);
void sp_view_mgr_push_view_content(Sp_View_Manager *view_mgr, Evas_Object *content, Sp_View_Type type);
void sp_view_mgr_pop_view_content(Sp_View_Manager *view_mgr, bool pop_to_first);
void sp_view_mgr_pop_view_to(Sp_View_Manager *view_mgr, Sp_View_Type type);
Elm_Object_Item * sp_view_mgr_get_play_view_navi_item(Sp_View_Manager *view_mgr);
void sp_view_mgr_play_view_title_label_set(Sp_View_Manager *view_mgr, const char *title);
void sp_view_mgr_set_title_label(Sp_View_Manager *view_mgr, const char *title);
void sp_view_mgr_set_title_visible(Sp_View_Manager *view_mgr, bool flag);
void sp_view_mgr_set_back_button(Sp_View_Manager *view_mgr, Evas_Smart_Cb cb, void *data);

#endif /* __SP_VIEW_MANAGER_H__ */
