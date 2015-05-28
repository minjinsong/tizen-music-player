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


#ifndef __MP_VCONF_PRIVATE_KEYS_H__
#define __MP_VCONF_PRIVATE_KEYS_H__

#include <vconf-keys.h>

#define VCONFKEY_MP_PRIVATE			"private/"PKG_NAME"/"
#define VCONFKEY_MP_DB_PREFIX		"db/"VCONFKEY_MP_PRIVATE
#define VCONFKEY_MP_MEMORY_PREFIX	"memory/"VCONFKEY_MP_PRIVATE

/**
 * @brief trigger of sound effect changes
 *
 * type: bool
 *
 * value is not meaningful
 */
#define VCONFKEY_MUSIC_SE_CHANGE			VCONFKEY_MP_DB_PREFIX"se_change"

/**
 * @brief trigger of extend sound effect changes
 *
 * type: int
 *
 * value is not meaningful
 */
#define VCONFKEY_MUSIC_SA_USER_CHANGE			VCONFKEY_MP_MEMORY_PREFIX"sa_user_change"

/**
 * @brief trigger of menu settng changes
 *
 * type: int
 *
 * value is not meaningful
 */
#define VCONFKEY_MUSIC_MENU_CHANGE			VCONFKEY_MP_DB_PREFIX"menu_change"


/**
 * @brief setting value of sound alive
 *
 * type: int
 *
 * value is not meaningful
 */
#define VCONFKEY_MUSIC_SOUND_ALIVE_VAL		VCONFKEY_MP_DB_PREFIX"sound_alive_val"
enum
{
	VCONFKEY_MUSIC_SOUND_ALIVE_AUTO,
	VCONFKEY_MUSIC_SOUND_ALIVE_POP,
	VCONFKEY_MUSIC_SOUND_ALIVE_ROCK,
	VCONFKEY_MUSIC_SOUND_ALIVE_DANCE,
	VCONFKEY_MUSIC_SOUND_ALIVE_JAZZ,
	VCONFKEY_MUSIC_SOUND_ALIVE_CLASSIC,
	VCONFKEY_MUSIC_SOUND_ALIVE_NORMAL,
	VCONFKEY_MUSIC_SOUND_ALIVE_USER,
	VCONFKEY_MUSIC_SOUND_ALIVE_VOCAL,
	VCONFKEY_MUSIC_SOUND_ALIVE_BASS_BOOST,
	VCONFKEY_MUSIC_SOUND_ALIVE_TREBLE_BOOST,
	VCONFKEY_MUSIC_SOUND_ALIVE_M_THEATER,
	VCONFKEY_MUSIC_SOUND_ALIVE_EXTERNALIZATION,
	VCONFKEY_MUSIC_SOUND_ALIVE_CAFE,
	VCONFKEY_MUSIC_SOUND_ALIVE_CONCERT_HALL,
	VCONFKEY_MUSIC_SOUND_ALIVE_NUM,
};


/**
 * @brief custom equalizer value
 *
 * type: double
 */
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_1	VCONFKEY_MP_DB_PREFIX"eq_custom_1"
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_2	VCONFKEY_MP_DB_PREFIX"eq_custom_2"
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_3	VCONFKEY_MP_DB_PREFIX"eq_custom_3"
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_4	VCONFKEY_MP_DB_PREFIX"eq_custom_4"
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_5	VCONFKEY_MP_DB_PREFIX"eq_custom_5"
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_6	VCONFKEY_MP_DB_PREFIX"eq_custom_6"
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_7	VCONFKEY_MP_DB_PREFIX"eq_custom_7"
#define VCONFKEY_MUSIC_EQUALISER_CUSTOM_8	VCONFKEY_MP_DB_PREFIX"eq_custom_8"


/**
 * @brief extended user audio effects
 *
 * type: double
 */
#define VCONFKEY_MUSIC_USER_AUDIO_EFFECT_3D			VCONFKEY_MP_DB_PREFIX"user_audio_effect_3d"
#define VCONFKEY_MUSIC_USER_AUDIO_EFFECT_BASS		VCONFKEY_MP_DB_PREFIX"user_audio_effect_bass"
#define VCONFKEY_MUSIC_USER_AUDIO_EFFECT_ROOM		VCONFKEY_MP_DB_PREFIX"user_audio_effect_room"
#define VCONFKEY_MUSIC_USER_AUDIO_EFFECT_REVERB		VCONFKEY_MP_DB_PREFIX"user_audio_effect_reverb"
#define VCONFKEY_MUSIC_USER_AUDIO_EFFECT_CLARITY	VCONFKEY_MP_DB_PREFIX"user_audio_effect_clarity"


/**
 * @brief auto off
 *
 * type: int (minute)
 */
#define VCONFKEY_MUSIC_AUTO_OFF_TIME_VAL	VCONFKEY_MP_MEMORY_PREFIX"auto_off_time_val"

/**
 * @brief auto off type
 *
 * type: int
 */
#define VCONFKEY_MUSIC_AUTO_OFF_TYPE_VAL	VCONFKEY_MP_MEMORY_PREFIX"auto_off_type_val"
enum
{
	VCONFKEY_MUSIC_AUTO_OFF_TIME_OFF	= 0,
	VCONFKEY_MUSIC_AUTO_OFF_TIME_15,
	VCONFKEY_MUSIC_AUTO_OFF_TIME_30,
	VCONFKEY_MUSIC_AUTO_OFF_TIME_60,
	VCONFKEY_MUSIC_AUTO_OFF_TIME_90,
	VCONFKEY_MUSIC_AUTO_OFF_TIME_120,
	VCONFKEY_MUSIC_AUTO_OFF_TIME_CUSTOM,
	VCONFKEY_MUSIC_AUTO_OFF_TIME_MAX,
};


/**
 * @brief playlist shuffle state
 *
 * type: bool
 *
 * 0 : off
 * 1 : on
 */
#define MP_VCONFKEY_MUSIC_SHUFFLE			VCONFKEY_MP_DB_PREFIX"shuffle"


/**
 * @brief playlist repeat state
 *
 * type: int
 *
 * 0 : repeat all
 * 1 : no repeat
 * 2 : repeat only a songs
 */
#define MP_VCONFKEY_MUSIC_REPEAT			VCONFKEY_MP_DB_PREFIX"repeat"
enum
{
	MP_SETTING_REP_ALL,
	MP_SETTING_REP_NON,
	MP_SETTING_REP_1
};


/**
 * @brief playlist repeat state
 *
 * type: int
 *
 * 0 : repeat all
 * 1 : no repeat
 * 2 : repeat only a songs
 */
#define MP_VCONFKEY_MUSIC_SQUARE_AXIS_VAL			VCONFKEY_MP_DB_PREFIX"square_axis_val"
enum
{
	MP_VCONFKEY_MUSIC_SQUARE_AXIS_MOOD,
	MP_VCONFKEY_MUSIC_SQUARE_AXIS_YEAR,
	MP_VCONFKEY_MUSIC_SQUARE_AXIS_ADDED,
	MP_VCONFKEY_MUSIC_SQUARE_AXIS_TIME
};


/**
 * @brief check which player is playing now
 *
 * type: int
 *
 * pid of music-player of sound-player
 */
#define MP_VCONFKEY_PLAYING_PID				VCONFKEY_MP_MEMORY_PREFIX"playing_pid"




/* for live-box */
#define MP_LIVE_VCONF_PREFIX				VCONFKEY_MP_MEMORY_PREFIX

#define MP_LIVE_PLAY_STATE					MP_LIVE_VCONF_PREFIX"player_state"

/**
 * @brief elapsed time of current playing song
 *
 * type: string
 *
 * ex) "00:00"
 */
#define MP_LIVE_CUR_POS						MP_LIVE_VCONF_PREFIX"pos"


/**
 * @brief progressbar position of current playing song
 *
 * type: double
 *
 * range : 0.0 ~ 1.0
 */
#define MP_LIVE_CUR_PROGRESS_POS			MP_LIVE_VCONF_PREFIX"progress_pos"


/**
 * @brief trigger of live box button click
 *
 * type: bool
 */
#define MP_LIVE_PLAY_CLICKED				MP_LIVE_VCONF_PREFIX"play_clicked"	//bool
#define MP_LIVE_PAUSE_CLICKED				MP_LIVE_VCONF_PREFIX"pause_clicked"	//bool
#define MP_LIVE_PREV_PRESSED				MP_LIVE_VCONF_PREFIX"prev_pressed"	//bool
#define MP_LIVE_PREV_RELEASED				MP_LIVE_VCONF_PREFIX"prev_released"	//bool
#define MP_LIVE_NEXT_PRESSED				MP_LIVE_VCONF_PREFIX"next_pressed"	//bool
#define MP_LIVE_NEXT_RELEASED				MP_LIVE_VCONF_PREFIX"next_released"	//bool
#define MP_LIVE_PROGRESS_RATIO_CHANGED				MP_LIVE_VCONF_PREFIX"position_changed"	//double


/**
 * @brief setting value of auto created playlist
 *
 * All playlist can be selected with OR operation
 *
*/
#define MP_VCONFKEY_PLAYLIST_VAL_INT VCONFKEY_MP_DB_PREFIX"playlist"
enum {
	MP_PLAYLIST_MOST_PLAYED = 0x0001,
	MP_PLAYLIST_RECENTLY_PLAYED = 0x0002,
	MP_PLAYLIST_RECENTLY_ADDED = 0x0004,
};

/**
 * @brief play speed
 *
 * type: double
 *
 * range : 0.5 ~ 2.0
 */
#define VCONFKEY_MUSIC_PLAY_SPEED			VCONFKEY_MP_MEMORY_PREFIX"playspeed"


#endif /* __MP_VCONF_PRIVATE_KEYS_H__ */

