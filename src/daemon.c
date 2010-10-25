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

void daemon_signal_term( int signum )
{
	syslog(LOG_DEBUG,"smbtad is being terminated,"
		"closing all connections.");
	network_close_connections();
	exit(0);
}



void daemon_daemonize( config_t *c )
{

	if ( c->daemon == 0 ) return;

	int fhandle;
	/* check if we are already daemonized. */
	if ( getppid() == 1 ) return;

	fhandle = fork();

	if ( fhandle < 0 ) {
		printf("ERROR: unable to fork!\n");
		exit(1);
	}
	if (fhandle > 0 ) {
		/* let the parent exit */
		exit(0);
	}

	/* continue in child. */
	setsid(); /* get a new process group */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals 	*/
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	/* catch term */
	signal(SIGTERM,daemon_signal_term);
}
