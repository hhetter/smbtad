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


/**
 * receive the protocol header, always fixed size.
 * should we receive 0 bytes, connection was closed,
 * we return NULL
 */
char *network_receive_header( int sock )
{
	char *buf = malloc( sizeof(char) * 30 );
	size_t t;
	t = recv( sock, buf, 28, 0);
	if ( t == 0 ) {
		/* connection closed */
		free(buf);
		return NULL;
	}
	*(buf + t + 1) = '\0';
	return buf;
}

char *network_receive_data( int sock, int length)
{
	char *buf = malloc( sizeof(char) * (length+2) );
	size_t t;
	t = recv( sock, buf, length, 0);
	if (t == 0) {
		/* connection closed */
		free(buf);
		return 0;
	}
	*(buf + t + 1) = '\0';
	return buf;
}


/**
 * Accept an incoming connection from the VFS module,
 * and add it to the list of connections.
 */
int network_accept_VFS_connection( config_t *c, struct sockaddr_in *remote)
{
	int t=sizeof(*remote);
	int sr;
	if ( (sr = accept(c->vfs_socket,
			(struct sockaddr *) remote, &t)) == -1) {
		syslog(LOG_DEBUG,"ERROR: accept failed.");
		return -1;
	}
	connection_list_add(sr, SOCK_TYPE_DATA);
	return sr;
}


void network_close_connection(int i)
{
	struct connection_struct *connection = connection_list_identify(i);
	close(connection->mysocket);
	connection_list_remove(connection->mysocket);
}


int network_handle_data( int i )
{
     	struct connection_struct *connection =
		connection_list_identify(i);
        if (connection->connection_function == SOCK_TYPE_DATA) {
		switch(connection->data_state) {
		case CONN_READ_HEADER: ;
			char *header = network_receive_header(i);
			if (header == NULL) {
				network_close_connection(i);
				break;
			}
			protocol_check_header(header);
			connection->data_state = CONN_READ_DATA;
			connection->blocklen =
				protocol_get_data_block_length( header );
			free(header);
			break;
		case CONN_READ_DATA: ;
			char *body = 
				network_receive_data(i, connection->blocklen);
			if (body == NULL) {
				network_close_connection(i);
				break;
			}
			connection->data_state = CONN_READ_HEADER;
			free(body);
			break;

		}
	}
}


	 
int network_create_socket( int port )
{
	int sock_fd, new_fd;
	struct sockaddr_in6 my_addr;
	struct sockaddr_in6 their_addr;
	socklen_t sin_size;
	struct sigaction sa;

	if ( (sock_fd = socket(AF_INET6, SOCK_STREAM,0)) == -1 ) {
		syslog( LOG_DEBUG, "ERROR: socket creation failed." );
		exit(1);
	}

	int y;
	if ( setsockopt( sock_fd, SOL_SOCKET, SO_REUSEADDR, &y,
		sizeof( int )) == -1 ) {
		syslog( LOG_DEBUG, "ERROR: setsockopt failed." );
		exit(1);
	}

	my_addr.sin6_port = htons( port );
	my_addr.sin6_addr = in6addr_any;

	if (bind(sock_fd,(struct sockaddr *)&my_addr,sizeof(my_addr)) == -1 ) {
		syslog( LOG_DEBUG, "ERROR: bind failed." );
		exit(1);
	}

	if ( listen( sock_fd, 50 ) == -1 ) {
		syslog( LOG_DEBUG, "ERROR: listen failed." );
		exit(1);
	}

	return sock_fd;
}


void network_handle_connections( config_t *c )
{
	int i;
	int z=0;
	int sr;
	int t;
	struct sockaddr_in remote;
	t=sizeof(remote);
	fd_set read_fd_set, active_read_fd_set;
	fd_set write_fd_set, active_write_fd_set;

	FD_ZERO(&active_read_fd_set );
	FD_ZERO(&active_write_fd_set );

	c->vfs_socket = network_create_socket( c->port );

	connection_list_add( c->vfs_socket, SOCK_TYPE_DATA );

	for (;;) {
		z = 0;
		connection_list_recreate_fs_sets(
			&active_read_fd_set,
			&active_write_fd_set,
			&read_fd_set,
			&write_fd_set);
		z = select( connection_list_max() +1,
			&read_fd_set, &write_fd_set, NULL,NULL);
		if (z < 0) {
			syslog(LOG_DEBUG,"ERROR: select error.");
			exit(1);
		}
		for( i = 0; i < connection_list_max() + 1; ++i) {
			if (FD_ISSET(i,&read_fd_set) && i == c->vfs_socket) {
				int sr;
				sr = network_accept_VFS_connection(c, &remote);
			} else 	if (FD_ISSET(i, &read_fd_set))
					network_handle_data(i);
		}
	}
}
