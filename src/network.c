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

int network_create_socket( int port, config_t *c )
{
	int sock_fd, new_fd;
	struct sockaddr_in6 my_addr;
	struct sockaddr_in6 their_addr;
	socklen_t sin_size;
	struct sigaction sa;

	if ( (sock_fd = socket(AF_INET6, SOCK_STREAM,0)) == -1 ) {
		syslog( LOG_DAEMON, "ERROR: socket creation failed." );
		exit(1);
	}

	int y;
	if ( setsockopt( sock_fd, SOL_SOCKET, SO_REUSEADDR, &y,
		sizeof( int )) == -1 ) {
		syslog( LOG_DAEMON, "ERROR: setsockopt failed." );
		exit(1);
	}

	my_addr.sin6_port = htons( port );
	my_addr.sin6_addr = in6addr_any;

	if (bind(sock_fd,(struct sockaddr *)&my_addr,sizeof(my_addr)) == -1 ) {
		syslog( LOG_DAEMON, "ERROR: bind failed." );
		exit(1);
	}

	if ( listen( sock_fd, 50 ) == -1 ) {
		syslog( LOG_DAEMON, "ERROR: listen failed." );
		exit(1);
	}

	return sock_fd;
}


void network_handle_connections( config_t *c )
{
	l_(0,"Hello World.",c);
	
}
