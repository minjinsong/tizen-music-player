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


#ifndef __MP_HTTP_MGR_H__
#define __MP_HTTP_MGR_H__

#include "mp-define.h"

#define HTTP_ADDR_LEN_MAX		64
typedef enum
{
	MP_HTTP_SVC_DEFAULT,
	MP_HTTP_SVC_SHAZAM,
	MP_HTTP_SVC_STREAMING,
	MP_HTTP_SVC_MAX,
} mp_http_svc_type;

typedef enum
{
	MP_HTTP_RESPONSE_NORMAL,
	MP_HTTP_RESPONSE_DISCONNECT,
} mp_http_response_type;

typedef enum
{
	MP_HTTP_RESP_FAIL,
	MP_HTTP_RESP_SUCCESS,
} MpHttpRespResultType_t;

typedef void (*MpHttpRespCb) (gpointer user_data, int type, char *id, MpHttpRespResultType_t res, int view_id);
typedef void (*MpHttpOpenCb) (gpointer user_data);
typedef bool (*MpHttpRespExcuteCb)(gpointer user_data, mp_http_response_type response_type);
typedef bool(*MpHttpOpenExcuteCb) (gpointer user_data);

typedef enum
{
	MP_HTTP_STATE_NONE = 0,
	MP_HTTP_STATE_OFF = 0,
	MP_HTTP_STATE_CELLULAR,
	MP_HTTP_STATE_WIFI,
} MpHttpState_t;

typedef struct mp_http_mgr_t
{
	struct appdata *ad;
	MpHttpState_t http_state;		//the state of the http
	char ip[HTTP_ADDR_LEN_MAX];
	char proxy[HTTP_ADDR_LEN_MAX];

	bool svc_state[MP_HTTP_SVC_MAX];
	void *cb_data[MP_HTTP_SVC_MAX];
	MpHttpOpenExcuteCb http_open_cb[MP_HTTP_SVC_MAX];
	MpHttpRespExcuteCb http_resp_cb[MP_HTTP_SVC_MAX];
} mp_http_mgr_t;

typedef struct mp_http_data_t
{
	struct appdata *ad;
	int req_type;
	char *req_id;
	MpHttpRespCb user_callback;
	int view_id;
	void *user_data;
} mp_http_data_t;

bool mp_http_mgr_create(void *data);
bool mp_http_mgr_destory(void *data);
MpHttpState_t mp_http_mgr_get_state(void *data);
char* mp_http_mgr_get_user_agent(); /* return value should be freed */

const char* mp_http_mgr_get_ip(void *data);
const char* mp_http_mgr_get_proxy(void *data);
bool mp_http_mgr_get_svc_state(void *data, mp_http_svc_type svc_type);
bool mp_http_mgr_start_svc(void *data, mp_http_svc_type svc_type, MpHttpOpenExcuteCb open_cb, MpHttpRespExcuteCb rsp_cb, gpointer cb_data);
bool mp_http_mgr_stop_svc(void *data, mp_http_svc_type svc_type, mp_http_response_type response_type);

#define TOKEN "025B58C0"
#endif //__MP_HTTP_MGR_H__
