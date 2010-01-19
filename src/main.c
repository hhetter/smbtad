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
	config_t conf;

	/* parse command line 						*/
	if ( configuration_parse_cmdline( &conf, argc, argv ) <0 ) exit;

	/* become a daemon, depending on configuration 			*/
	daemon_daemonize( &conf );

	/* enter the main network function.				*/
	network_handle_connections( &conf );

	exit(0);
}