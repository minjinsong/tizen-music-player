
#include "mp-playlist-mgr.h"
#include "mp-player-debug.h"
#include "mp-define.h"

void __mp_playlist_mgr_item_free(void *data)
{
	mp_plst_item *node = (mp_plst_item *)data;
	MP_CHECK(node);
	if(node->is_queue)
	{
		DEBUG_TRACE("queued item will be remained");
		return;
	}

	IF_FREE(node->uri);
	IF_FREE(node->uid);
	IF_FREE(node);
}

mp_plst_mgr *mp_playlist_mgr_create(void)
{
	startfunc;
	mp_plst_mgr *playlist_mgr = calloc(1, sizeof(mp_plst_mgr));
	srand((unsigned int)time(NULL));
	endfunc;
	return playlist_mgr;

}

void mp_playlist_mgr_destroy(mp_plst_mgr *playlist_mgr)
{
	startfunc;
	MP_CHECK(playlist_mgr);
	mp_playlist_mgr_clear(playlist_mgr);
	free(playlist_mgr);
	endfunc;
}

static int __mp_playlist_mgr_rand_position(int length, int queue_lenth)
{
	unsigned int seed = 0;
	unsigned int rand = 0;
	int pos = 0;

	if(length > 0)
	{
		rand = rand_r(&seed);
		pos =  rand%(length-queue_lenth+1);
	}

	return pos;
}

static void __mp_playlist_mgr_select_list(mp_plst_mgr *playlist_mgr)
{
	startfunc;
	if(playlist_mgr->shuffle_state)
		playlist_mgr->list = playlist_mgr->shuffle_list;
	else
		playlist_mgr->list = playlist_mgr->normal_list;
}

mp_plst_item * mp_playlist_mgr_item_append(mp_plst_mgr *playlist_mgr, const char *uri, const char *uid, mp_track_type type)
{
	DEBUG_TRACE("uri: %s, uid:%s, type: %d", uri, uid, type);
	MP_CHECK_VAL(playlist_mgr, NULL);
	MP_CHECK_VAL(uri, NULL);

	int pos;
	mp_plst_item * node, *cur = NULL;
	int queue_lenth;

	/*create data*/
	node = calloc(1, sizeof(mp_plst_item));
	MP_CHECK_VAL(node, NULL);

	node->track_type = type;
	node->uri = g_strdup(uri);
	node->uid = g_strdup(uid);


	/*insert to normal list*/
	playlist_mgr->normal_list = g_list_append(playlist_mgr->normal_list, node);

	/*insert to shuffle list*/
	queue_lenth = g_list_length(playlist_mgr->queue_list);
	pos = __mp_playlist_mgr_rand_position(g_list_length(playlist_mgr->normal_list), queue_lenth);

	int queue_start = g_list_index(playlist_mgr->shuffle_list, g_list_nth_data(playlist_mgr->queue_list, 0));
	if(pos >= queue_start-1)
		pos += queue_lenth;

	if(playlist_mgr->shuffle_state)
		cur = mp_playlist_mgr_get_current(playlist_mgr);

	playlist_mgr->shuffle_list = g_list_insert(playlist_mgr->shuffle_list, node, pos);

	if(cur)
	{
		int index= g_list_index(playlist_mgr->list, cur);
		playlist_mgr->current_index = index;
	}

	/*select list*/
	__mp_playlist_mgr_select_list(playlist_mgr);

	return node;
}

static GList * __mp_playlist_mgr_delete_queue_link(GList *list)
{
	startfunc;
	mp_plst_item *item;
	GList *new_list, *remove;
	int idx = 0;

	new_list = list;
	remove = g_list_nth(new_list, idx);
	MP_CHECK_NULL(remove);

	while(remove)
	{
		item = remove->data;

		if(item->is_queue)
		{
			DEBUG_TRACE("delete : %s", item->uid);
			new_list = g_list_remove_link(new_list, remove);
			g_list_free(remove);
		}
		else
			idx++;

		remove = g_list_nth(new_list, idx);
	}

	return new_list;
}

static void __mp_playlist_mgr_index(mp_plst_mgr *playlist_mgr, int *pos, int *shuffle_pos)
{
	MP_CHECK(playlist_mgr);
	int idx, s_idx;
	if(playlist_mgr->shuffle_state)
	{
		s_idx = playlist_mgr->current_index;
		idx = g_list_index(playlist_mgr->normal_list, g_list_nth_data(playlist_mgr->shuffle_list, s_idx));
	}
	else
	{
		idx = playlist_mgr->current_index;
		s_idx = g_list_index(playlist_mgr->shuffle_list, g_list_nth_data(playlist_mgr->normal_list, idx));
	}

	*pos = idx;
	*shuffle_pos = s_idx;
}

static void __mp_playlist_mgr_clear_queue(mp_plst_mgr *playlist_mgr)
{
	startfunc;
	int idx, s_idx;

	MP_CHECK(playlist_mgr);

	__mp_playlist_mgr_index(playlist_mgr, &idx, &s_idx);
	DEBUG_TRACE("idx: %d, s_idx: %d", idx, s_idx);

	playlist_mgr->normal_list = __mp_playlist_mgr_delete_queue_link(playlist_mgr->normal_list);
	playlist_mgr->shuffle_list = __mp_playlist_mgr_delete_queue_link(playlist_mgr->shuffle_list);

	__mp_playlist_mgr_select_list(playlist_mgr);

	endfunc;
}

static void
__mp_playlist_mgr_insert_queue_links(mp_plst_mgr *playlist_mgr)
{
	GList *list;
	int idx, s_idx;

	MP_CHECK(playlist_mgr);

	__mp_playlist_mgr_index(playlist_mgr, &idx, &s_idx);

	list = g_list_last(playlist_mgr->queue_list);
	MP_CHECK(list);

	idx++;
	s_idx++;

	do
	{
		playlist_mgr->normal_list = g_list_insert(playlist_mgr->normal_list, list->data, idx);
		playlist_mgr->shuffle_list= g_list_insert(playlist_mgr->shuffle_list, list->data, s_idx);
		list = g_list_previous(list);
	}while(list);

	if(playlist_mgr->queue_item_cb)
		playlist_mgr->queue_item_cb(MP_PLSYLIST_QUEUE_MOVED, playlist_mgr->shuffle_state? s_idx: idx, playlist_mgr->userdata);
}

mp_plst_item * mp_playlist_mgr_item_queue(mp_plst_mgr *playlist_mgr, const char *uri, const char *uid, mp_track_type type)
{
	DEBUG_TRACE("uri: %s, uid:%s", uri, uid);
	MP_CHECK_VAL(playlist_mgr, NULL);

	mp_plst_item *p_data = NULL;
	GList *last;
	int pos, s_pos;

	if(playlist_mgr->queue_list)
	{
		last = g_list_last(playlist_mgr->queue_list);
		if(last)
			p_data = last->data;
	}
	/*create data*/
	mp_plst_item *node = calloc(1, sizeof(mp_plst_item));
	MP_CHECK_VAL(node, NULL);

	if(uri)
	{
		node->track_type = EINA_TRUE;
		node->uri = g_strdup(uri);
	}
	node->uid = g_strdup(uid);
	node->is_queue = EINA_TRUE;

	/*append item*/
	playlist_mgr->queue_list = g_list_append(playlist_mgr->queue_list, node);

	/*insert queue items to list*/
	if(p_data)
	{
		pos = g_list_index(playlist_mgr->normal_list, p_data)+1;
		s_pos = g_list_index(playlist_mgr->shuffle_list, p_data)+1;
	}
	else
	{
		__mp_playlist_mgr_index(playlist_mgr, &pos, &s_pos);
		pos++; s_pos++;
	}
	playlist_mgr->normal_list = g_list_insert( playlist_mgr->normal_list, node, pos);
	playlist_mgr->shuffle_list= g_list_insert( playlist_mgr->shuffle_list, node, s_pos);

	/*select list */
	__mp_playlist_mgr_select_list(playlist_mgr);

	if(playlist_mgr->queue_item_cb)
		playlist_mgr->queue_item_cb(MP_PLAYLIST_QUEUE_ADDED, playlist_mgr->shuffle_state?s_pos:pos, playlist_mgr->userdata);

	return node;
}

void mp_playlist_mgr_item_remove_item(mp_plst_mgr *playlist_mgr, mp_plst_item *item)
{
	startfunc;
	GList *remove;
	MP_CHECK(playlist_mgr);
	MP_CHECK(item);

	MP_CHECK(playlist_mgr->shuffle_list);
	MP_CHECK(playlist_mgr->normal_list);

	/*remove from shuffle_list*/
	remove = g_list_find(playlist_mgr->shuffle_list, item);
	MP_CHECK(remove);

	playlist_mgr->shuffle_list = g_list_remove_link(playlist_mgr->shuffle_list, remove);
	g_list_free(remove);

	/*remove from normal_list*/
	remove = g_list_find(playlist_mgr->normal_list, item);
	MP_CHECK(remove);

	item->is_queue = EINA_FALSE;

	playlist_mgr->normal_list = g_list_remove_link(playlist_mgr->normal_list, remove);
	g_list_free_full(remove, __mp_playlist_mgr_item_free);

	/*remove from queue list*/
	if(playlist_mgr->queue_list)
	{
		remove = g_list_find(playlist_mgr->queue_list, item);
		if(remove)
		{
			playlist_mgr->queue_list = g_list_remove_link(playlist_mgr->queue_list, remove);
			g_list_free(remove);
		}
	}

	/*select list*/
	__mp_playlist_mgr_select_list(playlist_mgr);
	endfunc;
}

void mp_playlist_mgr_item_remove_nth(mp_plst_mgr *playlist_mgr, int index)
{
	startfunc;
	MP_CHECK(playlist_mgr);
	MP_CHECK(playlist_mgr->list);
	GList *link = g_list_nth(playlist_mgr->list, index);
	MP_CHECK(link);

	mp_playlist_mgr_item_remove_item(playlist_mgr, link->data);
	endfunc;
}

void mp_playlist_mgr_clear(mp_plst_mgr *playlist_mgr)
{
	startfunc;
	MP_CHECK(playlist_mgr);

	if(playlist_mgr->normal_list)
		g_list_free_full(playlist_mgr->normal_list,  __mp_playlist_mgr_item_free);
	if(playlist_mgr->shuffle_list)
		g_list_free(playlist_mgr->shuffle_list);

	if(playlist_mgr->queue_list)
	{
		playlist_mgr->normal_list = g_list_copy(playlist_mgr->queue_list);
		playlist_mgr->shuffle_list = g_list_copy(playlist_mgr->queue_list);

		__mp_playlist_mgr_select_list(playlist_mgr);
	}
	else
	{
		playlist_mgr->normal_list = NULL;
		playlist_mgr->shuffle_list = NULL;
		playlist_mgr->list = NULL;
	}

	playlist_mgr->current_index = 0;

	if(playlist_mgr->item_change_cb)
		playlist_mgr->item_change_cb(NULL, playlist_mgr->item_change_userdata);

	endfunc;
}

int mp_playlist_mgr_count(mp_plst_mgr *playlist_mgr)
{
	//startfunc;
	MP_CHECK_VAL(playlist_mgr, 0);
	MP_CHECK_VAL(playlist_mgr->list, 0);
	return g_list_length(playlist_mgr->list);
}

mp_plst_item *mp_playlist_mgr_get_current(mp_plst_mgr *playlist_mgr)
{
	MP_CHECK_VAL(playlist_mgr, 0);
	mp_plst_item *cur = NULL;

	if(playlist_mgr->list)
		cur = g_list_nth_data(playlist_mgr->list, playlist_mgr->current_index);

	return cur;
}

static void
__mp_playlist_list_foreach(gpointer data, gpointer user_data)
{
	int pos;
	mp_plst_mgr *playlist_mgr = user_data;
	MP_CHECK(playlist_mgr);

	pos = __mp_playlist_mgr_rand_position(g_list_length(playlist_mgr->shuffle_list), 0);

	playlist_mgr->shuffle_list = g_list_insert(playlist_mgr->shuffle_list, data, pos);
}

static void
__mp_playlist_mgr_shuffle(mp_plst_mgr *playlist_mgr)
{
	MP_CHECK(playlist_mgr);

	g_list_free(playlist_mgr->shuffle_list);
	playlist_mgr->shuffle_list = NULL;

	g_list_foreach(playlist_mgr->normal_list, __mp_playlist_list_foreach, playlist_mgr);
	playlist_mgr->list = playlist_mgr->shuffle_list;
}

mp_plst_item *mp_playlist_mgr_get_next(mp_plst_mgr *playlist_mgr, Eina_Bool force)
{
	startfunc;
	MP_CHECK_VAL(playlist_mgr, NULL);
	MP_CHECK_VAL(playlist_mgr->list, NULL);
	int index = 0;
	int count = 0;

	count = mp_playlist_mgr_count(playlist_mgr);

	if (playlist_mgr->repeat_state == MP_PLST_REPEAT_ONE && !force)
		index = playlist_mgr->current_index;
	else
		index = playlist_mgr->current_index + 1;

	if (count == index)
	{
		if (playlist_mgr->repeat_state == MP_PLST_REPEAT_ALL || force)
		{
			if(playlist_mgr->shuffle_state)
				__mp_playlist_mgr_shuffle(playlist_mgr);
			index = 0;
		}
		else
			return NULL;
	}

	if(index >= count)
	{
		DEBUG_TRACE("End of playlist");
		index = 0;
	}

	return (mp_plst_item *)g_list_nth_data(playlist_mgr->list, index);

}

mp_plst_item *mp_playlist_mgr_get_prev(mp_plst_mgr *playlist_mgr)
{
	startfunc;
	MP_CHECK_VAL(playlist_mgr, NULL);
	MP_CHECK_VAL(playlist_mgr->list, NULL);
	int index = 0;

	index = playlist_mgr->current_index;
	index --;
	if(index<0)
	{
		DEBUG_TRACE("Begin of playlist. ");
		index = mp_playlist_mgr_count(playlist_mgr) -1;
	}

	return (mp_plst_item *)g_list_nth_data(playlist_mgr->list, index);
}

mp_plst_item *mp_playlist_mgr_get_nth(mp_plst_mgr *playlist_mgr, int index)
{
	startfunc;
	MP_CHECK_VAL(playlist_mgr, NULL);
	MP_CHECK_VAL(playlist_mgr->list, NULL);
	return (mp_plst_item *)g_list_nth_data(playlist_mgr->list, index);
}

void mp_playlist_mgr_set_current(mp_plst_mgr *playlist_mgr, mp_plst_item *cur)
{
	startfunc;
	MP_CHECK(playlist_mgr);
	MP_CHECK(playlist_mgr->list);

	int index;
	mp_plst_item *before;
	bool call_remove_item_callback = false;
	int before_index = 0;
	bool insert_queue = false;

	if(!cur)
	{
		cur = mp_playlist_mgr_get_nth(playlist_mgr, 0);
	}
	MP_CHECK(cur);

	/*remove queue item*/
	before = mp_playlist_mgr_get_current(playlist_mgr);
	if(before && before->is_queue)
	{
		DEBUG_TRACE("queue spent");
		before_index = playlist_mgr->current_index;

		call_remove_item_callback = true;

		mp_playlist_mgr_item_remove_item(playlist_mgr, before);
	}

	/*clear queue item if needed*/
	if(!cur->is_queue && playlist_mgr->queue_list)
	{
		__mp_playlist_mgr_clear_queue(playlist_mgr);
		insert_queue = true;
	}

	/*set current*/
	index= g_list_index(playlist_mgr->list, cur);
	if(index < 0)
	{
		WARN_TRACE("No such item!! cur: %x", cur);
		goto finish;
	}
	playlist_mgr->current_index = index;

	/*insert queue item after cur*/
	if(insert_queue)
		__mp_playlist_mgr_insert_queue_links(playlist_mgr);

finish:
	if(call_remove_item_callback && playlist_mgr->queue_item_cb)
		playlist_mgr->queue_item_cb(MP_PLSYLIST_QUEUE_REMOVED, before_index, playlist_mgr->userdata);

	if(playlist_mgr->item_change_cb)
		playlist_mgr->item_change_cb(cur, playlist_mgr->item_change_userdata);

	return;
}

int mp_playlist_mgr_get_current_index(mp_plst_mgr *playlist_mgr)
{
	startfunc;
	MP_CHECK_VAL(playlist_mgr, -1);
	return playlist_mgr->current_index;
}

void mp_playlist_mgr_set_shuffle(mp_plst_mgr *playlist_mgr, Eina_Bool shuffle)
{
	startfunc;
	MP_CHECK(playlist_mgr);

	playlist_mgr->shuffle_state = shuffle;

	if(playlist_mgr->list)
	{
		mp_plst_item *cur;
		cur = mp_playlist_mgr_get_current(playlist_mgr);

		__mp_playlist_mgr_select_list(playlist_mgr);

		int index= g_list_index(playlist_mgr->list, cur);
		playlist_mgr->current_index = index;
	}
	endfunc;
}

Eina_Bool mp_playlist_mgr_is_shuffle(mp_plst_mgr *playlist_mgr)
{
	//startfunc;
	MP_CHECK_VAL(playlist_mgr, 0);
	return playlist_mgr->shuffle_state;
}

void mp_playlist_mgr_set_repeat(mp_plst_mgr *playlist_mgr, mp_plst_repeat_state repeat)
{
	startfunc;
	MP_CHECK(playlist_mgr);

	playlist_mgr->repeat_state = repeat;
	endfunc;
}

int mp_playlist_mgr_get_repeat(mp_plst_mgr *playlist_mgr)
{
	startfunc;
	MP_CHECK_VAL(playlist_mgr, 0);

	return playlist_mgr->repeat_state;
	endfunc;
}


Eina_Bool mp_playlist_mgr_set_queue_cb(mp_plst_mgr* playlist_mgr, mp_queue_item_removed_cb cb, void *userdata)
{
	MP_CHECK_VAL(playlist_mgr, 0);

	playlist_mgr->userdata = userdata;
	playlist_mgr->queue_item_cb = cb;

	return true;
}

int mp_playlist_mgr_set_item_change_callback(mp_plst_mgr *playlist_mgr, mp_playlist_item_change_callback cb, void *userdata)
{
	MP_CHECK_VAL(playlist_mgr, 0);

	playlist_mgr->item_change_userdata = userdata;
	playlist_mgr->item_change_cb = cb;

	return 0;
}


