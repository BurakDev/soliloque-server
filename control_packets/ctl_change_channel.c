/*
 * soliloque-server, an open source implementation of the TeamSpeak protocol.
 * Copyright (C) 2009 Hugo Camboulive <hugo.camboulive AT gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "control_packet.h"

#include "player.h"
#include "channel.h"
#include "server.h"
#include "log.h"
#include "packet_tools.h"
#include "server_stat.h"
#include "acknowledge_packet.h"
#include "database.h"

#include <errno.h>
#include <string.h>

/**
 * Send a notification to all  players indicating that
 * a channel's name has changed
 *
 * @param pl the player who changed it
 * @param ch the channel changed
 * @param name the new name
 */
static void s_resp_chan_name_changed(struct player *pl, struct channel *ch, char *name)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;
	size_t iter;

	/* header size (24) + chan_id (4) + user_id (4) + name (?) */
	data_size = 24 + 4 + 4 + (strlen(name) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_chan_name_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_CH_NAME, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(ch->id, &ptr);			/* channel changed */
	wu32(pl->public_id, &ptr);		/* player who changed */
	strcpy(ptr, name);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a player request to change a channel's name
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the change
 */
void *c_req_change_chan_name(char *data, unsigned int len, struct player *pl)
{
	uint32_t ch_id;
	char *name, *ptr;
	struct channel *ch;

	send_acknowledge(pl);

	ptr = data + 24;
	ch_id = ru32(&ptr);
	ch = get_channel_by_id(pl->in_chan->in_server, ch_id);

	if (ch != NULL) {
		if (player_has_privilege(pl, SP_CHA_CHANGE_NAME, ch)) {
			name = strdup(data + 28);
			free(ch->name);
			ch->name = name;
			/* Update the channel in the db if it is registered */
			if ((ch_getflags(ch) & CHANNEL_FLAG_UNREGISTERED) == 0) {
				db_update_channel(ch->in_server->conf, ch);
			}
			s_resp_chan_name_changed(pl, ch, name);
		}
	}
	return NULL;
}

/**
 * Send a notification to all players indicating that
 * a channel's topic has changed
 *
 * @param pl the player who changed it
 * @param ch the channel changed
 * @param topic the new topic
 */
static void s_resp_chan_topic_changed(struct player *pl, struct channel *ch, char *topic)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;
	size_t iter;

	/* header size (24) + chan_id (4) + user_id (4) + name (?) */
	data_size = 24 + 4 + 4 + (strlen(topic) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_chan_topic_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_CH_TOPIC, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(ch->id, &ptr);			/* channel changed */
	wu32(pl->public_id, &ptr);		/* player who changed */
	strcpy(ptr, topic);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a player request to change a channel's topic
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the change
 */
void *c_req_change_chan_topic(char *data, unsigned int len, struct player *pl)
{
	uint32_t ch_id;
	char *topic, *ptr;
	struct channel *ch;

	send_acknowledge(pl);

	ptr = data + 24;
	ch_id = ru32(&ptr);
	ch = get_channel_by_id(pl->in_chan->in_server, ch_id);

	if (ch != NULL) {
		if (player_has_privilege(pl, SP_CHA_CHANGE_TOPIC, ch)) {
			topic = strdup(data + 28); /* FIXME : possible exploit */
			free(ch->topic);
			ch->topic = topic;
			/* Update the channel in the db if it is registered */
			if ((ch_getflags(ch) & CHANNEL_FLAG_UNREGISTERED) == 0) {
				db_update_channel(ch->in_server->conf, ch);
			}
			s_resp_chan_topic_changed(pl, ch, topic);
		}
	}
	return NULL;
}

/**
 * Send a notification to all  players indicating that
 * a channel's description has changed
 *
 * @param pl the player who changed it
 * @param ch the channel changed
 * @param desc the new description
 */
static void s_resp_chan_desc_changed(struct player *pl, struct channel *ch, char *desc)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;
	size_t iter;

	/* header size (24) + chan_id (4) + user_id (4) + name (?) */
	data_size = 24 + 4 + 4 + (strlen(desc) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_chan_desc_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_CH_DESC, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(ch->id, &ptr);			/* channel changed */
	wu32(pl->public_id, &ptr);		/* player who changed */
	strcpy(ptr, desc);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a player request to change a channel's description
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the change
 */
void *c_req_change_chan_desc(char *data, unsigned int len, struct player *pl)
{
	uint32_t ch_id;
	char *desc, *ptr;
	struct channel *ch;

	send_acknowledge(pl);

	ptr = data + 24;
	ch_id = ru32(&ptr);
	ch = get_channel_by_id(pl->in_chan->in_server, ch_id);

	if (ch != NULL) {
		if (player_has_privilege(pl, SP_CHA_CHANGE_DESC, ch)) {
			desc = strdup(data + 28);	/* FIXME : possible exploit */
			free(ch->desc);
			ch->desc = desc;
			/* Update the channel in the db if it is registered */
			if ((ch_getflags(ch) & CHANNEL_FLAG_UNREGISTERED) == 0) {
				db_update_channel(ch->in_server->conf, ch);
			}
			s_resp_chan_desc_changed(pl, ch, desc);
		}
	}
	return NULL;
}

static void s_notify_channel_flags_codec_changed(struct player *pl, struct channel *ch)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;
	size_t iter;

	/* header size (24) + chan_id (4) + user_id (4) + name (?) */
	data_size = 24 + 4 + 4 + 2 + 2;
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_channel_flags_codec_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_CH_FLAGS_CODEC, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(ch->id, &ptr);			/* channel changed */
	wu16(ch->flags, &ptr);			/* new channel flags */
	wu16(ch->codec, &ptr);			/* new codec */
	wu32(pl->public_id, &ptr);		/* player who changed */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a request to change the channel flags
 * or change the codec of a channel.
 *
 * @param data the packet
 * @param len the length of data
 * @param pl the player who issued the request
 */
void *c_req_change_chan_flag_codec(char *data, unsigned int len, struct player *pl)
{
	uint16_t new_flags, flags;
	uint16_t new_codec;
	uint32_t ch_id;
	struct channel *ch;
	int priv_nok;
	struct server *s;
	char *ptr;

	send_acknowledge(pl);
	s =  pl->in_chan->in_server;
	priv_nok = 0;

	ptr = data + 24;
	ch_id = ru32(&ptr);
	new_flags = ru16(&ptr);
	new_codec = ru16(&ptr);

	ch = get_channel_by_id(s, ch_id);
	if (ch == NULL)
		return NULL;

	flags = ch_getflags(ch);
	/* For each flag, check if it is changed, and if we have the permission to change it */
	/* Registered / unregistered flag */
	if ((flags & CHANNEL_FLAG_UNREGISTERED) != (new_flags & CHANNEL_FLAG_UNREGISTERED)) {
		if ((new_flags & CHANNEL_FLAG_UNREGISTERED)  /* we want to unregister the channel */
				&& !player_has_privilege(pl, SP_CHA_CREATE_UNREGISTERED, NULL))
			priv_nok++;
		if (!(new_flags & CHANNEL_FLAG_UNREGISTERED) /* we want to register the channel */
				&& !player_has_privilege(pl, SP_CHA_CREATE_REGISTERED, ch))
			priv_nok++;
	}
	/* default flag */
	if ((flags & CHANNEL_FLAG_DEFAULT) != (new_flags & CHANNEL_FLAG_DEFAULT)
			&& !player_has_privilege(pl, SP_CHA_CREATE_DEFAULT, ch))
		priv_nok++;
	/* moderated flag */
	if ((flags & CHANNEL_FLAG_MODERATED) != (new_flags & CHANNEL_FLAG_MODERATED)
			&& !player_has_privilege(pl, SP_CHA_CREATE_MODERATED, ch))
		priv_nok++;
	/* subchannels flag */
	if ((flags & CHANNEL_FLAG_SUBCHANNELS) != (new_flags & CHANNEL_FLAG_SUBCHANNELS)
			&& !player_has_privilege(pl, SP_CHA_CREATE_SUBCHANNELED, ch))
		priv_nok++;
	/* password flag */
	if ((flags & CHANNEL_FLAG_PASSWORD) != (new_flags & CHANNEL_FLAG_PASSWORD)
			&& !player_has_privilege(pl, SP_CHA_CHANGE_PASS, ch))
		priv_nok++;
	/* codec changed ? */
	if ((ch->codec != new_codec)
			&& !player_has_privilege(pl, SP_CHA_CHANGE_CODEC, ch))
		priv_nok++;

	/* Do the actual work */
	if (priv_nok == 0) {
		/* The flags of a subchannel cannot be changed */
		if (ch->parent == NULL) {
			ch->flags = new_flags;
			if ((ch_getflags(ch) & CHANNEL_FLAG_PASSWORD) != 0)
				bzero(ch_getpass(ch), 30 * sizeof(char));
		}
		ch->codec = new_codec;
		/* If the channel changed registered or unregistered */
		if ( (flags & CHANNEL_FLAG_UNREGISTERED) != (new_flags & CHANNEL_FLAG_UNREGISTERED)) {
			if (new_flags & CHANNEL_FLAG_UNREGISTERED) {
				/* unregister the channel */
				db_unregister_channel(s->conf, ch);
			} else {
				/* register the channel */
				db_register_channel(s->conf, ch);
			}
		} else if ((flags & CHANNEL_FLAG_UNREGISTERED) == 0) {
			/* update the channel in the database */
			db_update_channel(s->conf, ch);
		}
		s_notify_channel_flags_codec_changed(pl, ch);
	}
	return NULL;
}

/**
 * Handle a request to change a channel password.
 *
 * @param data the request packet
 * @param len the length of the packet
 * @param pl the player issuing the request
 */
void *c_req_change_chan_pass(char *data, unsigned int len, struct player *pl)
{
	char *password;
	struct channel *ch;
	uint32_t ch_id;
	struct server *s;
	uint16_t old_flags;
	char *ptr;

	s = pl->in_chan->in_server;

	ptr = data + 24;
	ch_id = ru32(&ptr);
	password = rstaticstring(29, &ptr);

	ch = get_channel_by_id(s, ch_id);
	send_acknowledge(pl);
	if (ch != NULL && player_has_privilege(pl, SP_CHA_CHANGE_PASS, ch) && ch->parent == NULL) {
		logger(LOG_INFO, "Change channel password : %s->%s", ch->name, password);
		old_flags = ch_getflags(ch);

		/* We either remove or change the password */
		if (strlen(password) == 0) {
			logger(LOG_ERR, "This should not happened. Password removal is done using the change flags/codec function.");
			bzero(ch->password, 30 * sizeof(char));
			ch->flags &= (~CHANNEL_FLAG_PASSWORD);
		} else {
			bzero(ch->password, 30 * sizeof(char));
			strcpy(ch->password, password);
			ch->flags |= CHANNEL_FLAG_PASSWORD;
		}
		free(password);
		/* If we change the password when there is already one, the channel
		 * flags do not change, no need to notify. */
		if (old_flags != ch_getflags(ch)) {
			s_notify_channel_flags_codec_changed(pl, ch);
		}
		/* Update the channel in the db if it is registered */
		if ((ch_getflags(ch) & CHANNEL_FLAG_UNREGISTERED) == 0) {
			db_update_channel(s->conf, ch);
		}
	}
	return NULL;
}

/**
 * Notify all players that the sort order for a channel changed.
 *
 * @param pl the player who changed the max users
 * @param ch the channel whose max users changed
 */
static void s_notify_channel_order_changed(struct player *pl, struct channel *ch)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;
	size_t iter;

	/* header size (24) + chan_id (4) + user_id (4) + sort order (2) */
	data_size = 24 + 4 + 2 + 4;
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_channel_order_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_CH_ORDER, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(ch->id, &ptr);			/* channel changed */
	wu16(ch->sort_order, &ptr);		/* new sort order */
	wu32(pl->public_id, &ptr);		/* player who changed */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a request from a player to change a channel's sort order
 *
 * @param data the data packet
 * @param len the length of data
 * @param pl the player who made the request
 *
 * @return NULL
 */
void *c_req_change_chan_order(char *data, unsigned int len, struct player *pl)
{
	uint16_t order;
	uint32_t ch_id;
	struct channel *ch;
	struct server *s = pl->in_chan->in_server;
	char *ptr;

	ptr = data + 24;
	ch_id = ru32(&ptr);
	order = ru16(&ptr);

	ch = get_channel_by_id(s, ch_id);
	send_acknowledge(pl);
	if (ch != NULL && player_has_privilege(pl, SP_CHA_CHANGE_ORDER, ch)) {
		ch->sort_order = order;
		if ((ch_getflags(ch) & CHANNEL_FLAG_UNREGISTERED) == 0) {
			db_update_channel(s->conf, ch);
		}
		s_notify_channel_order_changed(pl, ch);
	}
	return NULL;
}

/**
 * Notify all players that the number of max users for a channel changed.
 *
 * @param pl the player who changed the max users
 * @param ch the channel whose max users changed
 */
static void s_notify_channel_max_users_changed(struct player *pl, struct channel *ch)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;
	size_t iter;

	/* header size (24) + chan_id (4) + user_id (4) + nb users (2) */
	data_size = 24 + 4 + 2 + 4;
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_channel_max_users_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_CH_MAX_USERS, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(ch->id, &ptr);			/* channel changed */
	wu16(ch->players->max_slots, &ptr);	/* new channel flags */
	wu32(pl->public_id, &ptr);		/* player who changed */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a request from a player to change a channel's max users value.
 *
 * @param data the data packet
 * @param len the length of data
 * @param pl the player who made the request
 *
 * @return NULL
 */
void *c_req_change_chan_max_users(char *data, unsigned int len, struct player *pl)
{
	uint16_t max_users;
	uint32_t ch_id;
	struct channel *ch;
	struct server *s = pl->in_chan->in_server;
	char *ptr;

	ptr = data + 24;
	ch_id = ru32(&ptr);
	max_users = ru16(&ptr);

	ch = get_channel_by_id(s, ch_id);
	send_acknowledge(pl);
	if (ch != NULL && player_has_privilege(pl, SP_CHA_CHANGE_MAXUSERS, ch)) {
		ch->players->max_slots = max_users;
		if ((ch_getflags(ch) & CHANNEL_FLAG_UNREGISTERED) == 0) {
			db_update_channel(s->conf, ch);
		}
		s_notify_channel_max_users_changed(pl, ch);
	}
	return NULL;
}

