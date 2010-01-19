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
	SOCK_TYPE_INCOMING_DATA,
	SOCK_TYPE_DB_QUERY
};


struct connection_struct {
	struct connection_struct *next;
	int mysocket;
	enum conn_fn_enum connection_function;
};

int connection_list_add( int socket, enum conn_fn_enum conn_fn );
int connection_list_remove( int socket );
int connection_list_max();

