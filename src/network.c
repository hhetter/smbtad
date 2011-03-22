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
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
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
		"network_receive_data: received %i bytes, >>%s<<",
		(int) t,buf);
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
int network_accept_connection( config_t *c,
	struct sockaddr_in *remote_inet,
	struct sockaddr_un *remote_unix,
	int type)
{
	socklen_t t;
	if ( c->unix_socket ==1 ) t=sizeof(*remote_unix);
	else t=sizeof(*remote_inet);
	int sr;
	int sock = 0;
	if (type == SOCK_TYPE_DATA) 	sock = c->vfs_socket;
	if (type == SOCK_TYPE_DB_QUERY)	sock = c->query_socket;
	if ( (c->unix_socket == 1 && type == SOCK_TYPE_DATA) || 
		(c->unix_socket_clients == 1 && type == SOCK_TYPE_DB_QUERY)) {
		if ( (sr = accept( sock,
				(struct sockaddr *) remote_unix, &t)) == -1) {
			syslog(LOG_DEBUG,
				"ERROR: accept (unix socket) failed.");
			return -1;
		}
	} else {
		if ( (sr = accept( sock,
				(struct sockaddr *) remote_inet, &t)) == -1) {
			syslog(LOG_DEBUG,"ERROR: accept (inet) failed.");
			return -1;
		}
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
        syslog(LOG_DEBUG,"network_close_connection: closed connection number "
                "%i, socket %i",i,connection->mysocket);
	monitor_list_delete_by_socket(connection->mysocket);
	connection_list_remove(connection->mysocket);
}

/**
 * close all network connections.
 */
void network_close_connections()
{
	struct connection_struct *connection = connection_list_begin();
	struct connection_struct *connection2 = connection;
	while (connection != NULL) {
		close(connection->mysocket);
		monitor_list_delete_by_socket(connection->mysocket);
		connection2=connection->next;
		connection_list_remove(connection->mysocket);
        	syslog(LOG_DEBUG,
			"network_close_connections: closed connection on "
                	"socket %i",connection->mysocket);
		connection=connection2;
	}

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
	enum header_states hstate;
     	struct connection_struct *connection =
		connection_list_identify(i);
        if (connection->connection_function == SOCK_TYPE_DATA ||
	    connection->connection_function == SOCK_TYPE_DB_QUERY) {
		switch(connection->data_state) {
		case CONN_READ_HEADER: ;
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
			hstate = 
				protocol_check_header(connection->header);
			if (hstate == HEADER_CHECK_FAIL ||
				hstate == HEADER_CHECK_NULL)
					network_close_connection(i);
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
			hstate =
				protocol_check_header(connection->header);
			if (hstate == HEADER_CHECK_FAIL ||
				hstate == HEADER_CHECK_NULL)
					network_close_connection(i);
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
				talloc_array(connection->header,
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
					protocol_decrypt(connection->header,
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
				cache_add(connection->body, connection->blocklen,c);
			}

			if (connection->connection_function == SOCK_TYPE_DB_QUERY) {
				if (c->dbg == 1) D_(LOG_DEBUG,
					"Adding to queries: %s | len = %i",
					connection->body,
					connection->blocklen);
				query_add(connection->body, connection->blocklen,i,0);
			}

			TALLOC_FREE(connection->header);
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
					protocol_decrypt(connection->header,
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
				cache_add(connection->body,
					connection->blocklen,c);
			}
			if (connection->connection_function == SOCK_TYPE_DB_QUERY) {
				DEBUG(1) syslog(LOG_DEBUG,
					"Adding to queries %s | len = %i",
					connection->body,
					connection->blocklen);
				query_add(connection->body,
					connection->blocklen,i,0);
			}
			TALLOC_FREE(connection->header);
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

	if (bind(sock_fd,
		(struct sockaddr *)&my_addr,
		sizeof(my_addr)) == -1 ) {
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
 * create a unix domain socket
 *
 */
int network_create_unix_socket(char *name)
{
        /* Create a streaming UNIX Domain Socket */
        int s,len;
        struct sockaddr_un local;
        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
                syslog(LOG_DAEMON, "ERROR: unix socket creation failed.");
                return 1;
        }

        local.sun_family= AF_UNIX;
        strcpy(local.sun_path,name);
        unlink(local.sun_path);
        len=strlen(local.sun_path) + sizeof(local.sun_family);
        bind(s,(struct sockaddr *) &local, len);
        listen(s,50);
        return s;
}

/**
 * sends *data over the network to socket sock, length
 * containing the length of the data package.
 * Create a header, specify encryption and encrypt if
 * needed.
 */
void network_send_data( char *data, int sock, int length)
{
	fd_set write_fd_set, read_fd_set;
	/* encryption changes the length of the data package */
	ssize_t len = (size_t) length;
	ssize_t check = 0;
	int send_len = 0;
	char *senddata = data;
	struct connection_struct *conn = NULL;
	config_t *conf = configuration_get_conf();
	FD_ZERO(&write_fd_set );
	char *headerstr=talloc_asprintf(NULL,"000000");
	char *crypted = NULL;
	char *header = NULL;

	/* Make sure we can write to the socket */
	connection_list_recreate_fs_sets(
			&read_fd_set,
			&write_fd_set);
	int h = connection_list_max() + 1;
	select( h,
		NULL, &write_fd_set, NULL,NULL);
	if ( FD_ISSET(sock, &write_fd_set) ) {
		/* get the connection struct according to the socket */
		conn = connection_list_identify(sock);
		/* enable encryption if required, mark the header byte */
		/* and encrypt the content */
		if (conn->encrypted==1) {
                	headerstr[2]='E';
			crypted = protocol_encrypt( NULL,
				(const char *) conf->key_clients,
				data,
				&len);
			senddata = crypted;
        	}
	
		header = network_create_header(NULL, headerstr, len);
		talloc_free(headerstr);
		
		/* send header */
		while ( send_len != strlen(header) ) {
			check = send(sock, header + send_len,
				strlen(header) - send_len,0);
			if (check == -1) {
				// send returns error, looks like we lost
				// the client
				talloc_free(crypted);
				return;
			}
			send_len = send_len + check;
		}
		/* send data */
 		send_len = 0;
		
		while ( send_len != len) {
			check = send(sock, senddata + send_len,
				len - send_len,0);
			if (check == -1) {
				// send returns error, looks like we lost
				// the client
				talloc_free(crypted);
				return;
			}
			send_len = send_len + check;
		}

		talloc_free(crypted);
	}
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
	struct sockaddr_un remote_unix;
	struct sockaddr_in remote_inet;

	fd_set read_fd_set;
	fd_set write_fd_set;

	FD_ZERO(&read_fd_set );
	FD_ZERO(&write_fd_set );
	if (c->unix_socket == 0) 
		c->vfs_socket = network_create_socket( c->port );
	else
		c->vfs_socket = network_create_unix_socket("/var/tmp/stadsocket");

	if (c->unix_socket_clients == 0)
		c->query_socket = network_create_socket( c->query_port );
	else
		c->query_socket = network_create_unix_socket("/var/tmp/stadsocket_client");

	connection_list_add( c->vfs_socket, SOCK_TYPE_DATA );
	connection_list_add( c->query_socket, SOCK_TYPE_DB_QUERY);
	for (;;) {
		z = 0;
		connection_list_recreate_fs_sets(
			&read_fd_set,
			&write_fd_set);
		int h = connection_list_max() + 1;
		z = select( h,
			&read_fd_set, NULL, NULL,NULL);
		if (z < 0) {
			syslog(LOG_DEBUG,"ERROR: select error.");
			exit(1);
		}
		for( i = 0; i < h; ++i) {
			if (FD_ISSET(i,&read_fd_set)) {
				int sr;
				if ( i == c->vfs_socket)
					sr = network_accept_connection(
						c,
						&remote_inet,
						&remote_unix,
						SOCK_TYPE_DATA); 
				else if ( i == c->query_socket)
					sr = network_accept_connection(
						c,
						&remote_inet,
						&remote_unix,
						SOCK_TYPE_DB_QUERY);
				else {
					network_handle_data(i,c);
					monitor_list_process(i);
				}
			}
		}
	}
}
