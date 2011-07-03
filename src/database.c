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
 * check the database version
 */
void database_check_db_version( struct configuration_data *conf )
{
	dbi_result result;
	result = dbi_conn_query( conf->DBIconn,
		"SELECT smbtad_database_version FROM status "
		"WHERE smbtad_control_entry = 'SMBTAD';");
	if (result == NULL) {
		printf("ERROR: Error getting the database"
			" version.\n");
		printf("Probably there is no database existing yet,\n");
		printf("or your existing database is not compatible\n");
		printf("with this version of smbtad. Please either\n");
		printf("upgrade your database by using 'smbtaquery -C'\n");
		printf("or create a new database with 'smbta -T'.\n");
		printf("\n");
		printf("Exiting.\n");
		exit(1);
	} else {
		dbi_result_first_row(result);
		if (strcmp(
			dbi_result_get_string_idx(result,1),
			STAD2_VERSION) != 0) {
			printf("Your existing database is not compatible\n");
			printf("with this version of smbtad. Please either\n");
			printf("upgrade your database by using 'smbtaquery -C'\n");
			printf("or create a new database with 'smbta -T'.\n");
			printf("\n");
			printf("Exiting.\n");
			exit(1);
		}
	}
	dbi_result_free(result);
}

/**
 * fill the status table with data
 */
void database_make_conf_table( struct configuration_data *conf )
{
	dbi_result result;
	result = dbi_conn_queryf( conf->DBIconn,
		"UPDATE status SET "
		"smbtad_version = '%s',"
		"smbtad_client_port = %i,"
		"smbtad_unix_socket_clients = %i,"
		"smbtad_dbname = '%s',"
		"smbtad_dbhost = '%s',"
		"smbtad_dbuser = '%s',"
		"smbtad_dbdriver = '%s',"
		"smbtad_maintenance_timer_str = '%s',"
		"smbtad_maintenance_run_time = %i,"
		"smbtad_debug_level = %i,"
		"smbtad_precision = %i,"
		"smbtad_daemon = %i,"
		"smbtad_use_db = %i,"
		"smbtad_config_file = '%s'"
		" WHERE smbtad_control_entry = 'SMBTAD';", 
		STAD2_VERSION,
		conf->query_port,
		conf->unix_socket_clients,
		conf->dbname,
		conf->dbhost,
		conf->dbuser,
		conf->dbdriver,
		conf->maintenance_timer_str,
		conf->maint_run_time,
		conf->dbg,
		conf->precision,
		conf->daemon,
		conf->use_db,
		conf->config_file);
	if (result == NULL) {
		// we're not daemonized at this point, use printf
		printf("\nERROR: could not update the status table!\n");
		printf("Exiting.\n");
		exit(1);
	}
}	

/**
 * Create the initial tables of the database, to be called at first
 * startup.
 * return 0 on success, 1 if fail
 *
 * ATTENTION: This function is called on the very first setup of
 * SMBTA. When making changes here, please take care for the
 * smbtaquery -C function in smbtatools, which has to do the
 * same changes on update
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
	/**
	 * create a table with version information
	 * and configuration status
	 */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE status ("
		"smbtad_control_entry varchar,"
		"smbtad_version varchar,"
		"smbtad_database_version varchar,"
		"smbtad_client_port integer,"
		"smbtad_unix_socket_clients integer,"
		"smbtad_dbname varchar,"
		"smbtad_dbhost varchar,"
		"smbtad_dbuser varchar,"
		"smbtad_dbdriver varchar,"
		"smbtad_maintenance_timer_str varchar,"
		"smbtad_maintenance_run_time integer,"
		"smbtad_debug_level integer,"
		"smbtad_precision integer,"
		"smbtad_daemon integer,"
		"smbtad_use_db integer,"
		"smbtad_config_file varchar);");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the status table!");
		return 1;
	}
	dbi_result_free(result);
	/**
	 * fill in initial data
	 */
	result = dbi_conn_query( conf->DBIconn,
		"INSERT INTO status ("
		"smbtad_control_entry,"
		"smbtad_version,"
		"smbtad_database_version)"
		"VALUES ("
		"'SMBTAD','"
		STAD2_VERSION
		"','"
		STAD2_VERSION
		"');");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"initial values for status!");
		return 1;
	}
	dbi_result_free(result);
	/**
	 * create a table for version information on
	 * modules
	 */
	result = dbi_conn_query( conf->DBIconn,
		"CREATE TABLE modules ("
		"module_subrelease_number integer,"
		"module_common_blocks_overflow integer,"
		"module_ip_address varchar);");
	if (result == NULL) {
		syslog(LOG_DEBUG,"create tables: could not create"
			"the modules table!");
		return 1;
	}
	dbi_result_free(result);

	return 0;
}


