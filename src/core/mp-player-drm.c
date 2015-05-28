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

#include "music.h"
#include <drm_client.h>
#include <drm_client_types.h>
#ifdef MP_FEATURE_DRM_CONSUMPTION
#include <drm_trusted_client.h>
#include <drm_trusted_client_types.h>
#endif

#include "mp-player-drm.h"
#include "mp-item.h"
#include "mp-player-control.h"
#include "mp-play.h"
#include "mp-player-debug.h"
#include "mp-playlist-mgr.h"
#include "mp-util.h"
#include "mp-popup.h"
#include "mp-widget.h"

#ifdef MP_FEATURE_DRM_CONSUMPTION
static DRM_DECRYPT_HANDLE _g_drm_handle = NULL;
static bool _g_drm_consumption = FALSE;
#endif
#define ACCUMULATED_DATE	 86400	// 24*60*60

static bool check_interval_constraint = false;

bool
mp_drm_get_content_info(const char *path, drm_content_info_e first_info, ...)
{
	/* return info shoud be freed */
	MP_CHECK_FALSE(path);

	drm_content_info_s info;
	memset(&info, 0x0, sizeof(drm_content_info_s));
	int res = drm_get_content_info(path, &info);
	if (res != DRM_RETURN_SUCCESS) {
		mp_error("drm_get_content_info().. [0x%x]", res);
		return false;
	}

	va_list var_args;
	drm_content_info_e attr;

	attr = first_info;
	va_start(var_args, first_info);
	char **ret_val = NULL;
	char *value = NULL;
	while (attr > DRM_CONTENT_INFO_NULL) {
		ret_val = va_arg((var_args), char **);

		switch (attr) {
		case DRM_CONTENT_INFO_AUTHOR:
			value = info.author;
			break;

		case DRM_CONTENT_INFO_RIGHTS_URL:
			value = info.rights_url;
			break;

		case DRM_CONTENT_INFO_DESCRIPTION:
			value = info.description;
			break;

		default:
			mp_debug("Not defined [%d]", attr);
			value = NULL;
			break;
		}

		/* output */
		*ret_val = g_strdup(value);

		attr = va_arg(var_args, drm_content_info_e);
	}

	va_end(var_args);

	return true;
}


static void
_mp_drm_check_remain_ro(struct appdata *ad)
{
	MP_CHECK(ad);

	char *title = NULL;
	MP_CHECK(ad->current_track_info);

	title = ad->current_track_info->title;

	const char *format_str = NULL;
	char *message = NULL;

	mp_constraints_info_s *info = &ad->drm_constraints_info;
	if (info->constraints & MP_DRM_CONSTRAINT_COUNT && info->remaining_count <= 2)
	{
		int remain = info->remaining_count - 1;
		if (remain == 1) {
			format_str = GET_SYS_STR("IDS_COM_POP_YOU_CAN_USE_PS_1_MORE_TIME_GET_ANOTHER_LICENCE_Q");
			message = g_strdup_printf(format_str, title);
		} else if (remain == 0) {
			format_str = GET_SYS_STR("IDS_COM_POP_YOU_CANNOT_USE_PS_ANY_MORE_TIMES_GET_ANOTHER_LICENCE_Q");
			message = g_strdup_printf(format_str, title);
		}
	}
	else if (info->constraints & MP_DRM_CONSTRAINT_ACCUMLATED_TIME)
	{
		if (info->remaining_acc_sec/ACCUMULATED_DATE <= 1) {
			format_str = GET_SYS_STR("IDS_COM_POP_PS_IS_ABOUT_TO_EXPIRE_GET_ANOTHER_LICENCE_Q");
			message = g_strdup_printf(format_str, title);
		}
	}

	if (message)
	{
		ad->can_play_drm_contents = false;	// wap launch
		mp_drm_set_notify(ad, message);
		SAFE_FREE(message);
	}

}

static void
mp_drm_popup_response_cb(void *data, Evas_Object * obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	DEBUG_TRACE("response callback=%d", (int)event_info);
	mp_retm_if(!ad, "ad is NULL!!!!");

	if(obj)
		evas_object_del(obj);

	mp_plst_item *current_item = mp_playlist_mgr_get_current(ad->playlist_mgr);

	int response = (int)event_info;
	if (response == MP_POPUP_YES && current_item)
	{
		if (ad->can_play_drm_contents)
		{
			if (mp_play_new_file(data, FALSE))
			{
				_mp_drm_check_remain_ro(ad);
			}
		}
		else
		{
			char *rights_url = NULL;
			bool ret = mp_drm_get_content_info(current_item->uri, DRM_CONTENT_INFO_RIGHTS_URL, &rights_url, -1);
			if (ret && rights_url) {
				DEBUG_TRACE("right url : %s", rights_url);
				if (!mp_util_launch_browser(rights_url, NULL))
					WARN_TRACE("Fail to launch browser!!!");
				SAFE_FREE(rights_url);
			}
			else
			{
				WARN_TRACE("Fail to get right url");
			}
		}
	}
	else if (response == MP_POPUP_NO)
	{
		// do nothing
	}
}

void
mp_drm_set_notify(void *data, char *message)
{
	struct appdata *ad = (struct appdata *)data;

	mp_popup_destroy(ad);

	Evas_Object *popup = mp_popup_create(ad->win_main, MP_POPUP_NORMAL, NULL, ad, mp_drm_popup_response_cb, ad);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(popup, message);
	mp_popup_button_set(popup, MP_POPUP_BTN_1, GET_SYS_STR("IDS_COM_SK_YES"), MP_POPUP_YES);
	mp_popup_button_set(popup, MP_POPUP_BTN_2, GET_SYS_STR("IDS_COM_SK_NO"), MP_POPUP_NO);
	evas_object_show(popup);

	ad->popup[MP_POPUP_NORMAL] = popup;
}

#ifdef MP_FEATURE_DRM_CONSUMPTION
static DRM_DECRYPT_HANDLE
_mp_drm_create_decrypt_handle(const char *path)
{
	MP_CHECK_NULL(path);

	drm_trusted_open_decrypt_info_s open_input_data;
	drm_trusted_open_decrypt_resp_data_s open_output_data;
	DRM_DECRYPT_HANDLE decrypt_handle = NULL;
	int ret = -1;

	memset(&open_input_data, 0x0, sizeof(drm_trusted_open_decrypt_info_s));
	memset(&open_output_data, 0x0, sizeof(drm_trusted_open_decrypt_resp_data_s));

	SAFE_STRCPY(open_input_data.filePath, path);

	ret = drm_trusted_open_decrypt_session(&open_input_data, &open_output_data, &decrypt_handle);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		mp_error("drm_trusted_open_decrypt_session() .. [0x%x]", ret);
		return NULL;
	}

	return decrypt_handle;
}

static void
_mp_drm_destroy_decrpyt_handle(DRM_DECRYPT_HANDLE handle)
{
	MP_CHECK(handle);

	int ret = drm_trusted_close_decrypt_session(&handle);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		mp_error("drm_trusted_close_decrypt_session() ... [0x%x]", ret);
	}

	handle = NULL;
}

void
mp_drm_set_consumption(bool enabled)
{
	_g_drm_consumption = enabled;
}

bool
mp_drm_get_consumption(void)
{
	return _g_drm_consumption;
}

void
mp_drm_start_consumption(char *path)
{
	DEBUG_TRACE();
	if (mp_drm_get_consumption())
	{
		if (_g_drm_handle) {
			drm_trusted_close_decrypt_session(&_g_drm_handle);
			_g_drm_handle = NULL;
		}
		/* handle create */
		_g_drm_handle = _mp_drm_create_decrypt_handle(path);
		MP_CHECK(_g_drm_handle);

		drm_trusted_set_consumption_state_info_s state_input_data;
		memset(&state_input_data, 0x0, sizeof(drm_trusted_set_consumption_state_info_s));
		state_input_data.state = DRM_CONSUMPTION_STARTED;
		int ret = drm_trusted_set_decrypt_state(_g_drm_handle, &state_input_data);
		if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
			mp_error("drm_trusted_set_decrypt_state()..[0x%x]", ret);
		}
	}
}


void
mp_drm_pause_consumption(void)
{
	if (mp_drm_get_consumption() && _g_drm_handle)
	{
		drm_trusted_set_consumption_state_info_s state_input_data;
		memset(&state_input_data, 0x0, sizeof(drm_trusted_set_consumption_state_info_s));
		state_input_data.state = DRM_CONSUMPTION_PAUSED;
		int ret = drm_trusted_set_decrypt_state(_g_drm_handle, &state_input_data);
		if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
			mp_error("drm_trusted_set_decrypt_state()..[0x%x]", ret);
		}
	}
}

void
mp_drm_resume_consumption(void)
{
	if (mp_drm_get_consumption() && _g_drm_handle)
	{
		drm_trusted_set_consumption_state_info_s state_input_data;
		memset(&state_input_data, 0x0, sizeof(drm_trusted_set_consumption_state_info_s));
		state_input_data.state = DRM_CONSUMPTION_RESUMED;
		int ret = drm_trusted_set_decrypt_state(_g_drm_handle, &state_input_data);
		if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
			mp_error("drm_trusted_set_decrypt_state()..[0x%x]", ret);
		}
	}
}

void
mp_drm_stop_consumption(void)
{
	if (mp_drm_get_consumption() && _g_drm_handle)
	{
		/* stop */
		drm_trusted_set_consumption_state_info_s state_input_data;
		memset(&state_input_data, 0x0, sizeof(drm_trusted_set_consumption_state_info_s));
		state_input_data.state = DRM_CONSUMPTION_STOPPED;
		int ret = drm_trusted_set_decrypt_state(_g_drm_handle, &state_input_data);
		if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
			mp_error("drm_trusted_set_decrypt_state()..[0x%x]", ret);
		}

		/* handle destroy */
		_mp_drm_destroy_decrpyt_handle(_g_drm_handle);
		_g_drm_handle = NULL;
	}
}
#endif

bool
mp_drm_file_right(char *path)
{
	MP_CHECK_FALSE(path);

	drm_bool_type_e is_drm = DRM_UNKNOWN;
	drm_is_drm_file(path, &is_drm);
	if (is_drm == DRM_TRUE)
		return TRUE;

	return FALSE;
}

bool
mp_drm_has_valid_ro(char *path)
{
	MP_CHECK_FALSE(path);

	drm_license_status_e license_status = DRM_LICENSE_STATUS_UNDEFINED;
	int res = drm_get_license_status(path, DRM_PERMISSION_TYPE_PLAY, &license_status);
	if (res != DRM_RETURN_SUCCESS) {
		mp_debug("drm_get_license_status().. [0x%x]", res);
	}

	mp_debug("license_status = %d", license_status);

	if (license_status == DRM_LICENSE_STATUS_VALID)
		return TRUE;

	return FALSE;
}

static drm_file_type_e
_mp_drm_get_file_type(const char *path)
{
	MP_CHECK_VAL(path, DRM_TYPE_UNDEFINED);

	drm_file_type_e type = DRM_TYPE_UNDEFINED;
	int ret = drm_get_file_type(path, &type);
	if (ret != DRM_RETURN_SUCCESS) {
		mp_error("drm_get_file_type().. [0x%x]", ret);
		type = DRM_TYPE_UNDEFINED;
	}

	mp_debug("DRM file type[%s=>%d]", path, type);
	return type;
}

bool
mp_drm_get_description(void *data, char *path)
{
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad && path);

	char *description = NULL;
	bool ret = mp_drm_get_content_info(path, DRM_CONTENT_INFO_DESCRIPTION, &description, -1);
	if (ret && description) {
		mp_debug("description = [%s]", description);
		SAFE_STRCPY(ad->drm_info.description, description);
		ret = TRUE;
	} else {
		ret = FALSE;
	}

	SAFE_FREE(description);

	return ret;
}

bool
mp_drm_check_forward(void *data, char *path)
{
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad && path);

	drm_file_type_e type = _mp_drm_get_file_type(path);

	if (type == DRM_TYPE_OMA_V1 || type == DRM_TYPE_OMA_V2)
	{
		drm_file_info_s info;
		memset(&info, 0x0, sizeof(drm_file_info_s));
		int ret = drm_get_file_info(path, &info);
		if (ret != DRM_RETURN_SUCCESS) {
			mp_error("drm_get_file_info()... [0x%x]", ret);
			return FALSE;
		}

		ad->drm_info.version = info.oma_info.version;
		if (info.oma_info.version == DRM_OMA_DRMV1_RIGHTS)
		{
			if (info.oma_info.method == DRM_METHOD_TYPE_SEPARATE_DELIVERY)
				ad->drm_info.forward = TRUE;
			else
				ad->drm_info.forward = FALSE;
		}
		else if (info.oma_info.version == DRM_OMA_DRMV2_RIGHTS)
		{
			//all drmv2 contents can be forwared.
			ad->drm_info.forward = TRUE;
		}
		else
		{
			ERROR_TRACE("Unknown version\n");
			return FALSE;
		}
	}
	else if (type == DRM_TYPE_PLAYREADY)
	{
		DEBUG_TRACE("playready drm file");
		ad->drm_info.forward = TRUE;
	}
	else
	{
		ERROR_TRACE("Not supported drm type");
		return FALSE;
	}
	return TRUE;
}

bool
mp_drm_get_left_ro_info(const char *path, mp_constraints_info_s *info)
{
	MP_CHECK_FALSE(path && info);

	if (!mp_drm_has_valid_ro((char *)path))
	{
		mp_error("No valid ro");
		return false;
	}

	/* init info */
	memset(info, 0x0, sizeof(mp_constraints_info_s));

	drm_constraint_info_s constraint;
	memset(&constraint, 0x0, sizeof(drm_constraint_info_s));

	int res = drm_get_constraint_info(path, DRM_PERMISSION_TYPE_PLAY, &constraint);
	if (res == DRM_RETURN_SUCCESS) {
		if (constraint.const_type.is_unlimited) {
			DEBUG_TRACE("UNLIMITED");
			info->constraints = MP_DRM_CONSTRAINT_UNLIMITED;
			return true;
		}

		if (constraint.const_type.is_count) {
			DEBUG_TRACE("DRM_COUNT [%d]", constraint.remaining_count);
			info->constraints |= MP_DRM_CONSTRAINT_COUNT;
			info->remaining_count = constraint.remaining_count;
		}

		if (constraint.const_type.is_datetime) {
			DEBUG_TRACE("DRM_DATETIME");
			info->constraints |= MP_DRM_CONSTRAINT_DATE_TIME;
			DEBUG_TRACE("%d.%d.%d %d:%d~%d.%d.%d %d:%d",
				constraint.start_time.tm_mday,
				constraint.start_time.tm_mon + 1,
				constraint.start_time.tm_year + 1900,
				constraint.start_time.tm_hour,
				constraint.start_time.tm_min,
				constraint.end_time.tm_mday,
				constraint.end_time.tm_mon + 1,
				constraint.end_time.tm_year + 1900,
				constraint.end_time.tm_hour,
				constraint.end_time.tm_min);

			struct timeval tv;
			gettimeofday(&tv, NULL);
			struct tm *ptm = NULL;
			ptm = localtime(&tv.tv_sec);

			if (ptm->tm_year >= constraint.start_time.tm_year
				&& ptm->tm_mon >= constraint.start_time.tm_mon
				&& ptm->tm_mday >= constraint.start_time.tm_mday
				&& ptm->tm_hour >= constraint.start_time.tm_hour
				&& ptm->tm_min >= constraint.start_time.tm_min
				&& ptm->tm_mon <= constraint.end_time.tm_mon
				&& ptm->tm_mday <= constraint.end_time.tm_mday
				&& ptm->tm_hour <= constraint.end_time.tm_hour
				&& ptm->tm_min <= constraint.end_time.tm_min)
			{
				/* not expired */
				info->date_time_expired = false;
			}
			else
			{
				info->date_time_expired = true;
			}

		}

		if (constraint.const_type.is_interval) {
			DEBUG_TRACE("DRM_INTERVAL");
			info->constraints |= MP_DRM_CONSTRAINT_INTERVAL;
			DEBUG_TRACE("Remain... %d.%d.%d %d:%d",
				constraint.interval_time.tm_mon,
				constraint.interval_time.tm_mday,
				constraint.interval_time.tm_year,
				constraint.interval_time.tm_hour,
				constraint.interval_time.tm_min);

			info->remaining_interval_sec = constraint.interval_time.tm_sec + constraint.interval_time.tm_min * 60 + constraint.interval_time.tm_hour * 3600;
			info->remaining_interval_sec += (constraint.interval_time.tm_mday + constraint.interval_time.tm_mon * 30 + constraint.interval_time.tm_year * 365) * (3600 *24);
		}

		if (constraint.const_type.is_timedcount) {
			DEBUG_TRACE("DRM_TIMED_COUNT");
			DEBUG_TRACE("%d left (%d sec)", constraint.timed_remaining_count, constraint.timed_count_timer);
			info->constraints |= MP_DRM_CONSTRAINT_TIMED_COUNT;
			info->remaining_timed_count = constraint.timed_remaining_count;
		}

		if (constraint.const_type.is_accumulated) {
			DEBUG_TRACE("DRM_ACCUMULATED [%d]", constraint.accumulated_remaining_seconds);
			info->constraints |= MP_DRM_CONSTRAINT_ACCUMLATED_TIME;
			info->remaining_acc_sec = constraint.accumulated_remaining_seconds;
		}

		if (constraint.const_type.is_individual) {
			mp_debug("DRM_INDIVISUAL_ID [%s]", constraint.individual_id);
		}

		if (constraint.const_type.is_system) {
			mp_debug("DRM_SYSTEM [ID:%s, type:%d]", constraint.system_id, constraint.system_identity_type);
		}
	}
	else {
		mp_error("drm_get_constraint_info().. 0x%x", res);
		return FALSE;
	}

	return TRUE;
}


bool
mp_drm_check_left_ro(void *data, char *path)
{
	struct appdata *ad = data;
	MP_CHECK_FALSE(ad);

	bool wap_launch = FALSE;
	bool expired = FALSE;
	char *title = NULL;

	mp_retvm_if(!ad, NULL, "ad is NULL!!!!");

	if(mp_drm_check_foward_lock(path))
	{
		DEBUG_TRACE("Foward lock");
		return true;
	}

	title = ad->current_track_info->title;

	ad->can_play_drm_contents = FALSE;

	DEBUG_TRACE("mp_drm_check_left_ro\n");
	mp_constraints_info_s *info = &(ad->drm_constraints_info);
	memset(info, 0x0, sizeof(mp_constraints_info_s));
	bool has_valid_ro = mp_drm_get_left_ro_info(path, info);

	const char *format_str = NULL;
	char *message = NULL;
	if (has_valid_ro)
	{
		while (info->constraints)
		{
			if (info->constraints & MP_DRM_CONSTRAINT_COUNT)
			{
				if (info->remaining_count == 2) {
					format_str = GET_SYS_STR("IDS_COM_POP_YOU_CAN_USE_PS_2_MORE_TIMES_START_NOW_Q");
					message = g_strdup_printf(format_str, title);
				} else if (info->remaining_count == 1) {
					format_str = GET_SYS_STR("IDS_COM_POP_YOU_CAN_USE_PS_1_MORE_TIME_START_NOW_Q");
					message = g_strdup_printf(format_str, title);
				} else if (info->remaining_count == 0) {
					wap_launch = true;
					break;
				}
			}

			if (info->constraints & MP_DRM_CONSTRAINT_DATE_TIME)
			{
				if (info->date_time_expired)	{
					wap_launch = true;
					break;
				}
			}

			if (info->constraints & MP_DRM_CONSTRAINT_INTERVAL)
			{
				if (info->remaining_interval_sec == 0) {
					wap_launch = true;
					break;
				} else if (!check_interval_constraint) {
					check_interval_constraint = true;
					format_str = GET_SYS_STR("IDS_COM_POP_YOU_CAN_USE_PS_FOR_PD_DAYS_START_NOW_Q");
					int days = info->remaining_interval_sec / ACCUMULATED_DATE + 1;
					message = g_strdup_printf(format_str, title, days);
				}
			}

			if (info->constraints & MP_DRM_CONSTRAINT_TIMED_COUNT)
			{
				if (info->remaining_timed_count == 2) {
					format_str = GET_SYS_STR("IDS_COM_POP_YOU_CAN_USE_PS_2_MORE_TIMES_START_NOW_Q");
					message = g_strdup_printf(format_str, title);
				} else if (info->remaining_timed_count == 1) {
					format_str = GET_SYS_STR("IDS_COM_POP_YOU_CAN_USE_PS_1_MORE_TIME_START_NOW_Q");
					message = g_strdup_printf(format_str, title);
				} else if (info->remaining_timed_count == 0) {
					wap_launch = true;
					break;
				}
			}

			if (info->constraints & MP_DRM_CONSTRAINT_ACCUMLATED_TIME)
			{
				if (info->remaining_acc_sec == 0) {
					wap_launch = true;
					break;
				}
			}

			ad->can_play_drm_contents = true;
			break;
		}

		if (info->constraints == MP_DRM_CONSTRAINT_UNLIMITED) {
			ad->can_play_drm_contents = true;
		}
	}

	if (!has_valid_ro || (wap_launch && !mp_drm_check_expiration(path, expired)))
	{
		DEBUG_TRACE("have no valid ro");
		format_str = GET_SYS_STR("IDS_COM_POP_PS_CURRENTLY_LOCKED_UNLOCK_Q");
		message = g_strdup_printf(format_str, title);
		mp_drm_set_notify(ad, message);
		SAFE_FREE(message);
		return FALSE;
	}
	else if (message)
	{
		/* play possible */
		mp_debug("warning popup=[%s]", message);
		mp_drm_set_notify(ad, message);
		SAFE_FREE(message);
		return FALSE;
	}
	return TRUE;
}

bool
mp_drm_check_expiration(char *path, bool expired)
{
	if (mp_drm_file_right(path))
	{

		drm_file_info_s info;
		memset(&info, 0x0, sizeof(drm_file_info_s));
		int ret = drm_get_file_info(path, &info);
		if (ret == DRM_RETURN_SUCCESS) {
			DEBUG_TRACE("method [%d] expired [%d]\n", info.oma_info.method, expired);
			if (info.oma_info.method == DRM_METHOD_TYPE_COMBINED_DELIVERY
				|| info.oma_info.method == DRM_METHOD_TYPE_FORWARD_LOCK) {

				return TRUE;
			}

			return FALSE;
		}
	}

	return FALSE;
}

bool
mp_drm_check_foward_lock(char *path)
{
	if (mp_drm_file_right(path))
	{

		drm_file_info_s info;
		memset(&info, 0x0, sizeof(drm_file_info_s));
		int ret = drm_get_file_info(path, &info);
		if (ret == DRM_RETURN_SUCCESS) {
			if (info.oma_info.method == DRM_METHOD_TYPE_FORWARD_LOCK) {
				mp_debug("forward lock..");
				return TRUE;
			}
		}
	}

	return FALSE;
}

static mp_drm_right_status_t*
_mp_drm_create_right_status_item(const char *type, const char *validity)
{
	MP_CHECK_NULL(type);

	mp_drm_right_status_t *rs = calloc(1, sizeof(mp_drm_right_status_t));
	mp_assert(rs);
	rs->type = g_strdup(type);
	rs->validity = g_strdup(validity);

	return rs;
}

GList*
mp_drm_get_right_status(const char *path)
{
	startfunc;
	MP_CHECK_NULL(path);

	GList *rs_list = NULL;

	drm_constraint_info_s constraint;
	memset(&constraint, 0x0, sizeof(drm_constraint_info_s));
	int res = drm_get_constraint_info(path, DRM_PERMISSION_TYPE_PLAY, &constraint);
	if (res != DRM_RETURN_SUCCESS) {
		mp_error("drm_get_constraint_info().. [0x%x]", res);
		return NULL;
	}

	char *validity_str = NULL;

	if (constraint.const_type.is_unlimited) {
		DEBUG_TRACE("UNLIMITED");
		mp_drm_right_status_t *rs = _mp_drm_create_right_status_item(GET_SYS_STR("IDS_COM_POP_UNLIMITED"), NULL);
		rs_list = g_list_append(rs_list, rs);
	}

	if (constraint.const_type.is_count) {
		validity_str = g_strdup_printf("%d left", constraint.remaining_count);
		mp_drm_right_status_t *rs = _mp_drm_create_right_status_item(GET_STR("Count"), validity_str);
		SAFE_FREE(validity_str);
		rs_list = g_list_append(rs_list, rs);
	}

	if (constraint.const_type.is_datetime) {
		DEBUG_TRACE("DRM_DATETIME\n");
		validity_str = g_strdup_printf("%d.%d.%d %d:%d~%d.%d.%d %d:%d",
					constraint.start_time.tm_mday,
					constraint.start_time.tm_mon + 1,
					constraint.start_time.tm_year + 109,
					constraint.start_time.tm_hour,
					constraint.start_time.tm_min,
					constraint.end_time.tm_mday,
					constraint.end_time.tm_mon + 1,
					constraint.end_time.tm_year + 109,
					constraint.end_time.tm_hour,
					constraint.end_time.tm_min);

		mp_drm_right_status_t *rs = _mp_drm_create_right_status_item(GET_SYS_STR("IDS_COM_POP_TIME"), validity_str);
		SAFE_FREE(validity_str);
		rs_list = g_list_append(rs_list, rs);
	}

	if (constraint.const_type.is_interval) {
		DEBUG_TRACE("DRM_INTERVAL\n");
		int val = constraint.interval_time.tm_hour;
		val += 24 * (constraint.interval_time.tm_mday + constraint.interval_time.tm_mon * 30 + constraint.interval_time.tm_year * 365);
		const char *format = GET_SYS_STR("IDS_COM_POP_PD_HOURS");
		validity_str = g_strdup_printf(format, val);
		mp_drm_right_status_t *rs = _mp_drm_create_right_status_item(GET_STR("Interval"), validity_str);
		SAFE_FREE(validity_str);
		rs_list = g_list_append(rs_list, rs);
	}

	if (constraint.const_type.is_timedcount) {
		validity_str = g_strdup_printf("%d left", constraint.timed_remaining_count);
		mp_drm_right_status_t *rs = _mp_drm_create_right_status_item(GET_STR("Timed count"), validity_str);
		SAFE_FREE(validity_str);
		rs_list = g_list_append(rs_list, rs);
	}

	if (constraint.const_type.is_accumulated) {
		const char *format = GET_SYS_STR("IDS_COM_POP_PD_HOURS");
		int acc_sec = constraint.accumulated_remaining_seconds;
		int hour = acc_sec / 3600;
		validity_str = g_strdup_printf(format, hour);
		mp_drm_right_status_t *rs = _mp_drm_create_right_status_item(GET_STR("Accumulate time"), validity_str);
		SAFE_FREE(validity_str);
		rs_list = g_list_append(rs_list, rs);
	}

	if (constraint.const_type.is_individual) {
		mp_debug("DRM_INDIVISUAL_ID [%s]", constraint.individual_id);
	}

	if (constraint.const_type.is_system) {
		mp_debug("DRM_SYSTEM [ID:%s, type:%d]", constraint.system_id, constraint.system_identity_type);
	}
	return rs_list;
}

void
mp_drm_free_right_status(GList *list)
{
	MP_CHECK(list);

	GList *current = list;
	while(current) {
		mp_drm_right_status_t *rs = current->data;
		if (rs) {
			SAFE_FREE(rs->type);
			SAFE_FREE(rs->validity);

			free(rs);
		}

		current = current->next;
	}

	g_list_free(list);
}

bool
mp_drm_request_setas_ringtone(const char *path, mp_drm_setas_request_type_e type)
{
	MP_CHECK_FALSE(path);

	bool ret = false;

	switch (type) {
	case SETAS_REQUEST_CHECK_STATUS:
	{
		drm_bool_type_e allowed = DRM_UNKNOWN;
		drm_action_allowed_data_s data;
		memset(&data, 0x0, sizeof(drm_action_allowed_data_s));
		SAFE_STRCPY(data.file_path, path);
		int res = drm_is_action_allowed(DRM_HAS_VALID_SETAS_STATUS, &data, &allowed);
		if (res == DRM_RETURN_SUCCESS) {
			if (allowed == DRM_TRUE)
				ret = true;
		} else {
			mp_error("drm_is_action_allowed().. [0x%x]", res);
		}

		break;
	}
	case SETAS_REQUEST_REGISTER:
	{
		drm_register_setas_info_s info;
		memset(&info, 0x0, sizeof(drm_register_setas_info_s));
		SAFE_STRCPY(info.file_path, path);
		info.setas_cat = DRM_SETAS_RINGTONE;
		int res = drm_process_request(DRM_REQUEST_TYPE_REGISTER_SETAS, (void *)&info, NULL);
		if (res == DRM_RETURN_SUCCESS)
			ret = true;
		else
			mp_error("drm_process_request().. [0x%x]", res);
		break;
	}
	case SETAS_REQUEST_UNREGISTER:
	{

		drm_unregister_setas_info_s info;
		memset(&info, 0x0, sizeof(drm_unregister_setas_info_s));
		SAFE_STRCPY(info.file_path, path);
		info.setas_cat = DRM_SETAS_RINGTONE;
		int res = drm_process_request(DRM_REQUEST_TYPE_UNREGISTER_SETAS, (void *)&info, NULL);
		if (res == DRM_RETURN_SUCCESS)
			ret = true;
		else
			mp_error("drm_process_request().. [0x%x]", res);
		break;
	}
	default:
		mp_debug("Not defined");
		break;
	}

	return ret;
}
