#include <utilX.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <sound_manager.h>
#include "mp-define.h"
#include "mp-volume.h"


typedef struct {
	Ecore_X_Window xwin;
	bool condition[MP_VOLUME_KEY_GRAB_COND_MAX];
	bool grabbed;

	/* key event callback */
	Mp_Volume_Key_Event_Cb key_event_cb;
	void *key_event_user_data;

	/* volume change callback */
	Mp_Volume_Change_Cb volume_change_cb;

	Ecore_Timer *pressed_timer;
} MpVolumeKeyMgr_t;

static MpVolumeKeyMgr_t g_volume_key_mgr;


void
mp_volume_key_grab_set_window(Ecore_X_Window xwin)
{
	g_volume_key_mgr.xwin = xwin;

}

static void
_mp_volume_key_grab_check_condition()
{
	bool start = true;
	int condition = 0;
	while (condition < MP_VOLUME_KEY_GRAB_COND_MAX) {
		if (!g_volume_key_mgr.condition[condition]) {
			/* do NOT start */
			start = false;
			break;
		}
		condition++;
	}

	/* start key grab */
	if (start)
		mp_volume_key_grab_start();
	else
		mp_volume_key_grab_end();
}

void
mp_volume_key_grab_condition_set(mp_volume_key_grab_condition_e condition, bool enabled)
{
	MP_CHECK(condition < MP_VOLUME_KEY_GRAB_COND_MAX);

	/* set condition */
	g_volume_key_mgr.condition[condition] = enabled;
	WARN_TRACE("VOL key grab condition(%d) changed => [%d]", condition, enabled);

	_mp_volume_key_grab_check_condition();
}

bool
mp_volume_key_grab_start()
{
	MP_CHECK_FALSE(g_volume_key_mgr.xwin);

	int error = 0;
	Ecore_X_Display* disp = ecore_x_display_get();

	error = utilx_grab_key(disp, g_volume_key_mgr.xwin, KEY_VOLUMEUP, OR_EXCLUSIVE_GRAB);
	if (error != 0) {
		mp_error("utilx_grab_key(KEY_VOLUMEUP)... [0x%x]", error);
		return false;
	}

	error = utilx_grab_key(disp, g_volume_key_mgr.xwin, KEY_VOLUMEDOWN, OR_EXCLUSIVE_GRAB);
	if (error != 0) {
		mp_error("utilx_grab_key(KEY_VOLUMEDOWN)... [0x%x]", error);
		utilx_ungrab_key(disp, g_volume_key_mgr.xwin, KEY_VOLUMEUP);
		return false;
	}

	error = vconf_set_int(VCONFKEY_STARTER_USE_VOLUME_KEY, 1);
	if (error != 0) {
		mp_error("vcont_set_int()...[0x%x]", error);
	}

	WARN_TRACE("START_volume_key_grab");
	g_volume_key_mgr.grabbed = true;
	return true;
}

void
mp_volume_key_grab_end()
{
	Ecore_X_Display* disp = ecore_x_display_get();
	utilx_ungrab_key(disp, g_volume_key_mgr.xwin, KEY_VOLUMEUP);
	utilx_ungrab_key(disp, g_volume_key_mgr.xwin, KEY_VOLUMEDOWN);

	if (g_volume_key_mgr.pressed_timer && g_volume_key_mgr.key_event_cb) {
		g_volume_key_mgr.key_event_cb(g_volume_key_mgr.key_event_user_data, MP_VOLUME_KEY_DOWN, true);
	}
	mp_ecore_timer_del(g_volume_key_mgr.pressed_timer);

	WARN_TRACE("STOP_volume_key_grab");
	g_volume_key_mgr.grabbed = false;
}

static Eina_Bool
_mp_volume_key_pressed_timer(void *data)
{
	volume_key_type_e type = (int)data;

	if (g_volume_key_mgr.key_event_cb) {
		g_volume_key_mgr.key_event_cb(g_volume_key_mgr.key_event_user_data, type, false);
	}

	return ECORE_CALLBACK_RENEW;
}

void _mp_volume_changed_cb(sound_type_e type, unsigned int volume, void *user_data)
{
	if(type == SOUND_TYPE_MEDIA)
	{
		DEBUG_TRACE("");
		if(g_volume_key_mgr.volume_change_cb)
			g_volume_key_mgr.volume_change_cb(volume, user_data);
	}
}


void
mp_volume_key_event_send(mp_volume_key_e type, bool released)
{
	WARN_TRACE("volume key[%d], released[%d]", type, released);

	if (!g_volume_key_mgr.grabbed) {
		WARN_TRACE("already ungrabbed.. ignore this event");
		return;
	}

	if (!released && g_volume_key_mgr.pressed_timer) {
		/* long press timer is working*/
		return;
	}

	mp_ecore_timer_del(g_volume_key_mgr.pressed_timer);

	if (g_volume_key_mgr.key_event_cb) {
		/* send callback */
		g_volume_key_mgr.key_event_cb(g_volume_key_mgr.key_event_user_data, type, released);
	}

	if (!released)
		g_volume_key_mgr.pressed_timer = ecore_timer_add(0.2, _mp_volume_key_pressed_timer, (void *)type);
}

void
mp_volume_key_event_callback_add(Mp_Volume_Key_Event_Cb event_cb, void *user_data)
{
	g_volume_key_mgr.key_event_cb = event_cb;
	g_volume_key_mgr.key_event_user_data = user_data;
}

void
mp_volume_key_event_callback_del()
{
	g_volume_key_mgr.key_event_cb = NULL;
	g_volume_key_mgr.key_event_user_data = NULL;
	mp_ecore_timer_del(g_volume_key_mgr.pressed_timer);
}

int
mp_volume_get_max()
{
	int max_vol = 0;

	int ret = sound_manager_get_max_volume(SOUND_TYPE_MEDIA, &max_vol);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		mp_error("sound_manager_get_max_volume().. [0x%x]", ret);
		return -1;
	}

	return max_vol;
}

int
mp_volume_get_current()
{
	int current = 0;

	int ret = sound_manager_get_volume(SOUND_TYPE_MEDIA, &current);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		mp_error("sound_manager_get_max_volume().. [0x%x]", ret);
		return -1;
	}

	return current;
}

bool
mp_volume_set(int volume)
{
	int ret = sound_manager_set_volume(SOUND_TYPE_MEDIA, volume);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		mp_error("sound_manager_set_volume().. [0x%x]", ret);
		return false;
	}

	WARN_TRACE("set volue [%d]", volume);
	return true;
}

bool
mp_volume_up()
{
	int current = mp_volume_get_current();
	int max = mp_volume_get_max();

	bool ret = true;
	if (current < max)
		ret = mp_volume_set(current+1);

	return ret;
}

bool
mp_volume_down()
{
	int current = mp_volume_get_current();

	bool ret = true;
	if (current > 0)
		ret = mp_volume_set(current-1);

	return ret;
}

void
mp_volume_add_change_cb(Mp_Volume_Change_Cb cb, void *user_data)
{
	if(g_volume_key_mgr.volume_change_cb) return;

	startfunc;
	int res = SOUND_MANAGER_ERROR_NONE;
	if(cb)
	{
		g_volume_key_mgr.volume_change_cb = cb;
		res = sound_manager_set_volume_changed_cb(_mp_volume_changed_cb, user_data);
		if(res != SOUND_MANAGER_ERROR_NONE)
		{
			ERROR_TRACE("Error: sound_manager_set_volume_changed_cb");
		}
	}

	endfunc;
}


