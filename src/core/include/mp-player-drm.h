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

#ifndef __MP_PLAYER_DRM_H_
#define __MP_PLAYER_DRM_H_

typedef struct
{
	bool forward;
	int version;
	int constraint_type;
	char validity[256];
	char description[256];
} mp_drm;

typedef struct
{
	char *type;
	char *validity;
} mp_drm_right_status_t;

typedef enum
{
	SETAS_REQUEST_CHECK_STATUS,
	SETAS_REQUEST_REGISTER,
	SETAS_REQUEST_UNREGISTER,
} mp_drm_setas_request_type_e;

typedef enum {
	DRM_CONTENT_INFO_NULL = -1,
	DRM_CONTENT_INFO_AUTHOR,
	DRM_CONTENT_INFO_RIGHTS_URL,
	DRM_CONTENT_INFO_DESCRIPTION,
} drm_content_info_e;

enum {
	MP_DRM_CONSTRAINT_UNLIMITED			= 0x0,
	MP_DRM_CONSTRAINT_COUNT				= 0x01,
	MP_DRM_CONSTRAINT_DATE_TIME			= 0x02,
	MP_DRM_CONSTRAINT_INTERVAL			= 0x04,
	MP_DRM_CONSTRAINT_TIMED_COUNT		= 0x08,
	MP_DRM_CONSTRAINT_ACCUMLATED_TIME	= 0x10,
};

typedef struct {
	int constraints;

	int remaining_count;
	bool date_time_expired;
	int remaining_interval_sec;
	int remaining_timed_count;
	int remaining_acc_sec;
} mp_constraints_info_s;

bool mp_drm_get_content_info(const char *path, drm_content_info_e first_info, ...);
void mp_drm_set_notify(void *data, char *message);
#ifdef MP_FEATURE_DRM_CONSUMPTION
void mp_drm_set_consumption(bool enabled);
bool mp_drm_get_consumption(void);
void mp_drm_start_consumption(char *path);
void mp_drm_pause_consumption(void);
void mp_drm_resume_consumption(void);
void mp_drm_stop_consumption(void);
#endif
bool mp_drm_file_right(char *path);
bool mp_drm_has_valid_ro(char *path);
bool mp_drm_get_description(void *data, char *path);
bool mp_drm_check_forward(void *data, char *path);
bool mp_drm_get_left_ro_info(const char *path, mp_constraints_info_s *info);
bool mp_drm_check_left_ro(void *data, char *path);
bool mp_drm_check_expiration(char *path, bool expired);
bool mp_drm_check_foward_lock(char *path);
GList* mp_drm_get_right_status(const char *path);
void mp_drm_free_right_status(GList *list);
bool mp_drm_request_setas_ringtone(const char *path, mp_drm_setas_request_type_e type);


#endif /*__MP_PLAYER_DRM_H_*/
