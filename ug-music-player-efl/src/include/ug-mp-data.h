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
#ifndef __UG_MP_DATA_H__
#define __UG_MP_DATA_H__

typedef struct _ug_mp_list_data *ug_mp_data;

typedef enum
{
	MP_DATA_ARTIST,
	MP_DATA_ALBUM,
	MP_DATA_PLAYLIST,
}mp_data_e;

int ug_mp_data_count(mp_data_e type, char *filter_text);
int ug_mp_data_create(mp_data_e type, char *filter_text, ug_mp_data *data);
int ug_mp_data_destory(ug_mp_data data);

int ug_mp_data_get_main_text(

#endif

