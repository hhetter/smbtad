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



typedef struct configuration_data {
	/* Number of the port to use */
	int port;
	/* the maintenance timer strings */
        /* user given string argument, as to when the maintenance       */
        /* routine should start.                                        */
        char maintenance_timer_str[200];
        char maint_run_time_str[200];
        /* time in seconds to restart the maintenance timer             */
        time_t maintenance_seconds;
        /* time in seconds for the maintenance run, everything older    */
        /* than this time will be deleted from the database.            */
        time_t maint_run_time;
	/* debug level */
	int dbg;
	/* configuration file */
	char *config_file;
	/* daemon mode */
	int daemon;
	/* 1 if a unix domain socket is used for the connection to the	*/
	/* module.							*/
	int unix_socket;
	/* run time configuration */
        /* integers used to separate the time for the maintenance run   */
        int mdays,mminutes,mseconds,mhours;
	int vfs_socket;
	char *current_query_result;
	int current_query_result_len;
	int result_socket;
	/* Database connection */
	/* hostname to connect to*/
	char *dbhost;
	/* name of the database */
	char *dbname;
	/* user of the db */
	char *dbuser;
	/* password of the user */
	char *dbpassword;
	/* AES Keyfile */
	char *keyfile;
	/* AES Key */
	unsigned char key[20];
	/* precision is used by the cache, it's value specifies the */
	/* life time of the cache in seconds, during this time it */
	/* sums up similar VFS R/W entries. Default is 5.	*/
	int precision;
	/* if use_db = 0, no db handling will be done. */
	int use_db;
} config_t;
pthread_mutex_t *configuration_get_lock();
int configuration_check_configuration( config_t *c );
int configuration_parse_cmdline( config_t *c, int argc, char *argv[] );
config_t *configuration_get_conf();
