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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <metadata_extractor.h>
#include <mm_file.h>
#include <mm_error.h>
#include <Ecore_File.h>
#include "mp-file-tag-info.h"
#include "mp-player-debug.h"

#define SAFE_FREE(x)       if(x) {free(x); x = NULL;}

/* tag_info which must be freed with mp_file_tag_free() after use. */
int
mp_file_tag_info_get_all_tag(const char *filename, mp_tag_info_t * tag_info)
{
	int ret = 0;

	if (!filename || !tag_info)
	{
		goto CATCH_ERROR;
	}

	memset(tag_info, 0x00, sizeof(mp_tag_info_t));

	metadata_extractor_h handle = NULL;
	ret = metadata_extractor_create(&handle);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		mp_error("metadata_extractor_create().. %d", ret);
		goto CATCH_ERROR;
	}

	ret = metadata_extractor_set_path(handle, filename);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE)
	{
		mp_error("metadata_extractor_set_path().. %d", ret);
		goto CATCH_ERROR;
	}

	char *value = NULL;
	ret = metadata_extractor_get_metadata(handle, METADATA_DURATION, &value);
	if (ret == METADATA_EXTRACTOR_ERROR_NONE && value) {
		tag_info->duration = atoi(value);
	}
	SAFE_FREE(value);

	ret = metadata_extractor_get_metadata(handle, METADATA_AUDIO_SAMPLERATE, &value);
	if (ret == METADATA_EXTRACTOR_ERROR_NONE && value) {
		tag_info->audio_samplerate = atoi(value);
	}
	SAFE_FREE(value);

	ret = metadata_extractor_get_metadata(handle, METADATA_AUDIO_BITRATE, &value);
	if (ret == METADATA_EXTRACTOR_ERROR_NONE && value) {
		tag_info->audio_bitrate = atoi(value);
	}
	SAFE_FREE(value);

	value = NULL;
	ret = metadata_extractor_get_metadata(handle, METADATA_AUDIO_CHANNELS, &value);
	if (ret == METADATA_EXTRACTOR_ERROR_NONE && value) {
		tag_info->audio_channel = atoi(value);
	}
	SAFE_FREE(value);

	metadata_extractor_get_metadata(handle, METADATA_ARTIST, &tag_info->artist);
	metadata_extractor_get_metadata(handle, METADATA_ALBUM, &tag_info->album);
	metadata_extractor_get_metadata(handle, METADATA_TITLE, &tag_info->title);
	metadata_extractor_get_metadata(handle, METADATA_GENRE, &tag_info->genre);
	metadata_extractor_get_metadata(handle, METADATA_AUTHOR, &tag_info->author);
	metadata_extractor_get_metadata(handle, METADATA_COPYRIGHT, &tag_info->copyright);
	metadata_extractor_get_metadata(handle, METADATA_DATE, &tag_info->date);
	metadata_extractor_get_metadata(handle, METADATA_DESCRIPTION, &tag_info->desc);
	metadata_extractor_get_metadata(handle, METADATA_TRACK_NUM, &tag_info->track);
	metadata_extractor_get_metadata(handle, METADATA_RATING, &tag_info->rating);

	if (!tag_info->title) {
		const char *name = ecore_file_file_get(filename);
		tag_info->title = ecore_file_strip_ext(name);
	}

	void *albumart = NULL;
	int albumart_size = 0;
	char *mime = NULL;
	ret = metadata_extractor_get_artwork(handle, &albumart, &albumart_size, &mime);
	if (ret == METADATA_EXTRACTOR_ERROR_NONE && albumart)
	{
		gchar *path = NULL;
		int fd = g_file_open_tmp(NULL, &path, NULL);

		if (fd != -1)
		{
			FILE *fp = fdopen(fd, "w");
			if (fp == NULL)
			{
				ERROR_TRACE("fail to fdopen()");
				close(fd);
			}
			else
			{
				int n = fwrite((unsigned char *)albumart, 1, albumart_size, fp);
				if (n != albumart_size)
				{
					ERROR_TRACE("fail to fwrite()");
					fclose(fp);
					close(fd);
				}
				else
				{
					fflush(fp);
					fclose(fp);
					close(fd);
				}
			}
		}
		tag_info->albumart_path = path;
	}
	SAFE_FREE(albumart);
	SAFE_FREE(mime);

	DEBUG_TRACE
		("file : %s\n duration: %d \n album: %s\n artist: %s\n title: %s\n genre: %s\n copyright:%s\n date: %s\n desc : %s\n author: %s\n albumart : %s",
		 filename, tag_info->duration, tag_info->album, tag_info->artist, tag_info->title, tag_info->genre,
		 tag_info->copyright, tag_info->date, tag_info->desc, tag_info->author, tag_info->albumart_path);

	if (handle)
		metadata_extractor_destroy(handle);

	return 0;

      CATCH_ERROR:
	if (handle)
		metadata_extractor_destroy(handle);

	return -1;
}

char *
mp_file_tag_info_get_genre(const char *filename)
{
	/* return value shold be freed */
	MP_CHECK_NULL(filename);

	int ret = METADATA_EXTRACTOR_ERROR_NONE;

	metadata_extractor_h handle = NULL;
	ret = metadata_extractor_create(&handle);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		mp_error("metadata_extractor_create().. %d", ret);
		goto CATCH_ERROR;
	}

	ret = metadata_extractor_set_path(handle, filename);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE)
	{
		mp_error("metadata_extractor_set_path().. %d", ret);
		goto CATCH_ERROR;
	}

	char *genre = NULL;
	ret = metadata_extractor_get_metadata(handle, METADATA_GENRE, &genre);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE)
	{
		mp_error("metadata_extractor_get_metadata().. %d", ret);
		SAFE_FREE(genre);
		goto CATCH_ERROR;
	}

	mp_debug("METADATA_GENRE = [%s]", genre);
	metadata_extractor_destroy(handle);
	return genre;

CATCH_ERROR:
	if (handle)
		metadata_extractor_destroy(handle);

	return NULL;
}


void
mp_file_tag_free(mp_tag_info_t * tag_info)
{
	if (tag_info == NULL)
		return;

	SAFE_FREE(tag_info->album);
	SAFE_FREE(tag_info->genre);
	SAFE_FREE(tag_info->author);
	SAFE_FREE(tag_info->artist);
	SAFE_FREE(tag_info->title);
	SAFE_FREE(tag_info->copyright);
	SAFE_FREE(tag_info->date);
	SAFE_FREE(tag_info->desc);
	SAFE_FREE(tag_info->albumart_path);
	SAFE_FREE(tag_info->track);
	SAFE_FREE(tag_info->rating);
	return;
}

/* albumart_path as a string which should be freed after use */
int
mp_file_tag_info_get_albumart(const char *filename, char **albumart_path)
{
	/* return value shold be freed */
	MP_CHECK_VAL(filename, -1);
	MP_CHECK_VAL(albumart_path, -1);

	int ret = METADATA_EXTRACTOR_ERROR_NONE;

	metadata_extractor_h handle = NULL;
	ret = metadata_extractor_create(&handle);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		mp_error("metadata_extractor_create().. %d", ret);
		goto CATCH_ERROR;
	}

	void *albumart = NULL;
	int albumart_size = 0;
	char *mime = NULL;
	ret = metadata_extractor_get_artwork(handle, &albumart, &albumart_size, &mime);
	if (ret == METADATA_EXTRACTOR_ERROR_NONE && albumart)
	{
		gchar *path = NULL;
		int fd = g_file_open_tmp(NULL, &path, NULL);

		if (fd != -1)
		{
			FILE *fp = fdopen(fd, "w");
			if (fp == NULL)
			{
				ERROR_TRACE("fail to fdopen()");
				close(fd);
			}
			else
			{
				int n = fwrite((unsigned char *)albumart, 1, albumart_size, fp);
				if (n != albumart_size)
				{
					ERROR_TRACE("fail to fwrite()");
					fclose(fp);
					close(fd);
				}
				else
				{
					fflush(fp);
					fclose(fp);
					close(fd);
				}
			}
		}
		*albumart_path = path;
	}
	SAFE_FREE(mime);
	SAFE_FREE(albumart);

	if (handle) {
		metadata_extractor_destroy(handle);
		handle = NULL;
	}

	return 0;

CATCH_ERROR:
	if (handle)
		metadata_extractor_destroy(handle);

	*albumart_path = NULL;
	return -1;
}
