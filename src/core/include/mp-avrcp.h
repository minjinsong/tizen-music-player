
#ifndef __MP_AVRCP_H__
#define __MP_AVRCP_H__

#include <stdbool.h>

typedef enum{
	MP_AVRCP_STATE_STOPPED,
	MP_AVRCP_STATE_PLAYING,
	MP_AVRCP_STATE_PAUSED,
	MP_AVRCP_STATE_FF,
	MP_AVRCP_STATE_REW,
}mp_avrcp_player_state_e;

typedef enum{
	MP_AVRCP_REPEAT_OFF,
	MP_AVRCP_REPEAT_ONE,
	MP_AVRCP_REPEAT_ALL,
}mp_avrcp_repeat_mode_e;

typedef enum{
	MP_AVRCP_SHUFFLE_OFF,
	MP_AVRCP_SHUFFLE_ON,
}mp_avrcp_shuffle_mode_e;

typedef enum{
	MP_AVRCP_EQ_OFF,
	MP_AVRCP_EQ_ON,
}mp_avrcp_eq_state_e;

typedef void (*mp_avrcp_connection_state_changed_cb) (bool connected, const char *remote_address, void *user_data);
typedef void (*mp_avrcp_shuffle_changed_cb)(mp_avrcp_shuffle_mode_e mode, void *user_data);
typedef void (*mp_avrcp_repeat_changed_cb)(mp_avrcp_repeat_mode_e mode, void *user_data);
typedef void (*mp_avrcp_eq_changed_cb)(mp_avrcp_eq_state_e state, void *user_data);

int mp_avrcp_target_initialize(mp_avrcp_connection_state_changed_cb callback, void *userdata);
int mp_avrcp_target_finalize(void);

int mp_avrcp_add_mode_change_cb(mp_avrcp_shuffle_changed_cb s_cb, mp_avrcp_repeat_changed_cb r_cb, mp_avrcp_eq_changed_cb e_cb, void *user_data);

int mp_avrcp_noti_player_state(mp_avrcp_player_state_e state);
int mp_avrcp_noti_eq_state(mp_avrcp_eq_state_e eq);
int mp_avrcp_noti_repeat_mode(mp_avrcp_repeat_mode_e repeat);
int mp_avrcp_noti_shuffle_mode(mp_avrcp_shuffle_mode_e shuffle);
int mp_avrcp_noti_track(const char *title, const char *artist, const char *album, const char *genre, unsigned int duration);

int mp_avrcp_set_mode_change_cb(mp_avrcp_shuffle_changed_cb s_cb,
	mp_avrcp_repeat_changed_cb r_cb, mp_avrcp_eq_changed_cb e_cb, void *user_data);

#endif

