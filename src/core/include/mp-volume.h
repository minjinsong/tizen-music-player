#ifndef __MP_VOLMUE_H__
#define __MP_VOLMUE_H__

#include <stdbool.h>
#include <Ecore_X.h>

typedef enum {
	MP_VOLUME_KEY_DOWN,
	MP_VOLUME_KEY_UP,
} mp_volume_key_e;

typedef enum {
	MP_VOLUME_KEY_GRAB_COND_WINDOW_FOCUS,
	MP_VOLUME_KEY_GRAB_COND_VIEW_VISIBLE,
	MP_VOLUME_KEY_GRAB_COND_MAX,
} mp_volume_key_grab_condition_e;


typedef void (*Mp_Volume_Key_Event_Cb)(void *user_data, mp_volume_key_e key, bool released);
typedef void (*Mp_Volume_Change_Cb)(int volume, void *user_data);

#ifdef __cplusplus
extern "C" {
#endif

void mp_volume_key_grab_set_window(Ecore_X_Window xwin);
void mp_volume_key_grab_condition_set(mp_volume_key_grab_condition_e condition, bool enabled);
bool mp_volume_key_grab_start();
void mp_volume_key_grab_end();
void mp_volume_key_event_send(mp_volume_key_e type, bool released);
void mp_volume_key_event_callback_add(Mp_Volume_Key_Event_Cb event_cb, void *user_data);
void mp_volume_key_event_callback_del();
int mp_volume_get_max();
int mp_volume_get_current();
bool mp_volume_set(int volume);
bool mp_volume_up();
bool mp_volume_down();
void mp_volume_add_change_cb(Mp_Volume_Change_Cb cb, void *user_data);

#ifdef __cplusplus
}
#endif


#endif /* __MP_VOLMUE_H__ */

