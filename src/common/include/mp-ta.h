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

#ifndef _MP_TA_H_
#define _MP_TA_H_

#include "mp-define.h"

#define ENABLE_MP_TA
#ifdef ENABLE_MP_TA

// defs.
#define MP_TA_MAX_CHECKPOINT	500
#define MP_TA_MAX_ACCUM		500

typedef struct _mp_ta_checkpoint
{
	unsigned long timestamp;
	char *name;
} mp_ta_checkpoint;

typedef struct _mp_ta_accum_item
{
	unsigned long elapsed_accum;
	unsigned long num_calls;
	unsigned long elapsed_min;
	unsigned long elapsed_max;
	unsigned long first_start;
	unsigned long last_end;

	char *name;

	unsigned long timestamp;
	int on_estimate;
	int num_unpair;
} mp_ta_accum_item;

#define MP_TA_SHOW_STDOUT	0
#define MP_TA_SHOW_STDERR	1
#define MP_TA_SHOW_FILE	2
#define MP_TA_ENABLE_FILE MP_INI_DIR"/ta"
#define MP_TA_RESULT_FILE MP_INI_DIR"/mp-ta.log"


/////////////////////////////
// COMMON
int mp_ta_init(void);
int mp_ta_release(void);
void mp_ta_set_enable(int enable);
char *mp_ta_fmt(const char *fmt, ...);

/////////////////////////////
// CHECK POINT
int mp_ta_add_checkpoint(char *name, int show, char *filename, int line);
void mp_ta_show_checkpoints(void);
void mp_ta_show_diff(char *name1, char *name2);

int mp_ta_get_numof_checkpoints();
unsigned long mp_ta_get_diff(char *name1, char *name2);
//char* mp_ta_get_name(int idx);
bool mp_ta_is_init(void);


/////////////////////////////
// ACCUM ITEM
int mp_ta_accum_item_begin(char *name, int show, char *filename, int line);
int mp_ta_accum_item_end(char *name, int show, char *filename, int line);
void mp_ta_accum_show_result(int direction);

// macro.
#define MP_TA_INIT()								(	mp_ta_init()												)
#define MP_TA_RELEASE()							(	mp_ta_release()											)
#define MP_TA_SET_ENABLE(enable)				(	mp_ta_set_enable(enable)								)

// checkpoint handling
#define MP_TA_ADD_CHECKPOINT(name,show)		(	mp_ta_add_checkpoint(name,show,__FILE__,__LINE__)		)
#define MP_TA_SHOW_CHECKPOINTS()				(	mp_ta_show_checkpoints()								)
#define MP_TA_SHOW_DIFF(name1, name2)			(	mp_ta_show_diff(name1, name2)							)
#define MP_TA_GET_NUMOF_CHECKPOINTS()			(	mp_ta_get_numof_checkpoints()							)
#define MP_TA_GET_DIFF(name1, name2)			(	mp_ta_get_diff(name1, name2)							)
//#define MP_TA_GET_NAME(idx)                                           (       mp_ta_get_name(idx)                                                                     )

// accum item handling
#define MP_TA_ACUM_ITEM_BEGIN(name,show)		(	mp_ta_accum_item_begin(name,show,__FILE__,__LINE__)	)
#define MP_TA_ACUM_ITEM_END(name,show)		(	mp_ta_accum_item_end(name,show,__FILE__,__LINE__)		)
#define MP_TA_ACUM_ITEM_SHOW_RESULT()		(	mp_ta_accum_show_result(MP_TA_SHOW_STDOUT)			)
#define MP_TA_ACUM_ITEM_SHOW_RESULT_TO(x)	(	mp_ta_accum_show_result(x)							)
/*
#define __mp_ta__(name, x) \
MP_TA_ACUM_ITEM_BEGIN(name, 0); \
x \
MP_TA_ACUM_ITEM_END(name, 0);

#define __mm_tafmt__(fmt, args...) 			(	mp_ta_fmt(fmt, ##args)	)
*/

#define TA_S(level, name)\
do{\
	if(!mp_ta_is_init())\
		break;\
	char buf[128] = {0,};\
	int i = 0, pos = 0;\
	while(i < level){pos += snprintf(buf+pos, 128-pos, "    "); i++;}\
	snprintf(buf+pos, 128-pos, "%s", name);\
	mp_ta_accum_item_begin(buf,0,__FILE__,__LINE__);\
}while(0)

#define TA_E(level, name)\
do{\
	if(!mp_ta_is_init())\
		break;\
	char buf[128] = {0,};\
	int i = 0, pos = 0;\
	while(i < level){pos += snprintf(buf+pos, 128-pos, "    "); i++;}\
	snprintf(&buf[pos], 128-pos, "%s", name);\
	mp_ta_accum_item_end(buf,0,__FILE__,__LINE__);\
}while(0)

#else //#ifdef ENABLE_MP_TA

#define MP_TA_INIT()
#define MP_TA_RELEASE()
#define MP_TA_SET_ENABLE(enable)

// checkpoint handling
#define MP_TA_ADD_CHECKPOINT(name,show)
#define MP_TA_SHOW_CHECKPOINTS()
#define MP_TA_SHOW_DIFF(name1, name2)
#define MP_TA_GET_NUMOF_CHECKPOINTS()
#define MP_TA_GET_DIFF(name1, name2)
//#define MP_TA_GET_NAME(idx)

// accum item handling
#define MP_TA_ACUM_ITEM_BEGIN(name,show)
#define MP_TA_ACUM_ITEM_END(name,show)
#define MP_TA_ACUM_ITEM_SHOW_RESULT()
#define MP_TA_ACUM_ITEM_SHOW_RESULT_TO(x)
/*
#define __mp_ta__(name, x)
#define __mm_tafmt__(fmt, args...)
*/
#define TA_S(level, name)
#define TA_E(level, name)
#endif //#ifdef ENABLE_MP_TA

#endif // _MP_TA_H_
