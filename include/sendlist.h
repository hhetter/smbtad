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

enum sendlist_item_state {
	SENDLIST_STATUS_SEND_HEADER = 0,
	SENDLIST_STATUS_SEND_HEADER_ONGOING,
	SENDLIST_STATUS_SEND_DATA,
	SENDLIST_STATUS_SEND_DATA_ONGOING};

struct sendlist_item {
	char *data;
	int len;
	int sock;
	char *header;
	int send_len;
	enum sendlist_item_state state;
	struct sendlist_item *next;
};

pthread_mutex_t sendlist_lock;
void sendlist_init();
int sendlist_add( char *data,int sock, int length);
int sendlist_send( fd_set *write_fd_set );
void sendlist_list();
