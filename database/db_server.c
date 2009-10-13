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

#include "database.h"
#include "log.h"

#include <errno.h>
#include <string.h>
#include <dbi/dbi.h>

/**
 * Create servers from a database
 *
 * @param c the config containing the connection info
 *
 * @return the servers followed by a NULL pointer
 */
struct server **db_create_servers(struct config *c)
{
	dbi_result res;
	char *q = "SELECT * FROM servers WHERE active = 1;";
	int nb_serv;
	struct server **ss;
	int i;

	res = dbi_conn_query(c->conn, q);
	nb_serv = dbi_result_get_numrows(res);
	if (nb_serv == 0) {
		return NULL;
	}
	ss = (struct server **)calloc(nb_serv + 1, sizeof(struct server *));
	if (ss == NULL) {
		logger(LOG_WARN, "db_create_server : calloc failed : %s.", strerror(errno));
		return NULL;
	}

	if (res) {
		for (i = 0 ; dbi_result_next_row(res) ; i++) {
			ss[i] = new_server();
			ss[i]->conf = c;

			ss[i]->id = dbi_result_get_uint(res, "id");
			strcpy(ss[i]->password, dbi_result_get_string(res, "password"));
			strcpy(ss[i]->server_name, dbi_result_get_string(res, "name"));
			strcpy(ss[i]->welcome_msg, dbi_result_get_string(res, "welcome_msg"));
			ss[i]->port = dbi_result_get_uint(res, "port");
			/* codecs is a bitfield */
			ss[i]->codecs =
				(dbi_result_get_uint(res, "codec_celp51") << CODEC_CELP_5_1) |
				(dbi_result_get_uint(res, "codec_celp63") << CODEC_CELP_6_3) |
				(dbi_result_get_uint(res, "codec_gsm148") << CODEC_GSM_14_8) |
				(dbi_result_get_uint(res, "codec_gsm164") << CODEC_GSM_16_4) |
				(dbi_result_get_uint(res, "codec_celp52") << CODEC_CELPWin_5_2) |
				(dbi_result_get_uint(res, "codec_speex2150") << CODEC_SPEEX_3_4) |
				(dbi_result_get_uint(res, "codec_speex3950") << CODEC_SPEEX_5_2) |
				(dbi_result_get_uint(res, "codec_speex5950") << CODEC_SPEEX_7_2) |
				(dbi_result_get_uint(res, "codec_speex8000") << CODEC_SPEEX_9_3) |
				(dbi_result_get_uint(res, "codec_speex11000") << CODEC_SPEEX_12_3) |
				(dbi_result_get_uint(res, "codec_speex15000") << CODEC_SPEEX_16_3) |
				(dbi_result_get_uint(res, "codec_speex18200") << CODEC_SPEEX_19_6) |
				(dbi_result_get_uint(res, "codec_speex24600") << CODEC_SPEEX_25_9);
		}
		dbi_result_free(res);
	}
	return ss;
}

