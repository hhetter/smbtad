/* 
 * stad2 
 * capture transfer data from the vfs_smb_traffic_analyzer module, and store
 * the data via various plugins
 *
 * Copyright (C) Holger Hetterich, 2008-2010
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


int main(int argc, char *argv[])
{

	syslog(LOG_DEBUG,"smbtad start up.");
	config_t conf;
	
	pthread_t thread;

	cache_init();
	query_init();
	sendlist_init();
	throughput_list_init();
	/* parse command line */
	if ( configuration_parse_cmdline( &conf, argc, argv ) <0 ) exit(1);
	/* global debug level */
	_DBG = conf.dbg;
	/* set the db */
	conf.dbhandle = database_create(conf.dbname);

	/* become a daemon, depending on configuration	*/
	daemon_daemonize( &conf );

       	pthread_create(&thread,NULL,(void *)&cache_manager,(void *) &conf);

	/* enter the main network function. */
	network_handle_connections( &conf );
	exit(0);
}
