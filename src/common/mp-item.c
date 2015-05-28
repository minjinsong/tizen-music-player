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

#include <sys/time.h>
#include <vconf.h>
#include <glib.h>
#include <fcntl.h>

#include "music.h"
#include "mp-item.h"
#include "mp-menu.h"
#include "mp-ug-launch.h"
#include "mp-player-debug.h"
#include "mp-group-view.h"

bool
mp_item_update_db(char *fid)
{

	mp_media_info_h media;
	int count = 0;
	bool ret = FALSE;

	ret = mp_media_info_create(&media, fid);
	if (ret != 0)
	{
		mp_media_info_destroy(media);
		return false;
	}

	mp_media_info_get_played_count(media, &count);

	mp_media_info_set_played_count(media, ++count);
	mp_media_info_set_played_time(media, time(NULL));

	mp_media_info_destroy(media);

	return TRUE;
}

