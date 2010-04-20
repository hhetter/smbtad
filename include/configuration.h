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
	/* portnumber for clients who query */
	int query_port;
	/* the maintenance timer strings */
	char maint_timer[200];
	char maint_timer_conf[200];
	/* debug level */
	int debug_level;
	/* configuration file */
	char *config_file;
	/* daemon mode */
	int daemon;

	/* run time configuration */
	int vfs_socket;
	int query_socket;
	/* file to use for the database */
	char *dbname;
	/* the db handle */
	sqlite3 *dbhandle;
	/* AES Keyfile */
	char *keyfile;
	/* AES Key */
	unsigned char key[20];

} config_t;

int configuration_check_configuration( config_t *c );
int configuration_parse_cmdline( config_t *c, int argc, char *argv[] );

