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

#ifndef __MP_LYRIC_MGR_H__
#define __MP_LYRIC_MGR_H__

#include "mp-define.h"

#define MP_LRC_LINE_BUF_LEN (int)255 /* The max length of one line string buffer */


typedef enum
{
	MP_LYRIC_SOURCE_BUFFER=0,
	MP_LYRIC_SOURCE_LIST,
	MP_LYRIC_SOURCE_FILE,
}mp_lyric_source_type;

typedef struct
{
	long time;
	char *lyric;
}mp_lrc_node_t;

typedef struct
{
	char *title;
	char *artist;
	char *album;
	long offset; /* The offset of all time tags */
	Eina_List *synclrc_list;
	Eina_List *unsynclrc_list;
	mp_lyric_source_type source_type;
}mp_lyric_mgr_t;

bool mp_lyric_mgr_create(void *data, void *lrcData, mp_lyric_source_type source_type);
bool mp_lyric_mgr_destory(void *data);

mp_lyric_mgr_t* mp_lyric_mgr_parse_file(const char *lrcPath);
mp_lyric_mgr_t* mp_lyric_mgr_parse_buffer(const char *lrcBuffer);
void mp_lyric_mgr_data_free(mp_lyric_mgr_t **data);

#endif /* __MP_LYRIC_MGR_H__ */
