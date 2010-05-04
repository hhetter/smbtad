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
 * Create a database and setup the required tables
 */
sqlite3 *database_create( char *filename )
{

	sqlite3 *db;
	int rc;
	char *zErrormsg;
	rc=sqlite3_open( filename,&db);
	if ( rc ) {
		syslog(LOG_DAEMON,
                        "plugin-sqlite3 : ERROR: Can't open Database :"
                        " %s, exiting.\n",
                        sqlite3_errmsg(db));
                return NULL;
        }

	/* write/pwrite */
	rc = sqlite3_exec( db, \
		"CREATE TABLE write ("
		 CREATE_COMMONS
		"filename varchar, length integer )",NULL,0,&zErrormsg);
	/* read/pread */
	rc = sqlite3_exec( db, \
		"CREATE TABLE read ("
		 CREATE_COMMONS
		"filename varchar, length integer )",NULL,0,&zErrormsg);
	/* mkdir */
	rc = sqlite3_exec( db, \
		"CREATE TABLE mkdir ("
		 CREATE_COMMONS
		"path varchar, mode varchar, result integer )",NULL,0,&zErrormsg);
	/* rmdir */
	rc = sqlite3_exec( db, \
		"CREATE TABLE rmdir ("
		 CREATE_COMMONS
		"path varchar, mode varchar, result integer )",NULL,0,&zErrormsg);
	/* rename */
	rc = sqlite3_exec( db, \
		"CREATE TABLE rename ("
		CREATE_COMMONS
		"source varchar, destination varchar, result integer)",NULL,0,&zErrormsg);
	/* chdir */
	rc = sqlite3_exec( db, \
		"CREATE TABLE chdir ("
		 CREATE_COMMONS
		"path varchar, result integer)", NULL,0, &zErrormsg);
	/* open */
	rc = sqlite3_exec( db, \
		"CREATE TABLE open ("
		 CREATE_COMMONS
		"filename varchar, mode varchar, result integer)",NULL,0,&zErrormsg);
	/* close */
	rc = sqlite3_exec( db, \
		"CREATE TABLE close ("
		CREATE_COMMONS
		"filename varchar, result integer)",NULL,0,&zErrormsg);

	return db;
}


