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

/* Initialize default values of the configuration.			*/
void configuration_define_defaults( config_t *c )
{
	c->port = 3390;
	strcpy( c->maint_timer, "01:00:00" );
	strcpy( c->maint_timer_conf, "01,00:00:00" );
	c->daemon = 1;
	c->config_file = NULL;
	c->debug_level = 0;
	_DBG = 0;
}


void configuration_status( config_t *c )
{
	syslog(LOG_DEBUG,"**** stad2 configuration table ***");
	switch(c->daemon) {
	case 1:
		syslog(LOG_DEBUG,"Running as daemon		: Yes");
		break;
	case 2: 
		syslog(LOG_DEBUG,"Running as daemon		: No");
		break;
	}
	if (c->config_file != NULL) {
		syslog(LOG_DEBUG,"Using configuration file	: %s",
			c->config_file);
	}
	syslog(LOG_DEBUG,"Running on port		: %i",c->port);
}



int configuration_parse_cmdline( config_t *c, int argc, char *argv[] )
{
	int i;
	int digit_optind;

	configuration_define_defaults( c );


	if ( argc == 1 ) {
		printf("ERROR: not enough arguments.\n\n");
		help_show_help();
		return -1;
	}

	while ( 1 ) {
		int option_index = 0;

		static struct option long_options[] = {\
			{ "inet-port", 1, NULL, 'i' },
			{ "interactive", 0, NULL, 'o' },
			{ "debug-level",1, NULL, 'd' },
			{ 0,0,0,0 }
		};

		i = getopt_long( argc, argv,
			"d:i:o", long_options, &option_index );

		if ( i == -1 ) break;

		switch (i) {
			case 'i':
				c->port = atoi( optarg );
				break;
			case 'o':
				c->daemon = 0;
				break;
			case 'd':
				c->debug_level = atoi( optarg );
				_DBG = c->debug_level;
				break;
			default	:
				printf("ERROR: unkown option.\n\n");
				help_show_help();
				return -1;
		}
	}

return 0;
}

int configuration_check_configuration( config_t *c )
{
	if ( c->debug_level <0 || c->debug_level>10 ) {
		printf("ERROR: debug level has to be between 0 and 10.\n");
		return -1;
	}
}
