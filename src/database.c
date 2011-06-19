/*
 * stad database services
 *
 *
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
#define CREATE_COMMONS "vfs_id integer,username varchar,usersid varchar,share varchar,domain varchar,timestamp timestamp,"
int database_create_tables( struct configuration_data *conf );


/*
 * Create a database connection and setup the required tables
 * returns 0 if fine, 1 on error
 */
int database_connect( struct configuration_data *conf )
{
	int rc;
	const char *dberror;
	/**
	 * Initialize the DBI layer
	 */
	if ( conf->dbdriver == NULL) {
		printf("ERROR: drivername == NULL. Exiting.\n");
		return 1;
	}
	rc = dbi_initialize(NULL);
	if ( rc == -1 ) {
		printf("DBI: ERROR dbi_initialize. Exiting.\n");
		return 1;
	}
	conf->DBIconn = dbi_conn_new(conf->dbdriver);
	if (conf->DBIconn == NULL) {
		printf("DBI: ERROR dbi_conn_new, with driver %s.\n",
			conf->dbdriver);
		dbi_conn_error(conf->DBIconn, &dberror);
		printf("DBI: %s\n",dberror);
		return 1;
	}

	dbi_conn_set_option(conf->DBIconn, "host", conf->dbhost);
	dbi_conn_set_option(conf->DBIconn, "username", conf->dbuser);
	dbi_conn_set_option(conf->DBIconn, "password", conf->dbpassword);
	dbi_conn_set_option(conf->DBIconn, "dbname", conf->dbname);
	dbi_conn_set_option(conf->DBIconn, "encoding", "UTF-8");
	if ( dbi_conn_connect(conf->DBIconn) < 0) {
		printf("DBI: could not connect, please check options.\n");
		dbi_conn_error(conf->DBIconn,&dberror);
		printf("DBI: %s\n",dberror);
		return 1;
	}
	if (conf->dbsetup == 1) {
		if (database_create_tables(conf) == 0) {
			printf("\nSuccesfully created database tables.\n");
			exit(0);
		} else {
			printf("\nError creating database tables!\n");
			exit(0);
		}
	}
	return 0;
}

/**
 * Create the initial tables of the database, to be called at first
 * startup.
 * return 0 on success, 1 if fail
 */
int database_create_tables( struct configuration_data *conf )
{
	dbi_result result;

	/**
	 *  we formerly created single tables for every VFS function,
	 *  let this be in one table now
	 */

	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE data ("
		 CREATE_COMMONS
		"string1 varchar, length integer, result bigint, string2 varchar)");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables : could not create"
			"the data table!");
		return 1;
	}
	dbi_result_free(result);
	return 0;
}


