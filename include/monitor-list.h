/* 
 * stad 
 * capture transfer data from the vfs_smb_traffic_analyzer module, and store
 * the data via various plugins
 *
 * Copyright (C) Holger Hetterich, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


/**
 * Monitor Protocol
 * Data incoming from a monitor always begins with ~~
 * This makes smbtad differ it from a SQL query
 *
 * ID and optional parameters      |        FILTER PATTERN
 * ~~|LEN|MONITOR_FN_ENUM|LEN|PARAM|LEN|USERNAME|LEN|USERSID|LEN|SHARE|LEN|FILE|LEN|DOMAIN
 *
 */

#include "monitor-fn-enum.h"


enum monitor_states { MONITOR_IDENTIFY = 0, MONITOR_INITIALIZE, MONITOR_PROCESS, MONITOR_STOP, MONITOR_ERROR };

struct monitor_item {
	char *data;

	/* data delivered with the protocol */
	enum monitor_fn function;
	char *param; /* optional parameter such as R RW W */
	char *username;
	char *usersid;
	char *share;
	char *file;
	char *domain;

	/* to be casted to a specific structure */
	void *local_data;

	
	int length;
	int sock;
	int id;
	enum monitor_states state;
	struct monitor_item *next;
};

struct monitor_local_data_adder {
	unsigned long int sum;
};

struct monitor_local_data_total {
	unsigned long int sum;
};

int monitor_list_add( char *data,int sock);
void monitor_list_process(int sock);
void monitor_list_update( int op_id,
        char *username,
        char *usersid,
        char *share,
        char *domain, char *data);

void monitor_list_delete_by_socket( int sock );
void monitor_list_set_init_result(char *res, int monitorid);


