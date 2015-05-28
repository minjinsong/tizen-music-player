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


#ifndef __MP_SEARCH_H_
#define __MP_SEARCH_H_
#include <Evas.h>

Evas_Object *mp_search_create_new(Evas_Object * parent, Evas_Smart_Cb change_cb, void *change_cb_data, Evas_Smart_Cb cancel_cb, void *cancel_cb_data);
void mp_search_hide_imf_pannel(Evas_Object * search);
void mp_search_show_imf_pannel(Evas_Object * search);
Evas_Object *mp_search_entry_get(Evas_Object *search);
const char *mp_search_text_get(Evas_Object *search);
void mp_search_text_set(Evas_Object *search, const char *text);

#endif //__MP_SEARCH_H_
