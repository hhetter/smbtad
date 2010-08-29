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
 * receive a data block. return 0 if the connection
 * was closed, or return the received buffer
 * int sock 		The handle
 * int length		The number of bytes to
 * 			receive
 */
int network_receive_data( char *buf, int sock, int length, int *rlen)
{
	size_t t;
	t = recv( sock, buf, length, 0);
	
	if (t == 0) {
		DEBUG(1) syslog(LOG_DEBUG,
			"network_receive_data: recv 0 bytes "
			"closing connection.");
		/* connection closed */
		*rlen = 0;
		return 0;
	}
	*(buf + t) = '\0';
	*rlen = *rlen + t;
	DEBUG(1) syslog(LOG_DEBUG,
		"network_receive_data: received %i bytes, >>%s<<",(int) t,buf);
	return t;
}

/**
 * Create a v2 header.
 * TALLLOC_CTX *ctx             Talloc context to work on
 * const char *state_flags      State flag string
 * int len                      length of the data block
 */
char *network_create_header( TALLOC_CTX *ctx,
        const char *state_flags, size_t data_len)
{
        char *header = talloc_asprintf( ctx, "V2.%s%017u",
		state_flags, (unsigned int) data_len);
	DEBUG(1) syslog(LOG_DEBUG,
		"network_create_header: created header :%s",header);
        return header;
}


/**
 * Accept an incoming connection
 * and add it to the list of connections.
 */
int network_accept_connection( config_t *c, struct sockaddr_in *remote, int type)
{
	socklen_t t=sizeof(*remote);
	int sr;
	int sock;
	if (type == SOCK_TYPE_DATA) 	sock = c->vfs_socket;
	if (type == SOCK_TYPE_DB_QUERY)	sock = c->query_socket;
	if ( (sr = accept( sock,
			(struct sockaddr *) remote, &t)) == -1) {
		syslog(LOG_DEBUG,"ERROR: accept failed.");
		return -1;
	}
	connection_list_add(sr, type);
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
	monitor_list_delete_by_socket(connection->mysocket);
	connection_list_remove(connection->mysocket);
	syslog(LOG_DEBUG,"network_close_connection: closed connection number "
		"%i, socket %i",i,connection->mysocket);
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
	int l;
     	struct connection_struct *connection =
		connection_list_identify(i);
        if (connection->connection_function == SOCK_TYPE_DATA ||
	    connection->connection_function == SOCK_TYPE_DB_QUERY) {
		switch(connection->data_state) {
		case CONN_READ_HEADER: ;
			connection->CTX = talloc(0,char);
			connection->header =
				talloc_array(connection->CTX, char, 29);
			connection->header_position = 0;
			DEBUG(1) syslog(LOG_DEBUG,
				"network_handle_data: recv header...");
			l = network_receive_data( connection->header, i,26,
				&connection->header_position);
			if ( l == 0) {
				network_close_connection(i);
				break;
			}

			if (connection->header_position != 26) {
				connection->data_state = 
					CONN_READ_HEADER_ONGOING;
				break;
			}
			protocol_check_header(connection->header);
			connection->data_state = CONN_READ_DATA;
			connection->blocklen =
				protocol_get_data_block_length(connection->header);
			connection->encrypted = 
				protocol_is_encrypted(connection->header);
			DEBUG(1) syslog(LOG_DEBUG,
				"network_handle_data: header complete.");
			break;
		case CONN_READ_HEADER_ONGOING:
			DEBUG(1) syslog(LOG_DEBUG,
				"network_handle_data: incomplete header,"
				"recv ongoing...");
			l = network_receive_data(
				connection->header + connection->header_position,
				i,
				26 - connection->header_position,
				&connection->header_position);
			if ( l == 0 ) {
				network_close_connection(i);
				break;
			}
			if (connection->header_position != 26) break;
			/* full header */
			protocol_check_header(connection->header);
                        connection->data_state = CONN_READ_DATA;
                        connection->blocklen =
                                protocol_get_data_block_length(connection->header);
                        connection->encrypted =
				protocol_is_encrypted(connection->header);
			DEBUG(1) syslog(LOG_DEBUG,
				"network_handle_data: header complete.");
                        break;

		case CONN_READ_DATA: ;
			DEBUG(1) syslog(LOG_DEBUG,
				"network_handle_data: recv data block");
			connection->body = 
				talloc_array(connection->CTX,
						char,
						connection->blocklen +2); 
			connection->body_position = 0;
			l = network_receive_data(connection->body,
					i,
					connection->blocklen,
					&connection->body_position);
			if ( l == 0 ) {
				network_close_connection(i);
				break;
			}
			if (connection->body_position != connection->blocklen) {
				connection->data_state = CONN_READ_DATA_ONGOING;
				break;
			}
			/* we have the full data block, encrypt if needed */
			if ( connection->encrypted == 1) {
				connection->body =
					protocol_decrypt(connection->CTX,
						connection->body,
						connection->blocklen,
						c->key);
			}

			connection->data_state = CONN_READ_HEADER;

			if (connection->connection_function == SOCK_TYPE_DATA) {
				if (c->dbg == 1) D_(LOG_DEBUG,
					"Adding to cache:  %s | len = %i",
					connection->body,
					connection->blocklen); 
				cache_add(connection->body, connection->blocklen);
			}

			if (connection->connection_function == SOCK_TYPE_DB_QUERY) {
				if (c->dbg == 1) D_(LOG_DEBUG,
					"Adding to queries: %s | len = %i",
					connection->body,
					connection->blocklen);
				query_add(connection->body, connection->blocklen,i,0);
			}

			TALLOC_FREE(connection->CTX);
			break;
		case CONN_READ_DATA_ONGOING: ;
			DEBUG(1) syslog(LOG_DEBUG,"network_handle_data: "
				"recv incomplete data block.");
			l = network_receive_data(
				connection->body + connection->body_position,
				i,
				connection->blocklen - connection->body_position,
				&connection->body_position);
			if ( l == 0) {
				network_close_connection(i);
				break;
			}
			if (connection->body_position != connection->blocklen)
				break;
			/* we finally have the full data block, encrypt if needed */
			if ( connection->encrypted == 1) {
				connection->body =
					protocol_decrypt(connection->CTX,
						connection->body,
						connection->blocklen,
						c->key);
			}
			connection->data_state = CONN_READ_HEADER;
			if (connection->connection_function == SOCK_TYPE_DATA) {
				DEBUG(1) syslog(LOG_DEBUG,
					"Adding to cache %s | len = %i",
					connection->body,
					connection->blocklen);
				cache_add(connection->body, connection->blocklen);
			}
			if (connection->connection_function == SOCK_TYPE_DB_QUERY) {
				DEBUG(1) syslog(LOG_DEBUG,
					"Adding to queries %s | len = %i",
					connection->body,
					connection->blocklen);
				query_add(connection->body, connection->blocklen,i,0);
			}
			TALLOC_FREE(connection->CTX);
			break;
			
		}
	}
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

	bzero (&my_addr, sizeof (my_addr));
	my_addr.sin6_family = AF_INET6;
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
	c->query_socket = network_create_socket( c->query_port );

	connection_list_add( c->vfs_socket, SOCK_TYPE_DATA );
	connection_list_add( c->query_socket, SOCK_TYPE_DB_QUERY);
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
			monitor_list_process(i);
			if (FD_ISSET(i,&read_fd_set)) {
				int sr;
				if ( i == c->vfs_socket)
					sr = network_accept_connection(
						c,
						&remote,
						SOCK_TYPE_DATA); 
				else if ( i == c->query_socket)
					sr = network_accept_connection(
						c,
						&remote,
						SOCK_TYPE_DB_QUERY);
				else {
					network_handle_data(i,c);
				}
			}
		}
		/*
		 * send a query result to a client, if our runtime
		 * data structure has a prepared result in the queue
		 */
		sendlist_send(&write_fd_set);

		/*
		 * process monitors
		 *
		 */
/*
		for ( i = 0; i < connection_list_max() + 1; ++i)
			if (FD_ISSET(i,&write_fd_set)) monitor_list_process(i);
*/				
	}
}
