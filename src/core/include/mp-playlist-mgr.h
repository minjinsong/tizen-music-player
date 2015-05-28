
#ifndef __MP_PLAY_LIST_H_
#define __MP_PLAY_LIST_H_

#include <glib.h>
#include <Eina.h>
#include "mp-define.h"

typedef enum{
	MP_TRACK_URI,
}mp_track_type;

typedef enum _mp_plst_repeat_state
{
	MP_PLST_REPEAT_ALL,
	MP_PLST_REPEAT_NONE,
	MP_PLST_REPEAT_ONE,
}mp_plst_repeat_state;

typedef enum{
	MP_PLAYLIST_QUEUE_ADDED,
	MP_PLSYLIST_QUEUE_REMOVED,
	MP_PLSYLIST_QUEUE_MOVED,
}mp_playlist_queue_cmd_type;

typedef void (*mp_queue_item_removed_cb)(mp_playlist_queue_cmd_type cmd_type, int index,void *userdata);

typedef struct _mp_list_item
{
	mp_track_type track_type;
	Eina_Bool is_queue;
	char *uri;		//local track uri..
	char *uid;  	//unique id (media_id or allshare item id)
}mp_plst_item;

typedef void (*mp_playlist_item_change_callback)(mp_plst_item *item, void *userdata);

typedef struct _mp_plst_mgr
{
	int current_index;
	Eina_Bool shuffle_state;	//shuffle on/off
	mp_plst_repeat_state repeat_state; //off:0/one:1/all:2
	GList *list;		//normal list do not free, just refer normal_list or shuffle_list
	GList *normal_list;
	GList *shuffle_list;
	GList *queue_list;

	void *userdata;
	void(*queue_item_cb)(mp_playlist_queue_cmd_type cmd_type, int index,void *userdata);

	void *item_change_userdata;
	mp_playlist_item_change_callback item_change_cb;


} mp_plst_mgr;


mp_plst_mgr *mp_playlist_mgr_create(void);
void mp_playlist_mgr_destroy(mp_plst_mgr *playlist_mgr);

mp_plst_item * mp_playlist_mgr_item_append(mp_plst_mgr *playlist_mgr, const char *uri, const char *uid, mp_track_type type);
mp_plst_item * mp_playlist_mgr_item_queue(mp_plst_mgr *playlist_mgr, const char *uri, const char *uid, mp_track_type type);

void mp_playlist_mgr_item_remove_item(mp_plst_mgr *playlist_mgr, mp_plst_item *item);
void mp_playlist_mgr_item_remove_nth(mp_plst_mgr *playlist_mgr, int index);
void mp_playlist_mgr_clear(mp_plst_mgr *playlist_mgr);

int mp_playlist_mgr_count(mp_plst_mgr *playlist_mgr);
mp_plst_item *mp_playlist_mgr_get_current(mp_plst_mgr *playlist_mgr);
mp_plst_item *mp_playlist_mgr_get_next(mp_plst_mgr *playlist_mgr, Eina_Bool force);
mp_plst_item *mp_playlist_mgr_get_prev(mp_plst_mgr *playlist_mgr);
mp_plst_item *mp_playlist_mgr_get_nth(mp_plst_mgr *playlist_mgr, int index);

void mp_playlist_mgr_set_current(mp_plst_mgr *playlist_mgr, mp_plst_item *cur);
int mp_playlist_mgr_get_current_index(mp_plst_mgr *playlist_mgr);

void mp_playlist_mgr_set_shuffle(mp_plst_mgr *playlist_mgr, Eina_Bool shuffle);
Eina_Bool mp_playlist_mgr_is_shuffle(mp_plst_mgr *playlist_mgr);
void mp_playlist_mgr_set_repeat(mp_plst_mgr *playlist_mgr, mp_plst_repeat_state repeat);
int mp_playlist_mgr_get_repeat(mp_plst_mgr *playlist_mgr);
Eina_Bool mp_playlist_mgr_set_queue_cb(mp_plst_mgr* playlist_mgr, mp_queue_item_removed_cb queue_item_removed, void *userdata);
int mp_playlist_mgr_set_item_change_callback(mp_plst_mgr *playlist_mgr, mp_playlist_item_change_callback cb, void *userdata);

#endif




