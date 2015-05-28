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

#ifndef __MP_ITEM_H_
#define __MP_ITEM_H_

#include <Elementary.h>
#include <mp-define.h>
bool mp_item_update_db(char *fid);

int mp_item_share_by_bt(const char *formed_path, int file_cnt);


//===========for ALL share DNLA======================//
#define AS_IPC_NAME "org.tizen.allshare"
#define AS_IPC_REQUEST_OBJ "/com/samsumg/allshare"
#define AS_IPC_INTERFACE "org.tizen.allshare"

#define AS_IPC_REQUEST_METHOD "Request"

typedef struct
{
	int param1;		//Allshare_opp_req_t type
	int param2;		//the number of files
	char *param3;		//file path
} Allshare_para_info_t;

typedef enum
{
	AS_OPP_REQ_FILE_PLAY
} Allshare_opp_req_t;

#define mp_object_free(obj)	\
do {						\
	if(obj != NULL) {		\
		g_free(obj);		\
		obj = NULL;			\
	}						\
}while(0)

#endif
