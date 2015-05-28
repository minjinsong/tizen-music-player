#include "mp-avrcp.h"
#include "mp-player-debug.h"
#include <bluetooth.h>
#include "mp-define.h"

typedef struct{
	mp_avrcp_shuffle_changed_cb s_cb;
	mp_avrcp_repeat_changed_cb r_cb;
	mp_avrcp_eq_changed_cb e_cb;
}MpAvrcpCb_t;

static MpAvrcpCb_t *gMpAvrcpCb;

static void _mp_avrcp_equalizer_state_changed_cb (bt_avrcp_equalizer_state_e equalizer, void *user_data)
{
	MP_CHECK(gMpAvrcpCb);
	mp_avrcp_eq_state_e mode ;
	if(equalizer == BT_AVRCP_EQUALIZER_STATE_OFF)
	{
		DEBUG_TRACE("eq off");
		mode =  MP_AVRCP_EQ_OFF;
	}
	else
	{
		DEBUG_TRACE("eq on");
		mode =  MP_AVRCP_EQ_ON;
	}
	if(gMpAvrcpCb->e_cb)
		gMpAvrcpCb->e_cb(mode, user_data);
}

static void _mp_avrcp_shuffle_mode_changed_cb (bt_avrcp_shuffle_mode_e shuffle, void *user_data)
{
	MP_CHECK(gMpAvrcpCb);
	mp_avrcp_shuffle_mode_e mode ;
	if(shuffle == BT_AVRCP_SHUFFLE_MODE_OFF)
	{
		DEBUG_TRACE("shuffle off");
		mode =  MP_AVRCP_SHUFFLE_OFF;
	}
	else
	{
		DEBUG_TRACE("shuffle on");
		mode =  MP_AVRCP_SHUFFLE_ON;
	}

	if(gMpAvrcpCb->s_cb)
		gMpAvrcpCb->s_cb(mode, user_data);
}

static void _mp_avrcp_repeat_mode_changed_cb (bt_avrcp_repeat_mode_e repeat, void *user_data)
{
	MP_CHECK(gMpAvrcpCb);
	mp_avrcp_repeat_mode_e mode ;
	if(repeat == BT_AVRCP_REPEAT_MODE_OFF)
	{
		DEBUG_TRACE("shuffle off");
		mode =  MP_AVRCP_REPEAT_OFF;
	}
	else if(repeat == BT_AVRCP_REPEAT_MODE_SINGLE_TRACK)
	{
		DEBUG_TRACE("shuffle off");
		mode =  MP_AVRCP_REPEAT_ONE;
	}
	else
	{
		DEBUG_TRACE("shuffle on");
		mode =  MP_AVRCP_REPEAT_ALL;
	}

	if(gMpAvrcpCb->r_cb)
		gMpAvrcpCb->r_cb(mode, user_data);
}

int mp_avrcp_target_initialize(mp_avrcp_connection_state_changed_cb callback, void *user_data)
{
	startfunc;
	int res = BT_ERROR_NONE;
	res = bt_initialize();
	MP_CHECK_VAL(res == BT_ERROR_NONE, res);

	res = bt_avrcp_target_initialize(callback, user_data);
	MP_CHECK_VAL(res == BT_ERROR_NONE, res);

	gMpAvrcpCb = calloc(1, sizeof(MpAvrcpCb_t));
	return res;
}

int mp_avrcp_target_finalize(void)
{
	startfunc;
	int res = BT_ERROR_NONE;

	res = bt_avrcp_target_deinitialize();
	MP_CHECK_VAL(res == BT_ERROR_NONE, res);

	res = bt_deinitialize();
	MP_CHECK_VAL(res == BT_ERROR_NONE, res);

	bt_avrcp_unset_shuffle_mode_changed_cb();
	bt_avrcp_unset_equalizer_state_changed_cb();
	bt_avrcp_unset_repeat_mode_changed_cb();

	IF_FREE(gMpAvrcpCb);
	return res;
}

int mp_avrcp_set_mode_change_cb(mp_avrcp_shuffle_changed_cb s_cb,
	mp_avrcp_repeat_changed_cb r_cb, mp_avrcp_eq_changed_cb e_cb, void *user_data)
{
	startfunc;
	gMpAvrcpCb->s_cb = s_cb;
	gMpAvrcpCb->r_cb = r_cb;
	gMpAvrcpCb->e_cb = e_cb;

	bt_avrcp_set_equalizer_state_changed_cb(_mp_avrcp_equalizer_state_changed_cb, user_data);
	bt_avrcp_set_shuffle_mode_changed_cb( _mp_avrcp_shuffle_mode_changed_cb, user_data);
	bt_avrcp_set_repeat_mode_changed_cb( _mp_avrcp_repeat_mode_changed_cb, user_data);

	return 0;
}

int mp_avrcp_noti_player_state(mp_avrcp_player_state_e state)
{
	DEBUG_TRACE("state: %d", state);
	bt_avrcp_player_state_e player_state = BT_AVRCP_PLAYER_STATE_STOPPED;
	switch(state)
	{
	case MP_AVRCP_STATE_STOPPED:
		player_state = BT_AVRCP_PLAYER_STATE_STOPPED;
		break;
	case MP_AVRCP_STATE_PLAYING:
		player_state = BT_AVRCP_PLAYER_STATE_PLAYING;
		break;
	case MP_AVRCP_STATE_PAUSED:
		player_state = BT_AVRCP_PLAYER_STATE_PAUSED;
		break;
	case MP_AVRCP_STATE_REW:
		player_state = BT_AVRCP_PLAYER_STATE_REWIND_SEEK;
		break;
	case MP_AVRCP_STATE_FF:
		player_state = BT_AVRCP_PLAYER_STATE_FORWARD_SEEK;
		break;
	default:
		break;
	}
	return bt_avrcp_target_notify_player_state(player_state);
}
int mp_avrcp_noti_eq_state(mp_avrcp_eq_state_e eq)
{
	DEBUG_TRACE("state: %d", eq);
	bt_avrcp_equalizer_state_e state = BT_AVRCP_EQUALIZER_STATE_OFF;
	if(eq == MP_AVRCP_EQ_ON)
		state = BT_AVRCP_EQUALIZER_STATE_ON;

	return bt_avrcp_target_notify_equalizer_state(state);
}

int mp_avrcp_noti_repeat_mode(mp_avrcp_repeat_mode_e repeat)
{
	DEBUG_TRACE("mode: %d", repeat);
	bt_avrcp_repeat_mode_e state = BT_AVRCP_REPEAT_MODE_OFF;
	switch(repeat)
	{
	case MP_AVRCP_REPEAT_OFF:
		state = BT_AVRCP_REPEAT_MODE_OFF;
		break;
	case MP_AVRCP_REPEAT_ONE:
		state = BT_AVRCP_REPEAT_MODE_SINGLE_TRACK;
		break;
	case MP_AVRCP_REPEAT_ALL:
		state = BT_AVRCP_REPEAT_MODE_ALL_TRACK;
		break;
	default:
		break;
	}
	return bt_avrcp_target_notify_repeat_mode(state);
}

int mp_avrcp_noti_shuffle_mode(mp_avrcp_shuffle_mode_e shuffle)
{
	DEBUG_TRACE("mode: %d", shuffle);
	bt_avrcp_shuffle_mode_e state = BT_AVRCP_SHUFFLE_MODE_OFF;
	switch(shuffle)
	{
	case MP_AVRCP_SHUFFLE_OFF:
		state = BT_AVRCP_SHUFFLE_MODE_OFF;
		break;
	case MP_AVRCP_SHUFFLE_ON:
		state = BT_AVRCP_SHUFFLE_MODE_ALL_TRACK;
		break;
	default:
		break;
	}
	return bt_avrcp_target_notify_shuffle_mode(state);

}

int mp_avrcp_noti_track(const char *title, const char *artist, const char *album, const char *genre, unsigned int duration)
{
	DEBUG_TRACE("title: %s", title);
	return bt_avrcp_target_notify_track(title, artist, album, genre, 0, 0, duration);
}

