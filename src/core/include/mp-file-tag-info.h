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


#ifndef __mp_file_tag_info_H__
#define __mp_file_tag_info_H__

typedef struct _tag_info_t
{
	char *album;
	char *genre;
	char *author;
	char *artist;
	char *title;
	char *copyright;
	char *date;
	char *desc;
	char *albumart_path;
	char *track;
	char *rating;
	int duration;
	int audio_samplerate;
	int audio_bitrate;
	int audio_channel;
} mp_tag_info_t;

/* tag_info which must be freed with mp_file_tag_free() after use. */
int mp_file_tag_info_get_all_tag(const char *filename, mp_tag_info_t * tag_info);

char *mp_file_tag_info_get_genre(const char *filename);
int mp_file_tag_info_get_albumart(const char *filename, char **albumart_path);

void mp_file_tag_free(mp_tag_info_t * tag_info);
#endif //__mp_file_tag_info_H__
