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

enum conn_fn_enum {
	SOCK_TYPE_DATA,
	SOCK_TYPE_DB_QUERY
};

enum conn_data_state {
	CONN_READ_HEADER,
	CONN_READ_DATA,
	CONN_READ_DATA_ONGOING 
};

struct connection_struct {
	struct connection_struct *next;
	int mysocket;
	int blocklen;
	enum conn_fn_enum connection_function;
	enum conn_data_state data_state;
	int header_position;
	char *header;
	int body_position;
	char *body;
	int encrypted;
};

struct connection_struct *connection_list_identify( int socket );
int connection_list_add( int socket, enum conn_fn_enum conn_fn );
int connection_list_remove( int socket );
int connection_list_max();
void connection_list_recreate_fs_sets(  fd_set *active_read_fd_set,
                                        fd_set *active_write_fd_set,
                                        fd_set *read_fd_set,
                                        fd_set *write_fd_set);


