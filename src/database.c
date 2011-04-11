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
#define CREATE_COMMONS "vfs_id integer,username varchar,usersid varchar,share varchar,domain varchar,timestamp DATE,"


/*
 * Create a database connection and setup the required tables
 * returns 0 if fine, 1 on error
 */
int database_connect( configuration_data *conf )
{
	int rc;
	/**
	 * Initialize the DBI layer
	 */
	if ( conf->dbdriver == NULL) {
		syslog(LOG_DAEMON,
			"ERROR: drivername == NULL. Exiting.\n");
		return 1;
	}
	rc = dbi_initialize(NULL);
	if ( rc == -1 ) {
		syslog(LOG_DAEMON,
			"DBI: ERROR dbi_initialize. Exiting.\n");
		return 1;
	}
	conf->DBIconn = dbi_conn_new(conf->dbdriver);
	if (conn == NULL) {
		syslog(LOG_DAEMON,
			"DBI: ERROR dbi_conn_new, with driver %s.",
			drivername);
		return NULL;
	}

	dbi_conn_set_option(conf->DBIconn, "host", conf->dbhost);
	dbi_conn_set_option(conf->DBIconn, "username", conf->dbuser);
	dbi_conn_set_option(conf->DBIconn, "password", conf->dbpassword);
	dbi_conn_set_option(conf->DBIconn, "dbname", conf->dbname);
	dbi_conn_set_option(conf->DBIconn, "encoding", "UTF-8");
	if ( dbi_conn_connect(conf->DBIconn) < 0) {
		syslog(LOG_DAEMON,
			"DBI: could not connect, please check options.");
		return 1;
	}
	return 0;
}

/**
 * Create the initial tables of the database, to be called at first
 * startup.
 * return 0 on success, 1 if fail
 */
int database_create_tables( configuration_data *conf )
{
	dbi_result result;
	/* write/pwrite */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE write ("
		 CREATE_COMMONS
		"filename varchar, length integer )");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables : could not create
			the write/pwrite table!");
		return 1;
	}

	/* read/pread */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE read ("
		 CREATE_COMMONS
		"filename varchar, length integer )");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables : could not create"
			"the read/pread table!");
		return 1;
	}

	/* mkdir */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE mkdir ("
		 CREATE_COMMONS
		"path varchar, mode varchar, result integer )");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the mkdir table!");
		return 1;
	}

	/* rmdir */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE rmdir ("
		 CREATE_COMMONS
		"path varchar, mode varchar, result integer )");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the rmdir table!");
		return 1;
	}


	/* rename */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE rename ("
		CREATE_COMMONS
		"source varchar, destination varchar, result integer)");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the rename table!");
		return 1;
	}

	/* chdir */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE chdir ("
		 CREATE_COMMONS
		"path varchar, result integer)");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the chdir table!");
		return 1;
	}

	/* open */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE open ("
		 CREATE_COMMONS
		"filename varchar, mode varchar, result integer)");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the open table!");
		return 1;
	}

	/* close */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE close ("
		CREATE_COMMONS
		"filename varchar, result integer)");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the close table!");
		return 1;
	}

	return 0;
}


