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

#ifndef __MP_UTIL_H_
#define __MP_UTIL_H_

#include "music.h"
#include "mp-view-layout.h"

typedef enum
{
	MP_FILE_DELETE_ERR_USING = -2,
	MP_FILE_DELETE_ERR_REMOVE_FAIL = -1,
	MP_FILE_DELETE_ERR_INVALID_FID = 0,
	MP_FILE_DELETE_ERR_NONE,
}mp_file_delete_err_t;

bool			mp_util_check_uri_available(const char *uri);
bool 			mp_check_file_exist (const char *path);
void 			mp_util_format_duration(char *time, int ms);
bool				mp_util_add_to_playlist_by_key(int playlist_id, char *key_id);
Evas_Object * 	mp_util_create_thumb_icon(Evas_Object *obj, const char *path, int w, int h);
const char* 		mp_util_get_index(const char *p);
const char* 		mp_util_get_second_index(const char *p);
Evas_Object * 			mp_util_create_selectioninfo_with_count(Evas_Object *parent, int count);
void mp_util_post_status_message(struct appdata *ad, const char *text);

char *			mp_util_get_new_playlist_name (void);
mp_file_delete_err_t 	mp_util_delete_track(void *data, char *fid, char *file_path, bool show_popup);

int				mp_util_share_via_bt(const char *formed_path, int file_cnt);
int 			mp_util_file_is_in_phone_memory(const char *path);

char *mp_util_get_fid_by_handle(mp_layout_data_t *layout_data, mp_media_info_h record);
char*  			mp_util_get_path_by_handle(mp_layout_data_t *layout_data , mp_media_info_h record);
 char* 			mp_util_isf_get_edited_str(Evas_Object *isf_entry, bool permit_first_blank);
 int				mp_util_create_playlist(struct appdata *ad, char *name, mp_playlist_h *playlist_handle);

bool 			mp_util_set_screen_mode(void *data , int mode);

bool mp_util_launch_browser(const char *url, struct appdata *ad);


#define mp_object_free(obj)	\
do {						\
	if(obj != NULL) {		\
		g_free(obj);		\
		obj = NULL;			\
	}						\
}while(0)

#define MMC_PATH	MP_MMC_ROOT_PATH

gchar *mp_util_get_utf8_initial(const char *name);
gchar * mp_get_new_playlist_name (void);
gchar *mp_parse_get_title_from_path (const gchar *path);
char *mp_util_get_title_from_path (const char *path);
bool	mp_util_is_playlist_name_valid(char *name);
bool	mp_util_get_playlist_data(mp_layout_data_t *layout_data, int *index, const char *playlist_name);
void mp_util_set_library_controlbar_items(void *data);

bool mp_util_get_uri_from_app_svc(service_h service, struct appdata *ad, char **path);
void mp_util_reset_genlist_mode_item(Evas_Object *genlist);
#ifndef MP_SOUND_PLAYER
view_data_t * mp_util_get_view_data(struct appdata *ad);
mp_layout_data_t* mp_util_get_layout_data(Evas_Object* obj);
#endif

bool mp_util_is_image_valid(Evas *evas, const char *path);
char *mp_util_shorten_path(char *path_info);

#ifndef MP_SOUND_PLAYER
void mp_util_unset_rename(mp_layout_data_t * layout_data);
#endif
bool mp_util_is_db_updating(void);
bool mp_util_is_bt_connected(void);
bool mp_util_is_earjack_inserted(void);
void mp_util_get_sound_path(mp_snd_path *snd_path);

const char * mp_util_search_markup_keyword(const char *string, char *searchword, bool *result);

bool mp_util_is_other_player_playing();

int mp_commmon_check_rotate_lock(void);
int mp_check_battery_available(void);
int mp_check_mass_storage_mode(void);

bool mp_util_sleep_lock_set(bool lock);
bool mp_util_is_nfc_feature_on(void);

void mp_util_strncpy_safe(char *x_dst, const char *x_src, int max_len);
bool mp_util_edit_image(Evas *evas, Evas_Object *src_image, const char *path, mp_playing_view_bg_capture_mode mode);

void mp_util_free_track_info(mp_track_info_t *track_info);
void mp_util_load_track_info(struct appdata *ad, mp_plst_item *cur_item, mp_track_info_t **info);
void mp_util_append_media_list_item_to_playlist(mp_plst_mgr *playlist_mgr, mp_media_list_h media_list, int count, int current_index, const char *uri);
char* mp_util_get_fid_by_full_path(const char *full_path);

#endif //__MP_UTIL_H_

