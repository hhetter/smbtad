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

#include "../include/includes.h"


struct connection_struct *connection_list_start = NULL;
struct connection_struct *connection_list_end = NULL;


int connection_list_add( int socket,
			enum conn_fn_enum conn_fn,
			char *addrstr,
			struct configuration_data *c)
{
	struct connection_struct *new_entry =
		malloc(sizeof(struct connection_struct));

	if (connection_list_start == NULL) {
		connection_list_start = new_entry;
		connection_list_end = new_entry;
		new_entry->mysocket = socket;
		new_entry->next = NULL;
		new_entry->connection_function = conn_fn;
		new_entry->data_state = CONN_READ_HEADER;
		new_entry->CTX = talloc_pool( NULL,2048);
	} else {
		new_entry->mysocket = socket;
		connection_list_end->next = new_entry;
		connection_list_end = new_entry;
		new_entry->connection_function = conn_fn;
		new_entry->data_state = CONN_READ_HEADER;
		new_entry->next = NULL;
		new_entry->CTX = talloc_pool( NULL, 2048);
	}
	/**
	 * if addr and uaddr are both NULL, this is an
	 * internal connection that doesn't need to be
	 * registered in the module table.
	 */
	if (addrstr == NULL) { /* set INTERNAL HERE */
		new_entry->internal = 1;
	} else { /**
		 * We store the clients IP adress or
		 * unix socket address with the connection list
		 */
		strcpy(new_entry->addrstr, addrstr);
	}
	/**
	 * If a connection is accepted, we set
	 * stored to 0. Upon the first incoming data
	 * we set stored to 1, and store version
	 * and ip address in the database
	 */
	new_entry->stored = 0;
	return 0;
}


int connection_list_remove( int socket )
{
	struct connection_struct *Searcher = connection_list_start;
	struct connection_struct *Prev = NULL;

	while (Searcher != NULL) {
		if ( Searcher->mysocket == socket ) {
			if ( Prev == NULL ) {
				/* first entry */
				connection_list_start = Searcher->next;
				TALLOC_FREE(Searcher->CTX);
				free(Searcher);
				return 0;
			}
			
			Prev->next = Searcher->next;
			if (Searcher == connection_list_end)
				connection_list_end=Prev;
			TALLOC_FREE(Searcher->CTX);
			free(Searcher);
			return 0;
		}
	Prev = Searcher;
	Searcher = Searcher->next;
	}
	return 0;
}

void connection_list_recreate_fs_sets( 	fd_set *active_read_fd_set,
					fd_set *active_write_fd_set)
{
	FD_ZERO(active_read_fd_set);
	FD_ZERO(active_write_fd_set);
	struct connection_struct *Searcher = connection_list_start;

	while (Searcher != NULL) {
		FD_SET(Searcher->mysocket, active_read_fd_set);
		FD_SET(Searcher->mysocket, active_write_fd_set);
		Searcher = Searcher->next;
	}
}

struct connection_struct *connection_list_begin()
{
	return connection_list_start;
}

int connection_list_max()
{
	struct connection_struct *Searcher = connection_list_start;
	int cc=0;

	while (Searcher != NULL) {
		if (Searcher->mysocket > cc) cc = Searcher->mysocket;
		Searcher = Searcher->next;
	}

	return cc;
}

struct connection_struct *connection_list_identify( int socket ) {
	struct connection_struct *Searcher = connection_list_start;
	while (Searcher != NULL) {
		if (Searcher->mysocket == socket) return Searcher;
		Searcher = Searcher->next;
	}
	return NULL;
}


