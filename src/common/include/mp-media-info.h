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

#ifndef __MP_MEDIA_INFO_H__
#define __MP_MEDIA_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define MAX_FILTER_LEN 4096

typedef struct mp_media_list_s *mp_media_list_h;
typedef struct mp_media_info_s *mp_media_info_h;
typedef struct filter_s *mp_filter_h;
typedef struct playlist_s *mp_playlist_h;

typedef bool(* mp_media_info_cb)(mp_media_info_h media, void *user_data);


typedef enum
{
	MP_MEDIA_CONTENT_COLLATE_DEFAULT	= 0,		/**< default collation BINARY */
	MP_MEDIA_CONTENT_COLLATE_NOCASE	= 1,		/**< collation NOCASE, not case sensitive */
	MP_MEDIA_CONTENT_COLLATE_RTRIM		= 2,		/**< collation RTRIM, trailing space characters are ignored */
} mp_media_content_collation_e;

typedef enum
{
	MP_MEDIA_TYPE_MUSIC,
	MP_MEDIA_TYPE_SOUND,
}mp_media_type_e;

typedef enum{
	MP_TRACK_ALL,							/**< All tracks*/
	MP_TRACK_BY_ALBUM,					/**< Album tracks*/
	MP_TRACK_BY_ARTIST_ALBUM,				/** < Albums which has special artist condition */
	MP_TRACK_BY_ARTIST,					/**< Artist tracks*/
	MP_TRACK_BY_ARTIST_GENRE,				/**< Genre tracks which has special artist condition*/
	MP_TRACK_BY_GENRE,						/**< Genre tracks*/
	MP_TRACK_BY_FOLDER,					/**< Genre tracks*/
	MP_TRACK_BY_YEAR,						/**< Year tracks*/
	MP_TRACK_BY_COMPOSER,				/**< Author tracks*/
	MP_TRACK_BY_FAVORITE,				/**< Toprating tracks*/
	MP_TRACK_BY_PLAYED_TIME,				/**< Recently played tracks*/
	MP_TRACK_BY_ADDED_TIME,				/**< Recently added tracks*/
	MP_TRACK_BY_PLAYED_COUNT,				/**< Most played tracks*/
	MP_TRACK_BY_PLAYLIST,					/**< User playlist tracks*/
}mp_track_type_e;

typedef enum{
	MP_GROUP_NONE,
	MP_GROUP_BY_ALBUM,					/**< Group by album*/
	MP_GROUP_BY_ARTIST,					/**< Group by artist*/
	MP_GROUP_BY_ARTIST_ALBUM,			/**< Group by album which has special artist condition*/
	MP_GROUP_BY_GENRE,					/**< Group by genre*/
	MP_GROUP_BY_FOLDER,					/**< Group by folder*/
	MP_GROUP_BY_YEAR,						/**< Group by year*/
	MP_GROUP_BY_COMPOSER,				/**< Group by author*/
	MP_GROUP_BY_PLAYLIST,
	MP_GROUP_BY_SYS_PLAYLIST,
	MP_GROUP_BY_SQUARE,
	MP_GROUP_BY_ALLSHARE,
}mp_group_type_e;

typedef enum{
	MP_SYS_PLST_NONE = -4,
	MP_SYS_PLST_MOST_PLAYED = -3,
	MP_SYS_PLST_RECENTELY_ADDED = -2,
	MP_SYS_PLST_RECENTELY_PLAYED = -1,
	MP_SYS_PLST_QUICK_LIST = 0,
}mp_sys_playlsit_id;

/*connection*/
int mp_media_info_connect(void);
int mp_media_info_disconnect(void);

/*filter*/
int mp_media_filter_create(mp_filter_h *filter);
int mp_media_filter_destory(mp_filter_h filter);
int mp_media_filter_set_offset(mp_filter_h filter, int offset, int count);
int mp_media_filter_set_order(mp_filter_h filter, bool asc, const char *order_keyword, mp_media_content_collation_e collation);
int mp_media_filter_set_condition(mp_filter_h filter, const char *condition, mp_media_content_collation_e collation );

/*media infomation*/
int mp_media_info_get_media_id(mp_media_info_h media, char **media_id);
int mp_media_info_get_file_path(mp_media_info_h media, char **path);
int mp_media_info_get_thumbnail_path(mp_media_info_h media, char **path);
int mp_media_info_get_favorite(mp_media_info_h media, bool *favorite);
int mp_media_info_is_drm(mp_media_info_h media, bool *drm);
int mp_media_info_get_title(mp_media_info_h media, char **title);
int mp_media_info_get_album(mp_media_info_h media, char **album);
int mp_media_info_get_genre(mp_media_info_h media, char **genre);
int mp_media_info_get_artist(mp_media_info_h media, char **artist);
int mp_media_info_get_composer(mp_media_info_h media, char **composer);
int mp_media_info_get_year(mp_media_info_h media, char **year);
int mp_media_info_get_copyright(mp_media_info_h media, char **copyright);
int mp_media_info_get_track_num(mp_media_info_h media, char **track_num);
int mp_media_info_get_format(mp_media_info_h media, char **format);
int mp_media_info_get_media_type(mp_media_info_h media, int *media_type);
int mp_media_info_get_bit_rate(mp_media_info_h media, int *bitrate);
int mp_media_info_get_sample_rate(mp_media_info_h media, int *sample_rate);
int mp_media_info_get_duration(mp_media_info_h media, int *duration);
int mp_media_info_get_played_time(mp_media_info_h media, time_t *time);
int mp_media_info_get_played_count(mp_media_info_h media, int *count);
int mp_media_info_get_added_time(mp_media_info_h media, time_t *time);
int mp_media_info_get_playlist_member_id(mp_media_info_h media, int *member_id);

int mp_media_info_set_favorite(mp_media_info_h media, bool favorite);
int mp_media_info_set_played_time(mp_media_info_h media, time_t time);
int mp_media_info_set_played_count(mp_media_info_h media, int count);
int mp_media_info_set_added_time(mp_media_info_h media, time_t time);

/*media*/
int mp_media_info_create(mp_media_info_h *media_list, const char *media_id);
int mp_media_info_create_by_path(mp_media_info_h *media_info, const char *file_path);
int mp_media_info_destroy(mp_media_info_h media_info);

/*media list*/
int mp_media_info_list_count(mp_track_type_e track_type, const char *type_string,const char *type_string2, const char *filter_string, int playlist_id, int *count);
int mp_media_info_list_count_w_filter(mp_track_type_e track_type, const char *folder_id, int playlist_id, mp_filter_h filter, int *count);
int mp_media_info_list_create(mp_media_list_h *media_list, mp_track_type_e track_type, const char *type_string, const char *type_string2, const char *filter_string, int playlist_id, int offset, int count);
int mp_media_info_list_create_w_filter(mp_track_type_e track_type, const char *folder_id, int playlist_id, mp_filter_h filter, mp_media_list_h *media_list);
int mp_media_info_list_destroy(mp_media_list_h media_list);
mp_media_info_h mp_media_info_list_nth_item(mp_media_list_h media_list, int index);

/*group list*/
int mp_media_info_group_list_count(mp_group_type_e group_type, const char *type_string, const char *filter_string, int *count);
int mp_media_info_group_list_count_w_filter(mp_group_type_e group_type, mp_filter_h filter, int *count);
int mp_media_info_group_list_create(mp_media_list_h *media_list, mp_group_type_e group_type, const char *type_string, const char *filter_string, int offset, int count);
int mp_media_info_group_list_create_w_filter(mp_filter_h filter, mp_group_type_e group_type, mp_media_list_h *media_list);
int mp_media_info_group_list_destroy(mp_media_list_h media_list);
mp_media_info_h mp_media_info_group_list_nth_item(mp_media_list_h media_list, int index);

int mp_media_info_group_get_main_info(mp_media_info_h media, char **main_info);
int mp_media_info_group_get_sub_info(mp_media_info_h media, char **sub_info);
int mp_media_info_group_get_playlist_id(mp_media_info_h media, int *playlist_id);
int mp_media_info_group_get_folder_id(mp_media_info_h media, char **folder_id);
int mp_media_info_group_get_thumbnail_path(mp_media_info_h media, char **path);

/*playlist list*/
int mp_media_info_playlist_get_id_by_name(const char *playlist_name, int *playlist_id);
int mp_media_info_playlist_insert_to_db(const char * name, int *playlist_id, mp_playlist_h *playlist_handle);
int mp_media_info_playlist_handle_destroy(mp_playlist_h playlist_handle);
int mp_media_info_playlist_delete_from_db(int playlist_id);
int mp_media_info_playlist_add_media(int playlist_id, const char *media_id);
int mp_media_info_playlist_remove_media(mp_media_info_h playlist, int memeber_id);
int mp_media_info_playlist_set_play_order(mp_media_info_h playlist, int memeber_id, int play_order);
int mp_media_info_playlist_update_db(mp_media_info_h playlist);
int mp_media_info_playlist_is_exist(const char *playlist_name, bool *exist);
int mp_media_info_playlist_unique_name(const char *orig_name, char *unique_name, size_t max_unique_name_length);
int mp_media_info_playlist_rename(mp_media_info_h playlist, const char *new_name);
int mp_media_info_playlist_get_play_order(mp_media_info_h playlist, int playlist_member_id, int * play_order);

//db sync
int mp_media_info_delete_from_db(const char *path);

#ifdef __cplusplus
}
#endif

#endif

