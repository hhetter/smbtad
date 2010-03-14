/* 
 * stad 
 * capture transfer data from the vfs_smb_traffic_analyzer module, and store
 * the data via various plugins
 *
 * Copyright (C) Holger Hetterich, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Firee Software Foundation; either version 3 of the License, or
 * (at yur option) any later version.
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

struct cache_entry *cache_start = NULL;
struct cache_entry *cache_end = NULL;




/*
 * adds an entry to the cache
 * returns -1 in case of an error
 */
int cache_add( char *data, int len ) {

	
        struct cache_entry *entry = malloc(sizeof(struct cache_entry));	
	if (entry == NULL) return -1;
	pthread_mutex_lock(&cache_mutex);
	if (cache_start == NULL) {
		entry->data = data;
		entry->length = len;
		cache_start = entry;
		entry->next = NULL;
		cache_end = entry;
		pthread_mutex_unlock(&cache_mutex);
		return 0;
	}
	cache_end->next = entry;
	entry->next = NULL;
	entry->data = data;
	entry->length = len;
	cache_end = entry;
	pthread_mutex_unlock(&cache_mutex);
	return 0;
}


char *cache_make_database_string( struct cache_entry *entry)
{
	char *username = NULL;
	char *domain = NULL;
	char *share = NULL;
	char *timestamp = NULL;
	char *usersid = NULL;
	char *vfs_id = NULL;
	char *go_through = entry->data;
	char *str = NULL;
	char *filename = NULL;
	char *len = NULL;
	char *mode = NULL;
	char *result = NULL;
	char *path = NULL;
	char *source = NULL;
	char *destination = NULL;
	int i;
        /* first check how many common data blocks will come */
	str = protocol_get_single_data_block(&go_through);
	int common_blocks_num = atoi(str);
	free(str);
        /**
         * don't run a newer smbtad with an older VFS module
         */
        if (common_blocks_num < 6) {
                DEBUG(0) syslog(LOG_DEBUG, "FATAL: Protocol error! Too less common data blocks!\n");
                exit(1);
        }

        /* vfs_operation_identifier */
	str = protocol_get_single_data_block(&go_through);
	int op_id = atoi(str);
	free(str);
	switch(op_id) {
	case vfs_id_read:
	case vfs_id_pread:
		vfs_id = strdup("read");
		break;
	case vfs_id_write:
	case vfs_id_pwrite:
		vfs_id = strdup("write");
		break;
	case vfs_id_mkdir:
		vfs_id = strdup("mkdir");
		break;
	case vfs_id_chdir:
		vfs_id = strdup("chdir");
		break;
	case vfs_id_rename:
		vfs_id = strdup("rename");
		break;
	case vfs_id_rmdir:
		vfs_id = strdup("rmdir");
		break;
	case vfs_id_open:
		vfs_id = strdup("open");
		break;
	case vfs_id_close:
		vfs_id = strdup("close");
		break;
	}

	/* in case we received a vfs_id that we don't support, return NULL */
	if (vfs_id == NULL) {
		syslog(LOG_DEBUG,"UNSUPPORTED");
		return NULL;
	}
        /* username */
        username = protocol_get_single_data_block( &go_through );
        /* user's SID */
        usersid = protocol_get_single_data_block( &go_through );
        /* share */
        share = protocol_get_single_data_block( &go_through );
        /* domain */
        domain = protocol_get_single_data_block( &go_through );
        /* timestamp */
        timestamp = protocol_get_single_data_block( &go_through );

	/* now receive the VFS function depending arguments */
	switch( op_id) {
	case vfs_id_read:
	case vfs_id_pread: ;
		filename = protocol_get_single_data_block( &go_through );
		len = protocol_get_single_data_block( &go_through);
		i = asprintf(&str, "INSERT INTO %s ("
			"username, usersid, share, domain, timestamp,"
			"filename, length) VALUES ("
			"%s,%s,%s,%s,%s,"
			"%s,%s);",
			vfs_id,username,usersid,share,domain,timestamp,
			filename,len);
		free(filename);
		free(len);
		break;
	case vfs_id_write:
	case vfs_id_pwrite: ;
                filename = protocol_get_single_data_block( &go_through );
                len = protocol_get_single_data_block( &go_through);
                i = asprintf(&str, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, length) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        filename,len);
		free(filename);
		free(len);
                break;
	case vfs_id_mkdir: ;
		path = protocol_get_single_data_block( &go_through);
		mode = protocol_get_single_data_block( &go_through);
		result=protocol_get_single_data_block( &go_through);
		i = asprintf(&str, "INSERT INTO %s ("
			"username, usersid, share, domain, timestamp,"
			"path, mode, result) VALUES ("
			"%s,%s,%s,%s,%s,"
			"%s,%s,%s);",
			vfs_id,username,usersid,share,domain,timestamp,
			path, mode, result);
		free(path);
		free(mode);
		free(result);
		break;
	case vfs_id_chdir: ;
		path = protocol_get_single_data_block( &go_through);
		result = protocol_get_single_data_block( &go_through);
		i = asprintf(&str, "INSERT INTO %s ("
			"username, usersid, share, daomin, timestamp,"
			"path, result) VALUES ("
			"%s,%s,%s,%s,%s,"
			"%s,%s);",
			vfs_id,username,usersid,share,domain,timestamp,
			path,result);
		free(path);
		free(result);
		break;
	case vfs_id_open: ;
		filename = protocol_get_single_data_block(&go_through);
		mode = protocol_get_single_data_block(&go_through);
		result = protocol_get_single_data_block(&go_through);
                i = asprintf(&str, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, mode, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        filename, mode, result);
		free(filename);
		free(mode);
		free(result);
		break;
	case vfs_id_close: ;
		filename = protocol_get_single_data_block(&go_through);
		result = protocol_get_single_data_block(&go_through);
                i = asprintf(&str, "INSERT INTO %s ("
                        "username, usersid, share, daomin, timestamp,"
                        "filename, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        filename,result);
		free(filename);
		free(result);
		break;
	case vfs_id_rename: ;
		source = protocol_get_single_data_block(&go_through);
		destination = protocol_get_single_data_block(&go_through);
		result = protocol_get_single_data_block(&go_through);
                i = asprintf(&str, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "source, destination, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        source,destination,result);
		free(source);
		free(destination);
		free(result);
		break;

	}

	/* free everything no longer needed */
	free(username);
	free(usersid);
	free(share);
	free(domain);
	free(timestamp);
	free(vfs_id);
	return str;
}

	
/*
 * cache_manager runs as a detached thread. Every half second,
 * it's checking if there's data available to store to the DB.
 * in case of yes, it walks through the list of data, enables
 * a new cache, and stores the data in the DB
 */
void cache_manager( )
{
	struct timespec mywait;
        mywait.tv_sec=0;
        mywait.tv_nsec=999999999 / 2;

        pthread_detach(pthread_self());
	
	/* backup the start and make it possible to
	 * allocate a new cache beginning
	 */

	while (1 == 1) {
                /* wait half a second; we don't need to check the       */
                /* feed-list all the time.                              */
                nanosleep(&mywait,NULL);
        	pthread_mutex_lock(&cache_mutex);
        	struct cache_entry *begin = cache_start;
       		cache_start = NULL;
        	cache_end = NULL;
        	pthread_mutex_unlock(&cache_mutex);
		/* store all existing entries into the database */
		while (begin != NULL) {
			char *a = cache_make_database_string( begin );
			syslog(LOG_DEBUG,"STR: %s\n",a);

			free(a);
			struct cache_entry *dummy = begin;
			begin = begin->next;
			free(dummy->data);
			free(dummy);
		}
	}
}
