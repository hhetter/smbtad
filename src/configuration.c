/* 
 * smbtad 
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
#include <sys/stat.h>

pthread_mutex_t config_mutex;

pthread_mutex_t *configuration_get_lock(void) {
	return &config_mutex;
}

int configuration_check_configuration( config_t *c );

static config_t *conf;


config_t *configuration_get_conf() {
	return conf;
}

/* Initialize default values of the configuration.
 * Also initialize the configuration mutex.
 */
void configuration_define_defaults( config_t *c )
{
	c->dbname = NULL;
	c->dbhost = NULL;
	c->dbuser = NULL;
	c->dbdriver = NULL;
	c->dbpassword = NULL;
	c->port = 3940;
	c->unix_socket_clients = 0;
	/* AES encryption key from SMBTAD to clients */
	c->keyfile_clients = NULL;
	c->query_port = 3941;

	strcpy( c->maintenance_timer_str, "01:00:00" );
	strcpy( c->maint_run_time_str, "01,00:00:00" );
	c->daemon = 1;
	c->config_file = NULL;
	c->dbg = 0; // debug level
	c->unix_socket = 0;
	/* AES encryption key from the VFS module to SMBTAD */
	c->keyfile =NULL;
	/** precision is a factor for the cache,
	 * which is summing up similar R/W VFS entries
	 * */
	c->precision = 5;
	c->current_query_result = NULL;
	/**
	 * if use_db == 0, no database handling will be done.
	 * this is useful if smbtad shall only be used for rrddriver or
	 * smbtamonitor.
	 * Be aware that the behaviour of some monitors might change,
	 * for example the TOTAL monitor simply begins with 0 instead
	 * of the result of an sql query to sum up the number of bytes
	 * that have been written or read.
	 * Also, a database will be created, but it will be left empty.
	 */
	c->use_db = 1;	
	pthread_mutex_init(&config_mutex,NULL);
}

int configuration_load_key_from_file( config_t *c)
{
        FILE *keyfile;
        char *key = malloc(sizeof(char) * 25);
        int l;
        keyfile = fopen(c->keyfile, "r");
        if (keyfile == NULL) {
                return -1;
        }
        l = fscanf(keyfile, "%16s", key);
	if ( l == EOF ) {
		printf("Error reading keyfile: %s\n",strerror(errno));
		exit(1);
	}
        if (strlen(key) != 16) {
                printf("ERROR: Key file in wrong format\n");
                fclose(keyfile);
                exit(1);
        }
        strncpy( (char *) c->key, key, 20);
	syslog(LOG_DEBUG,"loaded VFS module AES key.\n");
	return 0;
}

int configuration_load_client_key_from_file( config_t *c)
{
        FILE *keyfile;
        char *key = malloc(sizeof(char) * 25);
        int l;
        keyfile = fopen(c->keyfile_clients, "r");
        if (keyfile == NULL) {
                return -1;
        }
        l = fscanf(keyfile, "%16s", key);
       if ( l == EOF ) {
               printf("Error reading keyfile: %s\n",strerror(errno));
               exit(1);
       }
        if (strlen(key) != 16) {
               printf("ERROR: Key file in wrong format\n");
                fclose(keyfile);
                exit(1);
        }
        strncpy( (char *) c->key_clients, key, 20);
        syslog(LOG_DEBUG,"loaded client AES key.\n");
        return 0;
}


int configuration_load_config_file( config_t *c)
{
	dictionary *Mydict;
	Mydict=iniparser_load( c->config_file);
	char *cc;

	cc = iniparser_getstring( Mydict, "network:query_port",NULL);
	if (cc != NULL) c->query_port = atoi(cc);
	

	if ( Mydict == NULL ) return -1;
	cc = iniparser_getstring (Mydict, "network:unix_domain_socket",NULL);
	if (cc!= NULL) c->unix_socket = 1;

	cc = iniparser_getstring( Mydict, "network:port_number",NULL);
	if (cc != NULL) c->port = atoi(cc);

        cc = iniparser_getstring(Mydict,"database:name",NULL);
        if ( cc != NULL ) {
                c->dbname= strdup( cc);
        }
	cc = iniparser_getstring(Mydict,"database:host",NULL);
	if ( cc != NULL) {
		c->dbhost=strdup(cc);
	}
	cc = iniparser_getstring(Mydict,"database:driver",NULL);
	if ( cc != NULL) {
		c->dbdriver=strdup(cc);
	}
	cc = iniparser_getstring(Mydict,"database:user",NULL);
	if ( cc != NULL) {
		c->dbuser = strdup(cc);
	}
	cc = iniparser_getstring(Mydict,"database:password",NULL);
	if ( cc != NULL) {
		c->dbpassword = strdup(cc);
	}
	cc = iniparser_getstring(Mydict,"general:precision",NULL);
	if (cc != NULL) c->precision = atoi(cc);

	cc = iniparser_getstring(Mydict,"general:use_db",NULL);
	if (cc != NULL) c->use_db = atoi(cc);

	cc = iniparser_getstring(Mydict,"general:debug_level",NULL);
	if (cc != NULL) {
		c->dbg = atoi(cc);
	}
	cc = iniparser_getstring(Mydict,"general:keyfile",NULL);
	if (cc != NULL) {
		c->keyfile = strdup(cc);
		configuration_load_key_from_file( c);
	}
	cc = iniparser_getstring(Mydict,"maintenance:interval",NULL);
	if (cc != NULL) {
		strncpy(c->maintenance_timer_str,cc,199);
	}
	cc = iniparser_getstring(Mydict,"maintenance:config",NULL);
	if (cc != NULL) {
		strncpy(c->maint_run_time_str,cc,199);
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
	conf = c;

	if ( argc == 1 ) {
		printf("ERROR: not enough arguments.\n\n");
		help_show_help();
		return -1;
	}

	while ( 1 ) {
		int option_index = 0;

		static struct option long_options[] = {\
			{ "inet-port", 1, NULL, 'i' },
			{ "dbdriver",1,NULL,'M'},
			{ "dbname",1,NULL,'N'},
			{ "dbuser",1,NULL,'S'},
			{ "dbhost",1,NULL,'H'},
			{ "dbpassword",1,NULL,'P'},
			{ "interactive", 0, NULL, 'o' },
			{ "debug-level",1, NULL, 'd' },
			{ "config-file",1,NULL,'c'},
			{ "unix-domain-socket-cl",0,NULL,'n'},
			{ "key-file",1,NULL,'k'},
			{ "maintenance-timer",1,NULL,'t'},
			{ "maintenance-timer-config",1,NULL,'m'},
			{ "unix-domain-socket",0,NULL,'u'},
			{ "precision",1,NULL,'p'},
			{ "use-db",1,NULL,'D'},
			{ "query-port",1,NULL,'q'},
			{ 0,0,0,0 }
		};

		i = getopt_long( argc, argv,
			"nd:i:oc:k:q:t:m:up:U:M:N:U:H:P:", long_options, &option_index );

		if ( i == -1 ) break;

		switch (i) {
			case 'q':
				c->query_port = atoi(optarg);
				break;
			case 'n':
				c->unix_socket_clients = 1;
				break;
			case 'S':
				c->dbuser = strdup(optarg);
				break;
			case 'M':
				c->dbdriver = strdup(optarg);
				break;
			case 'N':
				c->dbname = strdup(optarg);
				break;
			case 'H':
				c->dbhost = strdup(optarg);
				break;
			case 'P':
				c->dbpassword = strdup(optarg);
				break;
			case 'u':
				c->unix_socket = 1;
				break;
			case 'i':
				c->port = atoi( optarg );
				break;
			case 't':
				strncpy(c->maintenance_timer_str,optarg,199);
				break;
			case 'm':
				strncpy(c->maint_run_time_str,optarg,199);
				break;
			case 'o':
				c->daemon = 0;
				break;
			case 'd':
				c->dbg = atoi( optarg );
				break;
			case 'c':
				c->config_file = strdup( optarg );
				configuration_load_config_file(c);
				break;
			case 'k':
				c->keyfile = strdup( optarg);
				configuration_load_key_from_file(c);
				break;
			case 'b':
				c->dbname = strdup( optarg );
				break;
			case 'p':
				c->precision = atoi(optarg);
				break;
			case 'U':
				c->use_db = atoi(optarg);
				break;
			default	:
				printf("ERROR: unkown option.\n\n");
				help_show_help();
				return -1;
		}
	}
	configuration_check_configuration(c);

return 0;
}


int configuration_check_configuration( config_t *c )
{
	// fixme: add checks
	// create the maintenance timer
        /* initialize the maintenance timer */
        struct tm maintenance_timer;
        char *Result;
        Result=strptime( c->maintenance_timer_str,"%H:%M:%S",
                 &maintenance_timer);
        if (Result==NULL) {
                        printf("\n\nERROR: The maintenance timer (option -m)"
                                " is not in the right format.\n");
                        exit (0);
        }

        c->maintenance_seconds=maintenance_timer.tm_sec
                +60*60*maintenance_timer.tm_hour
                +60*maintenance_timer.tm_min;
        if ( c->maintenance_seconds<10 ) {
                printf("\n\nWARNING: the maintenance timer (option -m) "
                        "might be too short!\n");
                }

        /* calculate the maintenance timeout value */
        char *Helper=strstr(c->maint_run_time_str,",");
        char *Begin;
        if ( Helper==NULL ) {
                printf("\n\nERROR: The maintenance-timer-conf value is "
                        "in the wrong format! \n");
                exit (0);
        }
        *Helper='\0';
        c->mdays=atoi(c->maint_run_time_str); /* 86400 */
        Begin=Helper+1;
        Helper=strstr(Helper+1,":");

        if ( Helper==NULL) {
                printf("\n\nERROR: The maintenance-timer-conf value is in"
                        " the wrong format! \n");
                exit (0);
        }
        *Helper='\0';
        c->mhours=atoi(Begin);

        Begin=Helper+1;
        Helper=strstr(Begin,":");

        if ( Helper==NULL) {
                printf("\n\nERROR: The maintenance-timer-conf value is "
                        "in the wrong format! \n");
                exit (0);
        }
        *Helper='\0';
        c->mminutes=atoi(Begin);
        Begin=Helper+1;
        c->mseconds=atoi(Begin);

        c->maint_run_time=86400*c->mdays 
                + 3600*c->mhours 
                + 60*c->mminutes 
                + c->mseconds;
        if ( c->maint_run_time==0 ) {
                printf("\n\nERROR: The maintenance-timer-conf"
                        " value is zero.\n");
                exit (0);
        }
	// check precision cache value
	if (c->precision<0) {
		printf("\n\nERROR: the precision value for the cache cannot\n");
		printf("be negative!\n");
		exit(0);
	}
	// check use_db
	if (c->use_db<0 || c->use_db>1) {
		printf("\n\nERROR: use_db must be 0 or 1.\n");
		exit(0);
	}
	return 0;
}
