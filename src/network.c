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
char *network_receive_header( TALLOC_CTX *ctx, int sock )
{
	char *buf = talloc_array( ctx, char, 30); // malloc( sizeof(char) * 30 );
	size_t t;
	t = recv( sock, buf, 26, 0);
	if ( t == 0 ) {
		/* connection closed */
		free(buf);
		return NULL;
	}
	*(buf + t) = '\0';
	return buf;
}


/**
 * receive a data block. return 0 if the connection
 * was closed, or return the received buffer
 * int sock 		The handle
 * int length		The number of bytes to
 * 			receive
 */
char *network_receive_data( TALLOC_CTX *ctx, int sock, int length)
{
	char *buf = talloc_array( ctx, char, length +2); // malloc( sizeof(char) * (length+2) );
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
	socklen_t t=sizeof(*remote);
	int sr;
	if ( (sr = accept(c->vfs_socket,
			(struct sockaddr *) remote, &t)) == -1) {
		syslog(LOG_DEBUG,"ERROR: accept failed.");
		return -1;
	}
	connection_list_add(sr, SOCK_TYPE_DATA);
	return sr;
}


/**
 * Close a network connection.
 * int i		The handle
 */
void network_close_connection(int i)
{
	struct connection_struct *connection = connection_list_identify(i);
	close(connection->mysocket);
	connection_list_remove(connection->mysocket);
}


/**
 * Handle incoming data.
 * int i 		The handle
 *
 * Receive the connection struct for the handle by identifying
 * it, if it's SOCK_TYPE_DATA, receive either a header, or
 * a data block. If the header has been received, set the
 * connection_struct->data_state to CONN_READ_DATA to indicate
 * that a data block will come. If a data block has been
 * received, set the state to CONN_READ_HEADER
 */
int network_handle_data( int i, config_t *c )
{
	char *context = talloc( NULL, char);
     	struct connection_struct *connection =
		connection_list_identify(i);
        if (connection->connection_function == SOCK_TYPE_DATA) {
		switch(connection->data_state) {
		case CONN_READ_HEADER: ;
			char *header = network_receive_header( context, i);
			if (header == NULL) {
				network_close_connection(i);
				break;
			}
			protocol_check_header(header);
			connection->data_state = CONN_READ_DATA;
			connection->blocklen =
				protocol_get_data_block_length( header );
			connection->encrypted = protocol_is_encrypted( header );
			break;
		case CONN_READ_DATA: ;
			char *body = 
				network_receive_data(context, i, connection->blocklen);
			if (body == NULL) {
				network_close_connection(i);
				break;
			}
			if ( connection->encrypted == 1) {
				body = protocol_decrypt(context, body, connection->blocklen, c->key);
			}
			connection->data_state = CONN_READ_HEADER;
			cache_add(body, connection->blocklen);
			break;

		}
	}
	talloc_free(context);
	return 0;
}


/**
 * Create a listening internet socket on a port.
 * int port		The port-number.
 */	 
int network_create_socket( int port )
{
	int sock_fd;
	struct sockaddr_in6 my_addr;

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

/**
 * Run select() on the server handle, and call
 * the required functions to handle data.
 * config_t *c		A configuration structure.
 */
void network_handle_connections( config_t *c )
{
	int i;
	int z=0;
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
					network_handle_data(i,c);
		}
	}
}
