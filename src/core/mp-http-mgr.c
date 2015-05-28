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


#include "mp-player-debug.h"
#include "music.h"
#include "mp-http-mgr.h"

#define USER_AGENT_KEY		VCONFKEY_ADMIN_UAGENT

static MpHttpState_t	_mp_http_mgr_get_network_status();
static void _mp_http_mgr_refresh_network_info(mp_http_mgr_t *http_mgr);
static bool _mp_http_mgr_register_vconf_change_cb(mp_http_mgr_t *http_mgr);
static void _mp_http_mgr_ignore_vconf_change_cb();
static void _mp_http_mgr_network_config_changed_cb(keynode_t *node, void *user_data);
static void _mp_http_mgr_network_disconnect_cb(mp_http_mgr_t *http_mgr);


bool mp_http_mgr_create(void *data)
{
	DEBUG_TRACE("");
	struct appdata *ad = (struct appdata *)data;

	MP_CHECK_FALSE(ad);
	MP_CHECK_FALSE((!ad->http_mgr));

	ad->http_mgr = calloc(1, sizeof(mp_http_mgr_t));
	MP_CHECK_FALSE(ad->http_mgr);
	ad->http_mgr->ad = ad;

	if (!_mp_http_mgr_register_vconf_change_cb(ad->http_mgr))
		goto mp_exception;

	_mp_http_mgr_refresh_network_info(ad->http_mgr);

	return true;

      mp_exception:
	mp_http_mgr_destory(ad);
	return false;
}

bool mp_http_mgr_destory(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	MP_CHECK_FALSE(ad);
	MP_CHECK_FALSE(ad->http_mgr);

	_mp_http_mgr_ignore_vconf_change_cb();
	_mp_http_mgr_network_disconnect_cb(ad->http_mgr);

	IF_FREE(ad->http_mgr);

	return TRUE;
}

const char* mp_http_mgr_get_ip(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(ad->http_mgr);

	if (strlen(ad->http_mgr->ip) > 0)
		return (const char *)ad->http_mgr->ip;
	else
		return NULL;
}

const char* mp_http_mgr_get_proxy(void *data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(ad->http_mgr);

	if (strlen(ad->http_mgr->proxy) > 0)
		return (const char *)ad->http_mgr->proxy;
	else
		return NULL;
}

bool mp_http_mgr_get_svc_state(void *data, mp_http_svc_type svc_type)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);
	MP_CHECK_FALSE(ad->http_mgr);

	mp_debug("[%d]svc state is [%d]", svc_type, ad->http_mgr->svc_state[svc_type]);
	return ad->http_mgr->svc_state[svc_type];
}

bool mp_http_mgr_start_svc(void *data, mp_http_svc_type svc_type, MpHttpOpenExcuteCb open_cb, MpHttpRespExcuteCb rsp_cb, gpointer cb_data)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);
	MP_CHECK_FALSE(ad->http_mgr);

	if (!open_cb && !rsp_cb) {
		mp_error("no callback");
		return FALSE;
	}

	/* excute open callback */
	if (open_cb) {
		if (!open_cb(cb_data)) {
			mp_error("[%d] svc fail to start", svc_type);
			return FALSE;
		}
	}

	mp_debug("## [%d] svc started ##", svc_type);
	mp_http_mgr_t *http_mgr = (mp_http_mgr_t *)ad->http_mgr;

	/* register callback */
	http_mgr->cb_data[svc_type] = cb_data;
	http_mgr->http_open_cb[svc_type] = open_cb;
	http_mgr->http_resp_cb[svc_type] = rsp_cb;

	/* set open state */
	http_mgr->svc_state[svc_type] = TRUE;
	return TRUE;
}

bool mp_http_mgr_stop_svc(void *data, mp_http_svc_type svc_type, mp_http_response_type response_type)
{
	startfunc;
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK_FALSE(ad);
	MP_CHECK_FALSE(ad->http_mgr);

	mp_http_mgr_t *http_mgr = (mp_http_mgr_t *)ad->http_mgr;
	if (http_mgr->svc_state[svc_type] && http_mgr->http_resp_cb[svc_type]) {
		/* send response */
		http_mgr->http_resp_cb[svc_type](http_mgr->cb_data[svc_type], response_type);
	}

	/* deregister callback */
	http_mgr->cb_data[svc_type] = NULL;
	http_mgr->http_open_cb[svc_type] = NULL;
	http_mgr->http_resp_cb[svc_type] = NULL;

	/* set open state */
	http_mgr->svc_state[svc_type] = FALSE;
	mp_debug("## [%d] svc stopped ##", svc_type);

	return TRUE;
}

static MpHttpState_t _mp_http_mgr_get_network_status()
{
	MpHttpState_t state = MP_HTTP_STATE_OFF;

	int status = 0;
	if (vconf_get_int(VCONFKEY_NETWORK_STATUS, &status) == 0) {
		mp_debug("Network status = [%d]", status);
	} else {
		mp_error("vconf_get_int() fail!!");
		status = 0;
	}

	if (status == VCONFKEY_NETWORK_CELLULAR)
		state = MP_HTTP_STATE_CELLULAR;
	else if (status == VCONFKEY_NETWORK_WIFI)
		state = MP_HTTP_STATE_WIFI;
	else
		state = MP_HTTP_STATE_OFF;

	return status;
}

static bool _mp_http_mgr_register_vconf_change_cb(mp_http_mgr_t *http_mgr)
{
	startfunc;
	MP_CHECK_FALSE(http_mgr);

	if (vconf_notify_key_changed(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND,
				_mp_http_mgr_network_config_changed_cb, http_mgr) != 0) {
		mp_error("vconf_notify_key_changed() fail");
		return FALSE;
	}

	return TRUE;
}

static void _mp_http_mgr_ignore_vconf_change_cb()
{
	startfunc;
	if (vconf_ignore_key_changed(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND,
				_mp_http_mgr_network_config_changed_cb) !=0) {
		mp_error("vconf_ignore_key_changed() fail");
	}
}

static void _mp_http_mgr_refresh_network_info(mp_http_mgr_t *http_mgr)
{
	startfunc;
	MP_CHECK(http_mgr);

	/* reset network info */
	memset(http_mgr->ip, 0, sizeof(http_mgr->ip));
	memset(http_mgr->proxy, 0, sizeof(http_mgr->proxy));

	http_mgr->http_state = _mp_http_mgr_get_network_status();

	if (http_mgr->http_state != MP_HTTP_STATE_OFF) {
		/* refresh network infomation */
		char *ip = vconf_get_str(VCONFKEY_NETWORK_IP);
		if (ip) {
			strncpy(http_mgr->ip, ip, sizeof(http_mgr->ip)-1);
			free(ip);
			ip = NULL;
			mp_debug("IP [%s]", http_mgr->ip);
		}

		char *proxy = vconf_get_str(VCONFKEY_NETWORK_PROXY);
		if (proxy) {
			strncpy(http_mgr->proxy, proxy, sizeof(http_mgr->proxy)-1);
			free(proxy);
			proxy = NULL;
			mp_debug("PROXY [%s]", http_mgr->proxy);
		}
	}
}

static void _mp_http_mgr_network_config_changed_cb(keynode_t *node, void *user_data)
{
	startfunc;
	mp_http_mgr_t *http_mgr = (mp_http_mgr_t *)user_data;
	MP_CHECK(http_mgr);

	if (http_mgr->http_state != MP_HTTP_STATE_OFF) {
		_mp_http_mgr_network_disconnect_cb(http_mgr);
	}

	_mp_http_mgr_refresh_network_info(http_mgr);
}

static void _mp_http_mgr_network_disconnect_cb(mp_http_mgr_t *http_mgr)
{
	MP_CHECK(http_mgr);

	mp_debug("!! disconnect old connections !!");
	int i;
	for (i = 0 ; i < MP_HTTP_SVC_MAX; i++) {
		if (http_mgr->svc_state[i])
			mp_http_mgr_stop_svc(http_mgr->ad, i , MP_HTTP_RESPONSE_DISCONNECT);
	}
}

MpHttpState_t
mp_http_mgr_get_state(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	MP_CHECK_FALSE(ad);
	if (!ad->http_mgr)
	{
		return MP_HTTP_STATE_NONE;
	}

	mp_http_mgr_t *http_mgr = ad->http_mgr;
	mp_debug("http_state = [%d]", http_mgr->http_state);

	return http_mgr->http_state;
}

char* mp_http_mgr_get_user_agent()
{
	/* return value should be free */
	char *user_agent = vconf_get_str(USER_AGENT_KEY);
	return user_agent;
}

