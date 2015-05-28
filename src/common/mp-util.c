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
#include "mp-util.h"
#include "mp-file-tag-info.h"
#include "mp-playlist-mgr.h"
#include "mp-ug-launch.h"
#include "mp-widget.h"
#include <app.h>
#include <sound_manager.h>
#include <player.h>
#include <power.h>
#include <notification.h>
#ifdef MP_SOUND_PLAYER
#include "sp-view-manager.h"
#include "mp-library.h"
#else
#include "mp-common.h"
#include "mp-library.h"
#endif

#define MP_UTIL_FILE_PREFIX             "file://"

#define SINGLE_BYTE_MAX 0x7F

struct index_s
{
	const char *index;
	unsigned short start;
	unsigned short end;
};

static struct index_s multi_index[] = {
	{"\xE3\x84\xB1", 0xAC00, 0xB098},	/* Kiyeok + A */
	{"\xE3\x84\xB4", 0xB098, 0xB2E4},	/* Nieun + A */
	{"\xE3\x84\xB7", 0xB2E4, 0xB77C},
	{"\xE3\x84\xB9", 0xB77C, 0xB9C8},
	{"\xE3\x85\x81", 0xB9C8, 0xBC14},
	{"\xE3\x85\x82", 0xBC14, 0xC0AC},
	{"\xE3\x85\x85", 0xC0AC, 0xC544},
	{"\xE3\x85\x87", 0xC544, 0xC790},
	{"\xE3\x85\x88", 0xC790, 0xCC28},
	{"\xE3\x85\x8A", 0xCC28, 0xCE74},
	{"\xE3\x85\x8B", 0xCE74, 0xD0C0},
	{"\xE3\x85\x8C", 0xD0C0, 0xD30C},
	{"\xE3\x85\x8D", 0xD30C, 0xD558},
	{"\xE3\x85\x8E", 0xD558, 0xD7A4},	/* Hieuh + A */

	{"\xE3\x84\xB1", 0x3131, 0x3134},	/* Kiyeok */
	{"\xE3\x84\xB4", 0x3134, 0x3137},	/* Nieun */
	{"\xE3\x84\xB7", 0x3137, 0x3139},
	{"\xE3\x84\xB9", 0x3139, 0x3141},
	{"\xE3\x85\x81", 0x3141, 0x3142},
	{"\xE3\x85\x82", 0x3142, 0x3145},
	{"\xE3\x85\x85", 0x3145, 0x3147},
	{"\xE3\x85\x87", 0x3147, 0x3148},
	{"\xE3\x85\x88", 0x3148, 0x314A},
	{"\xE3\x85\x8A", 0x314A, 0x314B},
	{"\xE3\x85\x8B", 0x314B, 0x314C},
	{"\xE3\x85\x8C", 0x314C, 0x314D},
	{"\xE3\x85\x8D", 0x314D, 0x314E},
	{"\xE3\x85\x8E", 0x314E, 0x314F},	/* Hieuh */
};

static char *single_upper_index[] = {
	"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
	"O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
};

static char *single_lower_index[] = {
	"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",
	"o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"
};


static char *single_numeric_index[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "."
};

static const char *kor_sec[] = {
	"\xe3\x85\x8f",		/* A */
	"\xe3\x85\x90",		/* AE */
	"\xe3\x85\x91",		/* YA */
	"\xe3\x85\x92",
	"\xe3\x85\x93",
	"\xe3\x85\x94",
	"\xe3\x85\x95",
	"\xe3\x85\x96",
	"\xe3\x85\x97",
	"\xe3\x85\x98",
	"\xe3\x85\x99",
	"\xe3\x85\x9a",
	"\xe3\x85\x9b",
	"\xe3\x85\x9c",
	"\xe3\x85\x9d",
	"\xe3\x85\x9e",
	"\xe3\x85\x9f",
	"\xe3\x85\xa0",
	"\xe3\x85\xa1",
	"\xe3\x85\xa2",
	"\xe3\x85\xa3",
};

static unsigned char mask_len[] = {
	0x80, /* 1000 0000 */ 0x00,	/* 0xxx xxxx */
	0xE0, /* 1110 0000 */ 0xC0,	/* 110x xxxx */
	0xF0, /* 1111 0000 */ 0xE0,	/* 1110 xxxx */
	0xF8, /* 1111 1000 */ 0xF0,	/* 1111 0xxx */
	0xFC, /* 1111 1100 */ 0xF8,	/* 1111 10xx */
	0xFE, /* 1111 1110 */ 0xFC,	/* 1111 110x */
};

static int
_mp_util_get_len(const char *p)
{
	int i, r = -1;
	unsigned char c;

	if (p)
	{
		c = *p;
		for (i = 0; i < sizeof(mask_len) / sizeof(char); i = i + 2)
		{
			if ((c & mask_len[i]) == mask_len[i + 1])
			{
				r = (i >> 1) + 1;
				break;
			}
		}
	}

	return r;
}

static unsigned short
_mp_util_utf8_to_ucs2(const char *p)
{
	unsigned short r = 0;
	int len;

	len = _mp_util_get_len(p);
	if (len == -1 || len > 3)
	{
		return r;
	}

	switch (len)
	{
	case 1:
		{
			r = *p & 0x7F;
			break;
		}
	case 2:
		{
			r = *p & 0x1F;
			break;
		}
	case 3:
		{
			r = *p & 0x0F;
			break;
		}
	default:
		{
			break;
		}
	}

	while (len > 1)
	{
		r = r << 6;
		p++;
		r |= *p & 0x3F;
		len--;
	}

	return r;
}

static const char *
_mp_util_get_single(const char *p)
{
	int c = (int)*p;

	if (islower(c) != 0)
	{
		return single_lower_index[c - 'a'];
	}
	else if (isupper(c) != 0)
	{
		return single_upper_index[c - 'A'];
	}
	else if (48 <= c && 57 >= c)
	{
		return single_numeric_index[c - '0'];
	}
	else
	{
		return single_numeric_index[10];
	}

	return NULL;
}

static const char *
_mp_util_get_multi(unsigned short u)
{
	int i;

	for (i = 0; i < sizeof(multi_index) / sizeof(struct index_s); i++)
	{
		if (u >= multi_index[i].start && u < multi_index[i].end)
		{
			return multi_index[i].index;
		}
	}
	return NULL;
}

static char *
_mp_util_get_next_char(const char *p)
{
	int n;

	MP_CHECK_NULL(p);

	n = _mp_util_get_len(p);
	if (n == -1)
	{
		return NULL;
	}

	if (strlen(p) < n)
	{
		return NULL;
	}

	DEBUG_TRACE("%s", &p[n]);

	return (char *)&p[n];
}


static const char *
_mp_util_get_second_kor(unsigned short u)
{
	unsigned short t;

	t = u - 0xAC00;
	t = (t / 28) % 21;

	return kor_sec[t];
}

void
mp_util_format_duration(char *time, int ms)
{
	int sec = (ms + 500) / 1000;
	int min = sec / 60;

	if(min >= 60)
	{
		int hour = min / 60;
		snprintf(time, TIME_FORMAT_LEN, "%02u:%02u:%02u", hour, min % 60, sec % 60);
	}
	else
		snprintf(time, TIME_FORMAT_LEN, "%02u:%02u", min, sec % 60);
}

const char *
mp_util_get_index(const char *p)
{
	if (p == NULL)
	{
		return NULL;
	}

	if ((unsigned char)*p < SINGLE_BYTE_MAX)
	{
		return _mp_util_get_single(p);
	}

	return _mp_util_get_multi(_mp_util_utf8_to_ucs2(p));
}

const char *
mp_util_get_second_index(const char *p)
{
	unsigned short u2;

	if (p == NULL)
	{
		return NULL;
	}

	if ((unsigned char)*p < SINGLE_BYTE_MAX)
	{
		return mp_util_get_index(_mp_util_get_next_char(p));
	}

	u2 = _mp_util_utf8_to_ucs2(p);
	if (u2 >= 0xAC00 && u2 < 0xD7A4)
	{
		return _mp_util_get_second_kor(u2);
	}

	return mp_util_get_index(_mp_util_get_next_char(p));
}

bool
mp_util_add_to_playlist_by_key(int playlist_id, char *key_id)
{
	int err;
	{
		err = mp_media_info_playlist_add_media(playlist_id, key_id);
		if (err != 0)
		{
			ERROR_TRACE("Error in mp_media_info_playlist_add_media (%d)\n", err);
			return FALSE;
		}
	}
	return TRUE;
}

Evas_Object *
mp_util_create_thumb_icon(Evas_Object * obj, const char *path, int w, int h)
{
	mp_layout_data_t *layout_data = evas_object_data_get(obj, "layout_data");

	Evas_Object *thumbnail = elm_bg_add(obj);
	elm_bg_load_size_set(thumbnail, w, h);

	Evas *evas = evas_object_evas_get(obj);
	MP_CHECK_NULL(evas);
	if (mp_util_is_image_valid(evas, path))
	{
		elm_bg_file_set(thumbnail, path, NULL);
	}
	else
	{
		if (layout_data->category == MP_LAYOUT_GROUP_LIST
		    && layout_data->view_data->view_type == MP_VIEW_TYPE_FOLDER)
		{
			elm_bg_file_set(thumbnail, DEFAULT_THUMBNAIL_FOLDER, NULL);
		}
		else
		{
			elm_bg_file_set(thumbnail, DEFAULT_THUMBNAIL, NULL);
		}
	}

	evas_object_size_hint_aspect_set(thumbnail, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	//evas_object_show(thumbnail);
	return thumbnail;
}

static void
_mp_util_notify_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	Evas_Object *parent = data;
	MP_CHECK(parent);

	evas_object_data_set(parent, "selectioninfo", NULL);
	elm_object_signal_emit(parent, "hide,selection,info", "elm");
}

static void
_mp_util_notify_parent_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	DEBUG_TRACE("");
	Ecore_Timer *timer = NULL;
	timer = evas_object_data_get(obj, "selectioninfo_timer");
	if(timer)
		ecore_timer_del(timer);
}

static Evas_Object *
_mp_util_create_selection_info(Evas_Object *parent, const char *text)
{
	Evas_Object *notify_layout = elm_layout_add(parent);;

	evas_object_size_hint_weight_set(notify_layout,
		                                 EVAS_HINT_EXPAND,
		                                 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(notify_layout,
		                                EVAS_HINT_FILL,
		                                EVAS_HINT_FILL);
	elm_layout_theme_set(notify_layout, "standard", "selectioninfo", "center_text");
	elm_object_part_text_set(notify_layout, "elm.text", text);
	elm_object_part_content_set(parent,
		                            "elm.swallow.content.selectioninfo",
		                            notify_layout);

	elm_object_signal_emit(parent, "show,selection,info", "elm");

	evas_object_show(notify_layout);

	return notify_layout;
}

Evas_Object *
mp_util_create_selectioninfo_with_count(Evas_Object *parent, int count)
{
	MP_CHECK_NULL(parent);
	Evas_Object *notify = NULL;
	notify = evas_object_data_get(parent, "selectioninfo");

	if(count)
	{
		char text[128];
		snprintf(text, 128, "%s (%d)", GET_SYS_STR("IDS_COM_POP_SELECTED"), count);

		if(!notify)
		{
			notify = _mp_util_create_selection_info(parent, text);
			evas_object_data_set(parent, "selectioninfo", notify);
			evas_object_event_callback_add(notify, EVAS_CALLBACK_DEL, _mp_util_notify_del_cb, parent);
		}
		else
			elm_object_part_text_set(notify, "elm.text", text);
	}
	else
		evas_object_del(notify);

	return notify;

}

static Eina_Bool
_mp_util_selectioninfo_timer_cb(void *user_data)
{
	Evas_Object *obj = user_data;
	Evas_Object *si = NULL;
	evas_object_data_set(obj, "selectioninfo_timer", NULL);

	si = evas_object_data_get(obj, "selectioninfo");
	evas_object_del(si);

	evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL, _mp_util_notify_parent_del_cb);

	return FALSE;
}

void
mp_util_post_status_message(struct appdata *ad, const char *text)
{
	MP_CHECK(ad);
	 int ret = notification_status_message_post(text);
	 if (ret != 0) {
                ERROR_TRACE("status_message_post()... [0x%x]", ret);
        }
}

int
mp_util_share_via_email(const char *formed_path, void *data)
{
	if (mp_ug_email_attatch_file(formed_path, data))
		return -1;

	return 0;
}

char *
mp_util_get_new_playlist_name(void)
{
	char unique_name[MP_PLAYLIST_NAME_SIZE] = "\0";
	int ret = 0;
	ret = mp_media_info_playlist_unique_name("My playlist", unique_name, MP_PLAYLIST_NAME_SIZE);
	if (ret == 0)
	{
		if (strlen(unique_name) <= 0)
		{
			ERROR_TRACE("playlist name is NULL");
			return NULL;
		}
		else
		{
			return g_strdup(unique_name);
		}
	}
	else
	{
		ERROR_TRACE("fail to mp_media_info_playlist_unique_name() : error code [%x] ", ret);
		return NULL;
	}

	return NULL;
}

char *
mp_util_get_fid_by_handle(mp_layout_data_t * layout_data, mp_media_info_h record)
{
	MP_CHECK_FALSE(record);

	int ret = 0;
	char *fid = NULL;

	ret = mp_media_info_get_media_id(record, &fid);

	return fid;
}

char *
mp_util_get_path_by_handle(mp_layout_data_t * layout_data, mp_media_info_h record)
{
	MP_CHECK_NULL(record);

	char *path = NULL;

	int ret = 0;

		ret = mp_media_info_get_file_path(record, &path);

	return path;
}


mp_file_delete_err_t
mp_util_delete_track(void *data, char *fid, char *file_path, bool show_popup)
{
	struct appdata *ad = (struct appdata *)data;
	int ret = 0;
	mp_media_info_h item = NULL;

	DEBUG_TRACE("music id = %s, path: %s", fid, file_path);
	MP_CHECK_VAL(fid, MP_FILE_DELETE_ERR_INVALID_FID);

	char *path = NULL;
	if (!file_path)
	{
		mp_media_info_create(&item, fid);
		mp_media_info_get_file_path(item, &path);
	}
	else
		path = file_path;

	if (!path) {
		if (item)
			mp_media_info_destroy(item);
		return MP_FILE_DELETE_ERR_INVALID_FID;
	}

	DEBUG_TRACE("path: %s", path);
	ret = remove(path);
	media_info_delete_from_db(fid);

	if(item) {
		mp_media_info_destroy(item);
		item = NULL;
	}

	if (ret < 0)
	{
		ERROR_TRACE("fail to unlink file, ret: %d", ret);
		if (show_popup)
			mp_widget_text_popup(ad, GET_SYS_STR("IDS_COM_POP_FAILED"));
		return MP_FILE_DELETE_ERR_REMOVE_FAIL;
	}

	return MP_FILE_DELETE_ERR_NONE;
}

int
mp_util_file_is_in_phone_memory(const char *path)
{
	MP_CHECK_VAL(path, 0);
	if (!strncmp(MP_PHONE_ROOT_PATH, path, strlen(MP_PHONE_ROOT_PATH)))
		return 1;
	else
		return 0;
}

// return value must be freed.
char *
mp_util_isf_get_edited_str(Evas_Object * isf_entry, bool permit_first_blank)
{

	const char *buf = NULL;
	char *strip_msg = NULL;
	int strip_len = 0;

	if (!isf_entry)
		return strdup("");
	buf = elm_entry_entry_get(isf_entry);
	if (!buf)
		return strdup("");

	strip_msg = elm_entry_markup_to_utf8(buf);

	if (strip_msg != NULL)
	{
		strip_len = strlen(strip_msg);

		if (strip_len > 0)
		{
			if (strip_msg[0] == ' ' && !permit_first_blank)	//start with space
			{
				DEBUG_TRACE("Filename should not be started with blank");
				free(strip_msg);
				return strdup("");
			}

			if (strip_msg[strip_len - 1] == '\n' || strip_msg[strip_len - 1] == '\r')
			{
				strip_msg[strip_len - 1] = '\0';
			}
			DEBUG_TRACE("=====  The new edited str = %s", strip_msg);
			return strip_msg;
		}
		else
		{
			DEBUG_TRACE(" strip_msg length is [%d], strip_msg [%s]", strip_len, strip_msg);
			return strip_msg;
		}
	}
	else
	{
		DEBUG_TRACE("strip_msg is NULL");
		return strdup("");
	}
}

bool
mp_util_set_screen_mode(void *data, int mode)
{
	struct appdata *ad = data;

	ad->current_appcore_rm = mode;	//set current appcore rm
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &ad->screen_width, &ad->screen_height);	//get current screen height width

	if (mode == APP_DEVICE_ORIENTATION_270 || mode == APP_DEVICE_ORIENTATION_90)
	{
		ad->screen_mode = MP_SCREEN_MODE_LANDSCAPE;
		mp_debug("Set MP_SCREEN_MODE_LANDSCAPE");
	}
	else if (mode == APP_DEVICE_ORIENTATION_0 || mode == APP_DEVICE_ORIENTATION_180)
	{
		ad->screen_mode = MP_SCREEN_MODE_PORTRAIT;
		mp_debug("Set MP_SCREEN_MODE_PORTRAIT");
	}
	return true;
}

bool
mp_util_check_uri_available(const char *uri)
{
	if (uri == NULL || strlen(uri) == 0)
	{
		return FALSE;
	}

	if (uri == strstr(uri, "http://") || uri == strstr(uri, "https://")
			|| uri == strstr(uri, "rtp://") || uri == strstr(uri, "rtsp://")) {
		DEBUG_TRACE("Streaming URI... OK");
		return TRUE;
	}
	else
	{
		DEBUG_TRACE("uri check failed : [%s]", uri);
		return FALSE;
	}
}

bool
mp_check_file_exist(const char *path)
{
	if (path == NULL || strlen(path) == 0)
	{
		return FALSE;
	}

	if(strstr(path,MP_UTIL_FILE_PREFIX))
    {
		if (!g_file_test(path+strlen(MP_UTIL_FILE_PREFIX), G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		if(!g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			return FALSE;
		}
		return TRUE;
	}
	DEBUG_TRACE("file check okay : [%s]", path);
	return TRUE;
}

bool
mp_util_launch_browser(const char *url, struct appdata * ad)
{
	DEBUG_TRACE("url: %s", url);

	service_h service;
	bool res;
	service_create(&service);
	service_set_operation(service, SERVICE_OPERATION_DEFAULT);
	service_set_package(service, "org.tizen.browser");
	service_set_uri(service, url);

	if (service_send_launch_request(service, NULL, NULL) == SERVICE_ERROR_NONE) {
		DEBUG_TRACE("Succeeded to launch a calculator app.");
		res = true;
	} else {
		DEBUG_TRACE("Failed to launch a calculator app.");
		res = false;
	}
	service_destroy(service);

	return res;
}


//korean initial consonant
//returns mallocated memory, have to free it after use
gchar *
mp_util_get_utf8_initial(const char *name)
{
	gunichar first;
	char *next = NULL;
	if (name == NULL)
		return NULL;

	if (g_utf8_strlen(name, -1) <= 0)
	{
		return strdup("");
	}

	first = g_utf8_get_char_validated(name, -1);
	if (first == (gunichar) - 1 || first == (gunichar) - 2) {
		DEBUG_TRACE ("failed to convert a sequence of bytes encoded as UTF-8 to a Unicode character.");
		return strdup("");
	}

	next = (char *)name;

	while (!g_unichar_isgraph(first))
	{
		next = g_utf8_next_char(next);
		first = g_utf8_get_char_validated(next, -1);
		if (first == (gunichar) - 1 || first == (gunichar) - 2) {
			DEBUG_TRACE ("failed to convert a sequence of bytes encoded as UTF-8 to a Unicode character.");
			return strdup("");
		}
	}

	if (first >= 0xAC00 && first <= 0xD7A3)
	{			//korean
		int index = 0;
		index = ((((first - 0xAC00) - ((first - 0xAC00) % 28)) / 28) / 21);
		if (index < 20 && index >= 0)
		{
			const gunichar chosung[20] = { 0x3131, 0x3132, 0x3134, 0x3137, 0x3138,
				0x3139, 0x3141, 0x3142, 0x3143, 0x3145,
				0x3146, 0x3147, 0x3148, 0x3149, 0x314a,
				0x314b, 0x314c, 0x314d, 0x314e, 0
			};

			gchar result[10] = { 0, };
			int len = 0;
			len = g_unichar_to_utf8(chosung[index], result);
			return strndup(result, len + 1);
		}
	}
	else
	{
		gchar result[10] = { 0, };
		int len = 0;
		len = g_unichar_to_utf8(first, result);
		return strndup(result, len + 1);
	}
	return NULL;
}


char *
mp_util_get_title_from_path(const char *path)
{
	gchar *file_ext = NULL, *file_name = NULL, *title = NULL;

	if (path == NULL || strlen(path) == 0)
	{
		return NULL;
	}

	file_name = g_path_get_basename(path);
	if (file_name)
	{
		file_ext = g_strrstr(file_name, ".");
		if (file_ext)
		{
			title = g_strndup(file_name, strlen(file_name) - strlen(file_ext));
		}
		free(file_name);
	}
	DEBUG_TRACE("title = %s\n", title);
	return title;
}

bool
mp_util_is_playlist_name_valid(char *name)
{
	MP_CHECK_NULL(name);

	char *test_space = strdup(name);
	if (strlen(g_strchug(test_space)) == 0)
	{
		IF_FREE(test_space);
		return FALSE;
	}
	IF_FREE(test_space);
	return TRUE;
}

int
mp_util_create_playlist(struct appdata *ad, char *name, mp_playlist_h *playlist_handle)
{
	MP_CHECK_VAL(ad, -1);
	MP_CHECK_VAL(name, -1);

	int plst_uid = -1;

	if (!mp_util_is_playlist_name_valid(name))
	{
		mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_CREATE_PLAYLIST"));
		return -1;
	}

	bool exist = false;
	int ret = mp_media_info_playlist_is_exist(name, &exist);
	if(ret != 0)
	{
		ERROR_TRACE("Fail to get playlist count by name: %d", ret);
		mp_widget_text_popup(ad, GET_STR("IDS_MUSIC_POP_UNABLE_CREATE_PLAYLIST"));
		return -1;
	}

	if (exist)
	{
		char buf[256] = { 0, };

		snprintf(buf, sizeof(buf), "Playlist name %s is exist", name);
		mp_widget_text_popup(ad, buf);
		return -1;
	}

	ret = mp_media_info_playlist_insert_to_db(name, &plst_uid, playlist_handle);
	if(ret != 0)
	{
		ERROR_TRACE("Fail to get playlist count by name: %d", ret);
		mp_widget_text_popup(ad, GET_SYS_STR("IDS_COM_BODY_UNABLE_TO_ADD"));
		*playlist_handle = NULL;
		return -1;
	}

	return plst_uid;
}

bool
mp_util_get_playlist_data(mp_layout_data_t * layout_data, int *index, const char *playlist_name)
{
	MP_CHECK_FALSE(layout_data);
	struct appdata *ad = layout_data->ad;
	MP_CHECK_FALSE(ad);

	int ret = 0;
	int playlist_id = 0;

	ret = mp_media_info_playlist_get_id_by_name(playlist_name, &playlist_id);
	mp_retvm_if(ret != 0, false, "ret: %d, playlist_name: %s", ret, playlist_name);
	layout_data->playlist_id = playlist_id;

	ret = mp_media_info_list_count(MP_TRACK_BY_PLAYLIST, NULL, NULL, layout_data->filter_str, layout_data->playlist_id, &(layout_data->item_count));
	mp_retvm_if(ret != 0, false, "ret: %d", ret);
	if (layout_data->item_count <= 0)
	{
		DEBUG_TRACE("Recently played tracks were removed...");
		return false;
	}
	ret = mp_media_info_list_create(&layout_data->svc_handle, MP_TRACK_BY_PLAYLIST,
		NULL, NULL, layout_data->filter_str, layout_data->playlist_id, 0, layout_data->item_count);
	if (ret != 0)
	{
		WARN_TRACE("fail to mp_media_info_list_create: %d", ret);
		return false;
	}
	return true;
}

#ifndef MP_SOUND_PLAYER
void
_mp_util_set_tabbar_item(Evas_Object *control_bar, Elm_Object_Item **item,
						 Elm_Object_Item **after, int enabled, char *icon, char *label, void *data)
{
	if (*item && !enabled)
	{
		mp_language_mgr_unregister_object_item(*item);
		elm_object_item_del(*item);
		*item = NULL;
	}
	else if(!(*item) && enabled)
	{
		*after = *item =
			elm_toolbar_item_insert_after(control_bar, *after, icon, label, mp_library_view_change_cb, data);
	}
	else if(*item && enabled)
	{
		*after = *item;
	}
	DEBUG_TRACE("after: 0x%x, item: 0x%x, enabled: %d ", *after ,*item, enabled);
}

void
mp_util_set_library_controlbar_items(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	MP_CHECK(ad);
	MP_CHECK(ad->library);

	Elm_Object_Item *prev_selected = elm_toolbar_selected_item_get(ad->tabbar);

	Elm_Object_Item *after = ad->library->ctltab_plist;
	_mp_util_set_tabbar_item(ad->tabbar, &(ad->library->ctltab_album), &after,
						true, MP_CTRBAR_ICON_ALBUM, GET_STR(STR_MP_ALBUMS), ad);

	_mp_util_set_tabbar_item(ad->tabbar, &(ad->library->ctltab_artist), &after,
						true, MP_CTRBAR_ICON_ARTIST, GET_STR(STR_MP_ARTISTS), ad);

	_mp_util_set_tabbar_item(ad->tabbar, &(ad->library->ctltab_genres), &after,
						true, MP_CTRBAR_ICON_GENRE, GET_STR(STR_MP_GENRES), ad);

	_mp_util_set_tabbar_item(ad->tabbar, &(ad->library->ctltab_composer), &after,
						true, MP_CTRBAR_ICON_COMPOSER, GET_STR(STR_MP_COMPOSERS), ad);

	_mp_util_set_tabbar_item(ad->tabbar, &(ad->library->ctltab_year), &after,
						true, MP_CTRBAR_ICON_YEAR, GET_STR(STR_MP_YEARS), ad);

	_mp_util_set_tabbar_item(ad->tabbar, &(ad->library->ctltab_folder), &after,
						true, MP_CTRBAR_ICON_FOLDER, GET_SYS_STR(STR_MP_FOLDERS), ad);


	if (prev_selected != elm_toolbar_selected_item_get(ad->tabbar)) {
		mp_debug("prev selected item is deleted.. select song tab");
		elm_toolbar_item_selected_set(ad->library->ctltab_songs, EINA_TRUE);
	}
}
#endif

bool
mp_util_get_uri_from_app_svc(service_h service, struct appdata *ad, char **path)
{
	char *uri = NULL;
	char *operation = NULL;
	char *mime = NULL;

	MP_CHECK_FALSE(service);

	service_get_operation(service, &operation);
	DEBUG_TRACE("operation: %s", operation);

	if(!operation)
	{
		return FALSE;
	}

	if(!strcmp(SERVICE_OPERATION_VIEW , operation))
	{
		service_get_uri(service, &uri);
		if (uri && strlen(uri))
		{
			if(strstr(uri,MP_UTIL_FILE_PREFIX))
		        {
		        	*path = g_strdup(uri+strlen(MP_UTIL_FILE_PREFIX));
		        	free(uri);
		        }
		        else
				*path = uri;
		}
		else
		{
			*path = NULL;
			WARN_TRACE("No URI.");
			SAFE_FREE(operation);
			return FALSE;
		}

		service_get_mime(service, &mime);
		mp_debug("mime : %s", mime);
		SAFE_FREE(mime);
	}
	else
	{
		*path = NULL;
		WARN_TRACE("No Operation.");
		return FALSE;
	}

	SAFE_FREE(operation);
	DEBUG_TRACE("URI path uri : %s", uri);
	return TRUE;
}

void
mp_util_reset_genlist_mode_item(Evas_Object *genlist)
{
	MP_CHECK(genlist);
	Elm_Object_Item *gl_item =
		(Elm_Object_Item *)elm_genlist_decorated_item_get(genlist);
	if (gl_item) {
		elm_genlist_item_decorate_mode_set(gl_item, "slide", EINA_FALSE);
		elm_genlist_item_select_mode_set(gl_item, ELM_OBJECT_SELECT_MODE_DEFAULT);
	}
}

#ifndef MP_SOUND_PLAYER
view_data_t *
mp_util_get_view_data(struct appdata *ad)
{
	MP_CHECK_NULL(ad);
	MP_CHECK_NULL(ad->naviframe);

	return evas_object_data_get(ad->naviframe, "view_data");
}

mp_layout_data_t*
mp_util_get_layout_data(Evas_Object* obj)
{
	MP_CHECK_NULL(obj);
	return evas_object_data_get(obj, "layout_data");
}
#endif

bool
mp_util_is_image_valid(Evas *evas, const char *path)
{
	MP_CHECK_FALSE(path);
	MP_CHECK_FALSE(evas);

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		mp_error("file not exitst");
		return false;
	}

	Evas_Object *image = NULL;
	int width = 0;
	int height = 0;

	image = evas_object_image_add(evas);
	MP_CHECK_FALSE(image);
	evas_object_image_file_set(image, path, NULL);
	evas_object_image_size_get(image, &width, &height);
	evas_object_del(image);

	if (width <= 0 || height <= 0) {
		mp_debug("Cannot load file : %s", path);
		return false;
	}

	return true;
}

#define MP_PATH_INFO_MAX_LENGTH		30
#define MP_PATH_INFO_TRANS_OMIT		".."
#define MP_PATH_INFO_LEVEL_BOUNDARY		3
#define MP_PATH_INFO_LEN_THRESHOLD	3
#define MP_PATH_INFO_SEP		"/"
#define MP_PATH_INFO_RETRENCH		128

bool
mp_util_is_string_elipsized(char *path)
{
	MP_CHECK_FALSE(path);
	if(strlen(path) < MP_PATH_INFO_MAX_LENGTH)
	{
		return false;
	}
	else
		return true;
}

char *mp_util_path_info_retrench(const char *string)
{
	mp_retvm_if(string == NULL, g_strdup(MP_PATH_INFO_TRANS_OMIT), "input path is NULL");
	char *retrench = NULL;
	if (strlen (string) > MP_PATH_INFO_LEN_THRESHOLD) {
		char *utf8_string = elm_entry_utf8_to_markup(string);
		MP_CHECK_NULL(utf8_string);
		if (g_utf8_strlen(utf8_string, -1) > 2) {
			retrench = calloc(1, MP_PATH_INFO_RETRENCH);
			if (retrench) {
				g_utf8_strncpy(retrench, utf8_string, 2);
				char *temp = retrench;
				retrench = g_strconcat(retrench, MP_PATH_INFO_TRANS_OMIT, NULL);
				free(utf8_string);
				free(temp);
			}

		} else {
			retrench = utf8_string;
		}
	} else {
		retrench = elm_entry_utf8_to_markup(string);
	}
	return retrench;
}

char *
mp_util_shorten_path(char *path_info)
{
	int start = 0;
	gchar **params = NULL;
	int count = 0;
	int len;
	int i = 0;
	int j = 0;
	char *output = NULL;
	char *temp = NULL;
	char *base = NULL;
	bool exception = true;

	MP_CHECK_EXCEP(path_info);

	if (!mp_util_is_string_elipsized(path_info))
		return g_strdup(path_info);

	params = g_strsplit(path_info, "/", 0);
	MP_CHECK_EXCEP(params);

	count = g_strv_length(params);

	if (count > MP_PATH_INFO_LEVEL_BOUNDARY)
	{
		start = count - MP_PATH_INFO_LEVEL_BOUNDARY;
		output = g_strdup("..");
	}
	else
	{
		output = g_strdup("");
	}
	MP_CHECK_EXCEP(output);

	for(i=start ; i < count; i++)
	{
		base = g_strdup(output);
		MP_CHECK_EXCEP(base);
		for(j=i ; j < count; j++)
		{
			temp = g_strconcat(base, MP_PATH_INFO_SEP, params[j], NULL);
			IF_FREE(base);
			base = temp;
			temp = NULL;
		}

		if(i == (count-1) || !mp_util_is_string_elipsized(base))
		{
			IF_FREE(output);
			output = base;
			base = NULL;
			break;
		}
		else
		{
			char *retrench = mp_util_path_info_retrench(params[i]);
			MP_CHECK_EXCEP(retrench);
			len = strlen(params[i]);
			IF_FREE(base);
			base = g_strconcat(output, MP_PATH_INFO_SEP, retrench, NULL);
			IF_FREE(output);
			free(retrench);
			output = base;
			base = NULL;
		}
	}

	exception = false;

	mp_exception:


	if(params)
		g_strfreev(params);

	if(exception)
	{
		IF_FREE(output);
		IF_FREE(base);
		return g_strdup(GET_SYS_STR("IDS_COM_BODY_UNKNOWN"));
	}
	else
		return output;
}

#ifndef MP_SOUND_PLAYER
void
mp_util_unset_rename(mp_layout_data_t * layout_data)
{
	DEBUG_TRACE("");
	MP_CHECK(layout_data);

	if (layout_data->rename_git)
	{
		DEBUG_TRACE("");
		if (elm_genlist_item_flip_get(layout_data->rename_git))
		{
			elm_genlist_item_flip_set(layout_data->rename_git, EINA_FALSE);
			elm_genlist_item_select_mode_set(layout_data->rename_git, ELM_OBJECT_SELECT_MODE_DEFAULT);
		}
		layout_data->rename_git = NULL;
		layout_data->rename_mode = false;

		//set title button sensitivity
		mp_common_set_toolbar_button_sensitivity(layout_data, layout_data->checked_count);
	}
}
#endif

bool
mp_util_is_db_updating(void)
{
	int db_status = VCONFKEY_FILEMANAGER_DB_UPDATED;
	vconf_get_int(VCONFKEY_FILEMANAGER_DB_STATUS, &db_status);
	if(db_status == VCONFKEY_FILEMANAGER_DB_UPDATED)
		return false;
	else
		return true;
}

bool
mp_util_is_bt_connected(void)
{
	int ret = 0;
	bool connected = 0;
	bool ret_val = FALSE;
	char *bt_name = NULL;

	ret = sound_manager_get_a2dp_status(&connected, &bt_name);
	if (ret == SOUND_MANAGER_ERROR_NONE)
	{
		DEBUG_TRACE("Is Bluetooth A2DP On Success : [%d][%s]", connected,
			 bt_name);
		if (connected != 0) {
			ret_val = TRUE;
		} else {
			DEBUG_TRACE("no bluetooth");
		}
	} else {
		DEBUG_TRACE("Is Bluetooth A2DP On Error : [%d]", ret);
	}

	if (bt_name)
		free(bt_name);

	return ret_val;
}

bool
mp_util_is_earjack_inserted(void)
{
	int value = 0;

	vconf_get_int(VCONFKEY_SYSMAN_EARJACK, &value);

	if (value == VCONFKEY_SYSMAN_EARJACK_REMOVED) {
		DEBUG_TRACE("no earjack..");
		return false;
	} else {
		DEBUG_TRACE("earjack inserted.. (type[%d])", value);
		return true;
	}
}

void
mp_util_get_sound_path(mp_snd_path *snd_path)
{
	sound_device_in_e in;
	sound_device_out_e out;

	sound_manager_get_active_device(&in, &out);
	switch(out) {
	case SOUND_DEVICE_OUT_SPEAKER:
		DEBUG_TRACE("SOUND_DEVICE_OUT_SPEAKER");
		*snd_path = MP_SND_PATH_SPEAKER;
		break;

	case SOUND_DEVICE_OUT_WIRED_ACCESSORY:
		DEBUG_TRACE("SOUND_DEVICE_OUT_WIRED_ACCESSORY");
		*snd_path = MP_SND_PATH_EARPHONE;
		break;

	case SOUND_DEVICE_OUT_BT_A2DP:
		DEBUG_TRACE("SOUND_DEVICE_OUT_BT_A2DP");
		*snd_path = MP_SND_PATH_BT;
		break;

	default:
		DEBUG_TRACE("default:speaker");
		*snd_path = MP_SND_PATH_SPEAKER;
		break;
	}
}

#define DEF_BUF_LEN            (512)
const char *
mp_util_search_markup_keyword(const char *string, char *searchword, bool *result)
{
	char pstr[DEF_BUF_LEN + 1] = {0,};
	static char return_string[DEF_BUF_LEN + 1] = { 0, };
	int word_len = 0;
	int search_len = 0;
	int i = 0;
	bool found = false;
	gchar* markup_text_start = NULL;
	gchar* markup_text_end= NULL;
	gchar* markup_text= NULL;

	MP_CHECK_NULL(string);
	MP_CHECK_NULL(searchword);
	MP_CHECK_NULL(result);

	//2 temporary disable until costomize genlist item style
	*result = true;
	return string;

	if(g_utf8_validate(string,-1,NULL)) {

		strncpy(pstr, string, DEF_BUF_LEN);

		word_len = strlen(pstr);
		search_len = strlen(searchword);

		for (i = 0; i < word_len; i++) {
			if (!strncasecmp(searchword, &pstr[i], search_len)) {
				found = true;
				break;
			}
		}

		*result = found;
		memset(return_string, 0x00, DEF_BUF_LEN+1);

		if (found) {
			if (i == 0) {
				markup_text = g_markup_escape_text(&pstr[0], search_len);
				markup_text_end = g_markup_escape_text(&pstr[search_len], word_len-search_len);
				MP_CHECK_NULL(markup_text && markup_text_end);
				snprintf(return_string,
							DEF_BUF_LEN,
							"<match>%s</match>%s",
							markup_text,
							(char*)markup_text_end);
				IF_FREE(markup_text);
				IF_FREE(markup_text_end);
			} else {
				markup_text_start = g_markup_escape_text(&pstr[0], i);
				markup_text = g_markup_escape_text(&pstr[i], search_len);
				markup_text_end =  g_markup_escape_text(&pstr[i+search_len], word_len-(i+search_len));
				MP_CHECK_NULL(markup_text_start &&markup_text && markup_text_end);
				snprintf(return_string,
							DEF_BUF_LEN,
							"%s<match>%s</match>%s",
							(char*)markup_text_start,
							markup_text,
							(char*)markup_text_end);
				IF_FREE(markup_text);
				IF_FREE(markup_text_start);
				IF_FREE(markup_text_end);
			}
		} else {
			snprintf(return_string, DEF_BUF_LEN, "%s", pstr);
		}
	}

	return return_string;
}

bool
mp_util_is_other_player_playing()
{
	bool ret = FALSE;

	int state = 0;
	int pid = 0;
	if (vconf_get_int(VCONFKEY_MUSIC_STATE, &state) == 0) {
		if (state == VCONFKEY_MUSIC_PLAY) {
			if (vconf_get_int(MP_VCONFKEY_PLAYING_PID, &pid) == 0) {
				if (pid != getpid()) {
					mp_debug("## other player is playing some music ##");
					ret = TRUE;
				}
			} else {
				mp_error("vconf_get_int() error");
			}
		}
	} else {
		mp_error("vconf_get_int() error");
	}

	return ret;
}

int
mp_commmon_check_rotate_lock(void)
{
	int lock = FALSE;
	lock = -1;
	if (!vconf_get_bool(VCONFKEY_SETAPPL_ROTATE_LOCK_BOOL, &lock))
	{
		mp_debug("lock state: %d", lock);
		return lock;
	}
	else
		return -1;
}


int
mp_check_battery_available(void)
{
	int batt_state = -1;

	if (!vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, &batt_state))
	{
		/* low battery status
		 *      VCONFKEY_SYSMAN_BAT_WARNING_LOW         = 15 %
		 *      VCONFKEY_SYSMAN_BAT_CRITICAL_LOW        = 5 %
		 *      VCONFKEY_SYSMAN_BAT_POWER_OFF           = 1 %
		 *      since 2011. 03. 02
		 */
		if (batt_state <= VCONFKEY_SYSMAN_BAT_POWER_OFF)	//VCONFKEY_SYSMAN_BAT_POWER_OFF - 1% remaninging
		{
			// don't need to check changing state.
			/*
			   if(!vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &batt_state))
			   {
			   if(batt_state ==1)
			   {
			   DEBUG_TRACE("current chargine status");
			   return 0;
			   }
			   } */

			WARN_TRACE("batt_state: %d", batt_state);
			return -1;
		}
	}
	else
	{
		ERROR_TRACE("Fail to get battery status");
	}
	return 0;
}

int
mp_check_mass_storage_mode(void)
{
	return 0;
}

bool
mp_util_sleep_lock_set(bool lock)
{
	int ret = POWER_ERROR_NONE;

	if (lock) {
		mp_debug("sleep_lock");
		ret = power_lock_state(POWER_STATE_SCREEN_OFF, 0);
	} else {
		mp_debug("sleep_unlock");
		ret = power_unlock_state(POWER_STATE_SCREEN_OFF);
	}

	if (ret != POWER_ERROR_NONE) {
		mp_error("pm_lock(un_lock) error.. [%d]", ret);
		return FALSE;
	}

	return TRUE;
}

bool
mp_util_is_nfc_feature_on(void)
{
	bool ret = FALSE;

	int state = 0;
	if (!vconf_get_bool(VCONFKEY_NFC_FEATURE, &state)) {
		mp_debug("NFC state = %d", state);
		if (state == VCONFKEY_NFC_FEATURE_ON)
			ret = TRUE;
	} else {
		mp_debug("vcont_get_bool() fail.. %s", VCONFKEY_NFC_FEATURE);
	}

	return ret;
}

void
mp_util_strncpy_safe(char *x_dst, const char *x_src, int max_len)
{
	if (!x_src || strlen(x_src) == 0) {
		mp_error("x_src is NULL");
		return;
	}

	if (max_len < 1) {
		mp_error("length is Wrong");
		return;
	}

    strncpy(x_dst, x_src, max_len-1);
	x_dst[max_len-1] = '\0';
}

static const double gaussian_template[7][7] =
{
	{0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067},
	{0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292},
	{0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117},
	{0.00038771, 0.01330373, 0.11098164, 0.22508352, 0.11098164, 0.01330373, 0.00038771},
	{0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117},
	{0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292},
	{0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067}
};

#define DARK_SCALE 0.6

static void __mp_util_gaussian_blur(unsigned char *src, unsigned char *dest, int w, int h)
{
         MP_CHECK(src);
         MP_CHECK(dest);

         int x, y, i, j, idx, idx2, xx, yy;
         for (y = 0; y < h; y++) {
                   for (x = 0; x < w; x++) {
		   idx = (y*w+x)*4;
                   double v1 = 0, v2 = 0, v3 = 0;

                   for (i = 0; i < 7; i++) {
                            for (j = 0; j < 7; j++) {
                                     yy = y + j;
                                     xx = x + i;
                                     if (xx >= w)
                                               xx = w - 1;
                                     if (yy >= h)
                                               yy = h - 1;
                                     idx2 = (yy*w+xx)*4;
                                     v1 += (*(src+idx2))*gaussian_template[i][j];
                                     v2 += (*(src+idx2+1))*gaussian_template[i][j];
                                     v3 += (*(src+idx2+2))*gaussian_template[i][j];
                            }
                   }
                   *(dest+idx) = v1 * DARK_SCALE;
                   *(dest+idx+1) = v2 * DARK_SCALE;
                   *(dest+idx+2) = v3 * DARK_SCALE;
                   *(dest+idx+3) = (*(src+idx+3));
                   }
         }
}


bool mp_util_edit_image(Evas *evas, Evas_Object *src_image, const char *path, mp_playing_view_bg_capture_mode mode)
{
	startfunc;
	MP_CHECK_FALSE(evas);
	MP_CHECK_FALSE(src_image);
	MP_CHECK_FALSE(path);
	MP_CHECK_FALSE(mode >= MP_PLAYING_VIEW_TOP_LEFT);
	MP_CHECK_FALSE(mode <= MP_PLAYING_VIEW_BOTTOM_RIGHT);

	float rate_w = 720.0/1500.0;
	float rate_h = 1280.0/1500.0;

	DEBUG_TRACE("rate_w=%f, rate_h=%f", rate_w, rate_h);

	Evas_Object *image = evas_object_image_add(evas);
	evas_object_image_file_set(image, path, NULL);

	int w, h, dest_w, dest_h, x, y;
	evas_object_image_size_get(image, &w, &h);
	unsigned char *src = NULL;
	src = (unsigned char *)evas_object_image_data_get(image, EINA_FALSE);

	MP_CHECK_FALSE(src);
	DEBUG_TRACE("path=%s, w=%d, h=%d", path, w, h);
	dest_w = (int)(rate_w * w);
	dest_h = (int)(rate_h * h);
	DEBUG_TRACE("rate_w=%f, rate_h=%f, dest_w=%d, dest_h=%d", rate_w, rate_h, dest_w, dest_h);

	int start_x, start_y, end_x, end_y;

	switch (mode) {
	case MP_PLAYING_VIEW_TOP_LEFT:
		start_x = 0;
		start_y = 0;
		break;

	case MP_PLAYING_VIEW_TOP_CENTER:
		start_x = (w - dest_w)/2;
		start_y = 0;
		break;

	case MP_PLAYING_VIEW_TOP_RIGHT:
		start_x = w - dest_w;
		start_y = 0;
		break;

	case MP_PLAYING_VIEW_BOTTOM_LEFT:
		start_x = 0;
		start_y = h - dest_h;
		break;

	case MP_PLAYING_VIEW_BOTTOM_CENTER:
		start_x = (w - dest_w)/2;
		start_y = h - dest_h;
		break;

	case MP_PLAYING_VIEW_BOTTOM_RIGHT:
		start_x = w - dest_w;
		start_y = h - dest_h;
		break;

	default:
		return false;
	}

	unsigned char *dest = NULL;
	dest = (unsigned char *)malloc(dest_w * dest_h * 4);
	MP_CHECK_EXCEP(dest);
	memset(dest, 0, dest_w * dest_h * 4);

	end_x = start_x + dest_w;
	end_y = start_y + dest_h;
	DEBUG_TRACE("(%d, %d), (%d, %d)", start_x, start_y, end_x, end_y);

	int dest_idx = 0;
	int src_idx = 0;
	unsigned char gray = 0;
	for (y = start_y; y < end_y; y++) {
		for (x = start_x; x < end_x; x++) {
			dest_idx = ((y-start_y)*dest_w+(x-start_x))*4;
			src_idx = (y*w+x)*4;

			gray = (*(src+src_idx))*0.3+(*(src+src_idx+1))*0.59+(*(src+src_idx+2))*0.11;
			*(dest+dest_idx) = gray;
			*(dest+dest_idx+1) = gray;
			*(dest+dest_idx+2) = gray;
			*(dest+dest_idx+3) = 0;
			//*(dest+dest_idx+3) = (*(src+src_idx+3));
		}
	}

	unsigned char *dest_data = NULL;
	dest_data = (unsigned char *)malloc(dest_w * dest_h * 4);
	MP_CHECK_EXCEP(dest_data);
	memset(dest_data, 0, dest_w * dest_h * 4);
	__mp_util_gaussian_blur(dest, dest_data, dest_w, dest_h);
	IF_FREE(dest);

	evas_object_image_data_set(src_image, NULL);
	evas_object_image_size_set(src_image, dest_w, dest_h);
	evas_object_image_smooth_scale_set(src_image, EINA_TRUE);
	evas_object_image_data_copy_set(src_image, dest_data);
	evas_object_image_data_update_add(src_image, 0, 0, dest_w, dest_h);
	IF_FREE(dest_data);

	mp_evas_object_del(image);

	endfunc;
	return true;

	mp_exception:
	mp_evas_object_del(image);
	IF_FREE(dest);
	IF_FREE(dest_data);
	return false;
}

void
mp_util_free_track_info(mp_track_info_t *track_info)
{
	MP_CHECK(track_info);

	IF_FREE(track_info->uri);
	IF_FREE(track_info->title);
	IF_FREE(track_info->artist);
	IF_FREE(track_info->album);
	IF_FREE(track_info->genre);
	IF_FREE(track_info->location);
	IF_FREE(track_info->format);

	IF_FREE(track_info->thumbnail_path);
	IF_FREE(track_info->copyright);

	IF_FREE(track_info->author);
	IF_FREE(track_info->track_num);

	free(track_info);
}

void
mp_util_load_track_info(struct appdata *ad, mp_plst_item *cur_item, mp_track_info_t **info)
{
	MP_CHECK(ad);
	MP_CHECK(cur_item);
	MP_CHECK(info);

	int ret = 0;
	mp_media_info_h svc_audio_item = NULL;
	mp_track_info_t *track_info = NULL;

	*info = track_info = calloc(1, sizeof(mp_track_info_t));
	MP_CHECK(track_info);

	track_info->uri = g_strdup(cur_item->uri);

	if(cur_item->uid)
	{
		{
			ret = mp_media_info_create(&svc_audio_item, cur_item->uid);

			mp_media_info_get_title(svc_audio_item, &track_info->title);
			mp_media_info_get_album(svc_audio_item, &track_info->album);
			mp_media_info_get_artist(svc_audio_item, &track_info->artist);
			mp_media_info_get_thumbnail_path(svc_audio_item, &track_info->thumbnail_path);
			mp_media_info_get_genre(svc_audio_item, &track_info->genre);
			mp_media_info_get_copyright(svc_audio_item, &track_info->copyright);
			mp_media_info_get_composer(svc_audio_item, &track_info->author);
			mp_media_info_get_duration(svc_audio_item, &track_info->duration);
			mp_media_info_get_track_num(svc_audio_item, &track_info->track_num);
			mp_media_info_get_format(svc_audio_item, &track_info->format);
			mp_media_info_get_favorite(svc_audio_item, &track_info->favorite);
		}

		track_info->title = g_strdup(track_info->title);
		track_info->album = g_strdup(track_info->album);
		track_info->artist = g_strdup(track_info->artist);
		track_info->thumbnail_path = g_strdup(track_info->thumbnail_path);
		track_info->genre = g_strdup(track_info->genre);
		track_info->copyright = g_strdup(track_info->copyright);
		track_info->author = g_strdup(track_info->author);
		track_info->track_num = g_strdup(track_info->track_num);
		track_info->format = g_strdup(track_info->format);
		track_info->location = g_strdup(track_info->location);

		if(svc_audio_item)
		{
				mp_media_info_destroy(svc_audio_item);
		}


	}
	else
	{
		mp_tag_info_t tag_info;
		mp_file_tag_info_get_all_tag(cur_item->uri, &tag_info);


		track_info->title = g_strdup(tag_info.title);
		track_info->album = g_strdup(tag_info.album);
		track_info->artist = g_strdup(tag_info.artist);
		track_info->thumbnail_path = g_strdup(tag_info.albumart_path);
		track_info->genre = g_strdup(tag_info.genre);
		track_info->copyright = g_strdup(tag_info.copyright);
		track_info->author = g_strdup(tag_info.author);
		track_info->track_num = g_strdup(tag_info.track);

		track_info->duration = tag_info.duration;

		GString *format = g_string_new("");
		if (format)
		{
			if (tag_info.audio_bitrate > 0)
				g_string_append_printf(format, "%dbps ", tag_info.audio_bitrate);

			if (tag_info.audio_samplerate > 0)
				g_string_append_printf(format, "%.1fHz ", (double)tag_info.audio_samplerate);

			if (tag_info.audio_channel > 0)
				g_string_append_printf(format, "%dch", tag_info.audio_channel);

			track_info->format = g_strdup(format->str);
			g_string_free(format, TRUE);
		}

		mp_file_tag_free(&tag_info);
	}

}

void
mp_util_append_media_list_item_to_playlist(mp_plst_mgr *playlist_mgr, mp_media_list_h media_list, int count, int current_index, const char *path)
{
	int i;
	char *uid;
	char *uri;
	mp_plst_item *cur_item = NULL;

	for(i = 0; i < count; i++)
	{
		mp_plst_item *plst_item;
		mp_media_info_h item = mp_media_info_list_nth_item(media_list, i);
		mp_media_info_get_media_id(item, &uid);
		mp_media_info_get_file_path(item, &uri);
		plst_item = mp_playlist_mgr_item_append(playlist_mgr, uri, uid, MP_TRACK_URI);
		if(i == current_index || !g_strcmp0(uri, path))
			cur_item = plst_item;
	}
	mp_playlist_mgr_set_current(playlist_mgr, cur_item);

}

char* mp_util_get_fid_by_full_path(const char *full_path)
{
	startfunc;

	char *uid = NULL;

	MP_CHECK_NULL(full_path);

	int ret = 0;
	mp_media_info_h record = NULL;
	if (mp_check_file_exist(full_path))
	{
		ret = mp_media_info_create_by_path(&record, full_path);
		if(ret == 0)
		{
			ret = mp_media_info_get_media_id(record, &uid);
			uid = g_strdup(uid);
			mp_media_info_destroy(record);
		}
	}
	return uid;
}
