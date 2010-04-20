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
	c->dbname = strdup( "/var/lib/staddb");
	c->dbhandle = NULL;
	c->keyfile =NULL;
	c->query_port = 3391;
	_DBG = 0;
}

int configuration_load_key_from_file( config_t *c)
{
        FILE *keyfile;
        char *key = malloc(sizeof(char) * 17);
        int l;
        keyfile = fopen(c->keyfile, "r");
        if (keyfile == NULL) {
                return -1;
        }
        l = fscanf(keyfile, "%s", key);
        if (strlen(key) != 16) {
                printf("ERROR: Key file in wrong format\n");
                fclose(keyfile);
                exit(1);
        }
        strcpy( (char *) c->key, key);
	syslog(LOG_DEBUG,"KEY LOADEDi\n");
	return 0;
}



int configuration_load_config_file( config_t *c)
{
	dictionary *Mydict;
	Mydict=iniparser_load( c->config_file);
	char *cc;

	if ( Mydict == NULL ) return -1;

	cc = iniparser_getstring( Mydict, "network:port_number",NULL);
	if (cc != NULL) c->port = atoi(cc);

	cc = iniparser_getstring( Mydict, "network:query_port",NULL);
	if (cc != NULL) c->port = atoi(cc);

        cc = iniparser_getstring(Mydict,"database:sqlite_filename",NULL);
        if ( cc != NULL ) {
                free(c->dbname);
                c->dbname= strdup( cc);
        }

	cc = iniparser_getstring(Mydict,"general:debug_level",NULL);
	if (cc != NULL) {
		c->debug_level = atoi(cc);
	}
	cc = iniparser_getstring(Mydict,"general:keyfile",NULL);
	if (cc != NULL) {
		configuration_load_key_from_file( c);
	}
	return 0;
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
			{ "query-port",1, NULL, 'q' },
			{ "interactive", 0, NULL, 'o' },
			{ "debug-level",1, NULL, 'd' },
			{ "config-file",1,NULL,'c'},
			{ "key-file",1,NULL,'k'},
			{ 0,0,0,0 }
		};

		i = getopt_long( argc, argv,
			"d:i:oc:k:q:", long_options, &option_index );

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
			case 'q':
				c->query_port = atoi( optarg);
				break;
			case 'c':
				c->config_file = strdup( optarg );
				configuration_load_config_file(c);
				break;
			case 'k':
				c->keyfile = strdup( optarg);
				configuration_load_key_from_file(c);
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
	return 0;
}
