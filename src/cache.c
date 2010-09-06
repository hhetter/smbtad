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

#define _BSD_SOURCE
#include "../include/includes.h"
#include <unistd.h>




struct cache_entry *cache_start = NULL;
struct cache_entry *cache_end = NULL;
TALLOC_CTX *cache_pool = NULL;
pthread_mutex_t cache_mutex;
void cache_update_monitor( struct cache_entry *entry);

/*
 * init the cache system */
void cache_init( ) {
        pthread_mutex_init(&cache_mutex, NULL);
}

/*
 * adds an entry to the cache
 * returns -1 in case of an error
 */
int cache_add( char *data, int len ) {

	
        struct cache_entry *entry;	
	pthread_mutex_lock(&cache_mutex);
	if (cache_start == NULL) {
		cache_pool = talloc_pool(NULL, 8*1024*1024);
		cache_start = talloc(cache_pool, struct cache_entry);
		entry = cache_start;
		entry->data = talloc_steal( cache_start, data);
		entry->length = len;
		cache_start = entry;
		entry->next = NULL;
		cache_end = entry;
		cache_update_monitor( entry); 
		pthread_mutex_unlock(&cache_mutex);
		return 0;
	}
	entry = talloc(cache_start, struct cache_entry);
	cache_end->next = entry;
	entry->next = NULL;
	entry->data = talloc_steal( cache_start, data);
	entry->length = len;
	cache_end = entry;
	cache_update_monitor(entry);
	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

void cache_update_monitor(struct cache_entry *entry)
{
        TALLOC_CTX *data = talloc_pool( NULL, 2048);
        char *montimestamp = NULL;

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
        char *mondata = NULL;
        /* first check how many common data blocks will come */
        str = protocol_get_single_data_block( data, &go_through);
        int common_blocks_num = atoi(str);
        /**
         * don't run a newer smbtad with an older VFS module
         */
        if (common_blocks_num < 6) {
                syslog(LOG_DEBUG, "FATAL: Protocol error!"
                        " Too less common data blocks! (%i), ignoring data!",
                        common_blocks_num);
                TALLOC_FREE(data);
                return;
        }

        /* vfs_operation_identifier */
        str = protocol_get_single_data_block( data, &go_through);
        int op_id = atoi(str);
        /* username */
        username = protocol_get_single_data_block_quoted( data, &go_through );
        /* user's SID */
        usersid = protocol_get_single_data_block_quoted( data, &go_through );
        /* share */
        share = protocol_get_single_data_block_quoted( data, &go_through );
        /* domain */
        domain = protocol_get_single_data_block_quoted( data, &go_through );
        /* timestamp */
        timestamp = protocol_get_single_data_block_quoted( data, &go_through );
        /* now receive the VFS function depending arguments */
        switch( op_id) {
        case vfs_id_read:
        case vfs_id_pread: ;
                filename = protocol_get_single_data_block_quoted( data, &go_through );
                len = protocol_get_single_data_block( data, &go_through);
                mondata = len;
                break;
        case vfs_id_write:
        case vfs_id_pwrite: ;
                filename= protocol_get_single_data_block_quoted( data,&go_through );
                len = protocol_get_single_data_block( data,&go_through);
                mondata = len;
                break;
        case vfs_id_mkdir: ;
                path = protocol_get_single_data_block_quoted( data,&go_through);
                mode = protocol_get_single_data_block_quoted( data,&go_through);
                result=protocol_get_single_data_block( data,&go_through);
                break;
        case vfs_id_chdir: ;
                path = protocol_get_single_data_block_quoted( data,&go_through);
                result = protocol_get_single_data_block( data,&go_through);
                break;
        case vfs_id_open: ;
                filename = protocol_get_single_data_block_quoted(data,&go_through);
                mode = protocol_get_single_data_block_quoted(data,&go_through);
                result = protocol_get_single_data_block(data,&go_through);
                break;
        case vfs_id_close: ;
                filename = protocol_get_single_data_block_quoted(data,&go_through);
                result = protocol_get_single_data_block(data,&go_through);
                break;
        case vfs_id_rename: ;
                source = protocol_get_single_data_block_quoted(data,&go_through);
                destination = protocol_get_single_data_block_quoted(data,&go_through);
                result = protocol_get_single_data_block(data,&go_through);
                break;

        }
        if (filename == NULL) filename = talloc_asprintf(data,"\"*\"");
	monitor_list_update( op_id, username, usersid, share,filename,domain, mondata, timestamp);
        TALLOC_FREE(data);
}


char *cache_make_database_string( TALLOC_CTX *ctx,struct cache_entry *entry)
{
	TALLOC_CTX *data = talloc_pool( NULL, 2048);
	char *montimestamp = NULL;
	char *retstr = NULL;

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
	char *mondata = NULL;
        /* first check how many common data blocks will come */
	str = protocol_get_single_data_block( data, &go_through);
	int common_blocks_num = atoi(str);
        /**
         * don't run a newer smbtad with an older VFS module
         */
        if (common_blocks_num < 6) {
                syslog(LOG_DEBUG, "FATAL: Protocol error!"
			" Too less common data blocks! (%i), ignoring data!",
			common_blocks_num);
                TALLOC_FREE(data);
		return NULL;
        }

        /* vfs_operation_identifier */
	str = protocol_get_single_data_block( data, &go_through);
	int op_id = atoi(str);
	switch(op_id) {
	case vfs_id_read:
	case vfs_id_pread:
		vfs_id = talloc_strdup( data, "read");
		break;
	case vfs_id_write:
	case vfs_id_pwrite:
		vfs_id = talloc_strdup(data, "write");
		break;
	case vfs_id_mkdir:
		vfs_id = talloc_strdup(data, "mkdir");
		break;
	case vfs_id_chdir:
		vfs_id = talloc_strdup(data, "chdir");
		break;
	case vfs_id_rename:
		vfs_id = talloc_strdup(data, "rename");
		break;
	case vfs_id_rmdir:
		vfs_id = talloc_strdup(data, "rmdir");
		break;
	case vfs_id_open:
		vfs_id = talloc_strdup(data, "open");
		break;
	case vfs_id_close:
		vfs_id = talloc_strdup(data, "close");
		break;
	}

	/* in case we received a vfs_id that we don't support, return NULL */
	if (vfs_id == NULL) {
		syslog(LOG_DEBUG,"Unsupported VFS function!");
		TALLOC_FREE(data);
		return NULL;
	}
        /* username */
        username = protocol_get_single_data_block_quoted( data, &go_through );
        /* user's SID */
        usersid = protocol_get_single_data_block_quoted( data, &go_through );
        /* share */
        share = protocol_get_single_data_block_quoted( data, &go_through );
        /* domain */
        domain = protocol_get_single_data_block_quoted( data, &go_through );
        /* timestamp */
        timestamp = protocol_get_single_data_block_quoted( data, &go_through );


	/* now receive the VFS function depending arguments */
	switch( op_id) {
	case vfs_id_read:
	case vfs_id_pread: ;
		filename = protocol_get_single_data_block_quoted( data, &go_through );
		len = protocol_get_single_data_block( data, &go_through);
                if (len == 0) {
			retstr=NULL;
			break;
		}
		retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
			"username, usersid, share, domain, timestamp,"
			"filename, length) VALUES ("
			"%s,%s,%s,%s,%s,"
			"%s,%s);",
			vfs_id,username,usersid,share,domain,timestamp,
			filename,len);
		mondata = len;
		break;
	case vfs_id_write:
	case vfs_id_pwrite: ;
                filename= protocol_get_single_data_block_quoted( data,&go_through );
                len = protocol_get_single_data_block( data,&go_through);
                if (len == 0) {
			retstr=NULL;
			break;
                }
		retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, length) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        filename,len);
		mondata = len;
                break;
	case vfs_id_mkdir: ;
		path = protocol_get_single_data_block_quoted( data,&go_through);
		mode = protocol_get_single_data_block_quoted( data,&go_through);
		result=protocol_get_single_data_block( data,&go_through);
		retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
			"username, usersid, share, domain, timestamp,"
			"path, mode, result) VALUES ("
			"%s,%s,%s,%s,%s,"
			"%s,%s,%s);",
			vfs_id,username,usersid,share,domain,timestamp,
			path, mode, result);
		break;
	case vfs_id_chdir: ;
		path = protocol_get_single_data_block_quoted( data,&go_through);
		result = protocol_get_single_data_block( data,&go_through);
		retstr = talloc_asprintf( ctx, "INSERT INTO %s ("
			"username, usersid, share, domain, timestamp,"
			"path, result) VALUES ("
			"%s,%s,%s,%s,%s,"
			"%s,%s);",
			vfs_id,username,usersid,share,domain,timestamp,
			path,result);
		break;
	case vfs_id_open: ;
		filename = protocol_get_single_data_block_quoted(data,&go_through);
		mode = protocol_get_single_data_block_quoted(data,&go_through);
		result = protocol_get_single_data_block(data,&go_through);
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, mode, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        filename, mode, result);
		break;
	case vfs_id_close: ;
		filename = protocol_get_single_data_block_quoted(data,&go_through);
		result = protocol_get_single_data_block(data,&go_through);
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        filename,result);
		break;
	case vfs_id_rename: ;
		source = protocol_get_single_data_block_quoted(data,&go_through);
		destination = protocol_get_single_data_block_quoted(data,&go_through);
		result = protocol_get_single_data_block(data,&go_through);
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "source, destination, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s,%s);",
                        vfs_id,username,usersid,share,domain,timestamp,
                        source,destination,result);
		break;

	}

	if (filename == NULL) filename = talloc_asprintf(data,"\"*\"");

	DEBUG(1) syslog(LOG_DEBUG,
		"Calling monitor_list update with:"
		"username: %s, "
		"usersid : %s, "
		"share   : %s, "
		"filename: %s, "
		"domain  : %s, "
		"montimestamp: %s",
		username,
		usersid,
		share,
		filename,
		domain,
		montimestamp);

		

	/* free everything no longer needed */
	TALLOC_FREE(data);
	return retstr;
}

/* 
 * sqlite >= 3.7.0 allows WAL. We will use this feature.
 * Run as a thread and run any query that is waiting.
 */
void cache_query_thread(struct configuration_data *config)
{
	pthread_detach(pthread_self());
	sqlite3 *database = config->dbhandle;
	/* run a query and add the result to the sendlist */
	int count = 0;
	while (1 == 1) {
		usleep(5000);
                int res_len;
                int res_socket;
                int monitorid = 0;
                char *res = query_list_run_query(database,
			&res_len, &res_socket, &monitorid);
		if (res != NULL && monitorid ==0) sendlist_add(res,res_socket,res_len);
		if (res != NULL && monitorid !=0) monitor_list_set_init_result(res, monitorid);
	}
}


/*
 * cache_manager runs as a detached thread. Every half second,
 * it's checking if there's data available to store to the DB.
 * in case of yes, it walks through the list of data, enables
 * a new cache, and stores the data in the DB
 */
void cache_manager(struct configuration_data *config )
{

        pthread_detach(pthread_self());
	sqlite3 *database = config->dbhandle;
	/* backup the start and make it possible to
	 * allocate a new cache beginning
	 */
	TALLOC_CTX *database_str = talloc_pool(NULL, 2000);
	while (1 == 1) {
		sleep(5);
        	pthread_mutex_lock(&cache_mutex);
        	struct cache_entry *go_through = cache_start;
		struct cache_entry *backup = cache_pool;
       		cache_start = NULL;
        	cache_end = NULL;
        	pthread_mutex_unlock(&cache_mutex);
		/* store all existing entries into the database */
		sqlite3_exec(database, "BEGIN TRANSACTION;", 0, 0, 0);
		while (go_through != NULL) {
			char *a = cache_make_database_string(database_str, go_through );
			if (a!= NULL) sqlite3_exec(database, a,0,0,0);
			TALLOC_FREE(a);
			go_through = go_through->next;
		}
		if (backup != NULL) TALLOC_FREE(backup);
		sqlite3_exec(database, "COMMIT;", 0, 0, 0); 

		
	}
}
