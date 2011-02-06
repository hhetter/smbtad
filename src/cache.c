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
int cache_prepare_entry( TALLOC_CTX *ctx,struct cache_entry *entry);
/*
 * init the cache system */
void cache_init( ) {
        pthread_mutex_init(&cache_mutex, NULL);
	pthread_mutex_init(&database_access, NULL);
}

/*
 * adds an entry to the cache
 * returns -1 in case of an error
 */
int cache_add( char *data, int len ) {
	unsigned long int convlen = 0;
	struct cache_entry *entry = talloc(cache_start, struct cache_entry);
	entry->left = NULL;
	entry->right = NULL;
	entry->down = NULL;
	entry->other_ops = NULL;
	entry->data = talloc_steal( entry, data);
	struct cache_entry *gotr = cache_start;
	struct cache_entry *backup = cache_start;
	pthread_mutex_lock(&cache_mutex);
	cache_prepare_entry( cache_start, entry);
	if (cache_start == NULL) {
		/*
		 * first element, we just create it
		 */
		cache_start = entry;
		entry->left = NULL;
		entry->right = NULL;
		entry->down = NULL;
		entry->other_ops = NULL;
		cache_end = entry->other_ops;
		pthread_mutex_unlock(&cache_mutex);
		return 0;
	}
	switch(entry->op_id) {
	case vfs_id_read:
	case vfs_id_pread: 
     	case vfs_id_write:
      	case vfs_id_pwrite:
	while (gotr != NULL) {
		/*
		 * on a write or read operation, we insert-sort the
		 * entries, and simply add the number of bytes written
		 * to an already existing similar entry
		 */
		DEBUG(5) syslog(LOG_DEBUG,"cache: comparing %s vs %s",entry->filename,gotr->filename);
		if ( entry->filename[1] == gotr->filename[1] ) {
			while (gotr != NULL) {
				/*
			 	 * This could be a fitting entry, check it
			 	 */
				if ( entry->op_id == gotr->op_id && strncmp(entry->share, gotr->share, strlen(entry->share)) == 0
					&& strncmp(entry->filename, gotr->filename, strlen(entry->filename)) == 0
					&& strncmp(entry->username, gotr->username, strlen(entry->username)) == 0
					&& strncmp(entry->domain, gotr->domain, strlen(entry->domain)) == 0) {
					/*
				 	 * entry fits, add the value
				 	 */
					DEBUG(5) syslog(LOG_DEBUG,"cache : adding value.");
					gotr->len = gotr->len + entry->len;
					pthread_mutex_unlock(&cache_mutex);
					return 0;
				} else {
				/*
			 	 * That wasn't the right entry, it can be
			 	 * the next down from this entry.
			 	 */
				backup = gotr;
				gotr=gotr->down;
				DEBUG(5) syslog(LOG_DEBUG,"cache: going down on that entry...");
				}
			}
			/*
			 * This entry wasn't yet found, it's being added
			 * to this list
			 */
			DEBUG(5) syslog(LOG_DEBUG,"cache: entry created after moving down");
			backup->down = entry;
			pthread_mutex_unlock(&cache_mutex);
			return 0;
		} else if ( entry->filename[1] > gotr->filename[1] ) {
			/*
			 * continue search on the right
			 */
			DEBUG(5) syslog(LOG_DEBUG,"cache: right... as %s > %s",entry->filename,gotr->filename);
			backup = gotr;
			gotr= gotr->right;
			/*
			 * create that entry if we haven't found a fitting entry
			 *
			*/
			if (gotr == NULL) {
				backup->right = entry;
				DEBUG(5) syslog(LOG_DEBUG,"cache: entry created after moving right.");
				pthread_mutex_unlock(&cache_mutex);
				return 0;
			}
		} else if ( entry->filename[1] < gotr->filename[1]) {
			/*
			 * continue search on the left
			 *
			*/
			DEBUG(5) syslog(LOG_DEBUG,"cache: left... as %s < %s",entry->filename,gotr->filename);
			backup = gotr;
			gotr = gotr->left;
			/*
			 * create that entry if we haven't found a fitting entry
			 *
			*/
			if (gotr == NULL) {
				backup->left = entry;
				DEBUG(5) syslog(LOG_DEBUG,"cache: entry created after moving left.");
				pthread_mutex_unlock(&cache_mutex);
				return 0;
			}
		}
	}
	/*
	 * We should never come by here. Indicates an error.
	*/
	break;
	default: ;
		/*
		 * Just insert items as usual if they aren't READ/WRITE
		 * operations
		 */
		if (cache_end == NULL) {
			cache_start->other_ops = entry;
			cache_end=entry;
			pthread_mutex_unlock(&cache_mutex);
			return 0;
		} else {
			cache_end->other_ops = entry;
			cache_end=entry;
			pthread_mutex_unlock(&cache_mutex);
			return 0;
		}
		break;
	}	
		
}

void cache_update_monitor(struct cache_entry *entry)
{
        TALLOC_CTX *data = talloc_pool( NULL, 2048);

        char *username = NULL;
        char *domain = NULL;
        char *share = NULL;
        char *timestamp = NULL;
        char *usersid = NULL;
        char *go_through = entry->data;
        char *str = NULL;
        char *filename = NULL;
        unsigned long int len = 0;
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
                mondata = protocol_get_single_data_block( data, &go_through);
                len = atol(mondata);
                break;
        case vfs_id_write:
        case vfs_id_pwrite: ;
                filename= protocol_get_single_data_block_quoted( data,&go_through );
                mondata = protocol_get_single_data_block( data,&go_through);
		len = atol(mondata);
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


int cache_prepare_entry( TALLOC_CTX *ctx,struct cache_entry *entry)
{
	TALLOC_CTX *data = talloc_pool( ctx, 2048);
	char *go_through = entry->data;
	char *str = NULL;
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
		return -1;
        }

        /* vfs_operation_identifier */
	str = protocol_get_single_data_block( data, &go_through);
	entry->op_id = atoi(str);
	switch(entry->op_id) {
	case vfs_id_read:
	case vfs_id_pread:
		entry->vfs_id = talloc_strdup( data, "read");
		break;
	case vfs_id_write:
	case vfs_id_pwrite:
		entry->vfs_id = talloc_strdup(data, "write");
		break;
	case vfs_id_mkdir:
		entry->vfs_id = talloc_strdup(data, "mkdir");
		break;
	case vfs_id_chdir:
		entry->vfs_id = talloc_strdup(data, "chdir");
		break;
	case vfs_id_rename:
		entry->vfs_id = talloc_strdup(data, "rename");
		break;
	case vfs_id_rmdir:
		entry->vfs_id = talloc_strdup(data, "rmdir");
		break;
	case vfs_id_open:
		entry->vfs_id = talloc_strdup(data, "open");
		break;
	case vfs_id_close:
		entry->vfs_id = talloc_strdup(data, "close");
		break;
	}

	/* in case we received a vfs_id that we don't support, return NULL */
	if (entry->vfs_id == NULL) {
		syslog(LOG_DEBUG,"Unsupported VFS function!");
		TALLOC_FREE(data);
		return -2;
	}
        /* username */
        entry->username = protocol_get_single_data_block_quoted( data, &go_through );
        /* user's SID */
        entry->usersid = protocol_get_single_data_block_quoted( data, &go_through );
        /* share */
        entry->share = protocol_get_single_data_block_quoted( data, &go_through );
        /* domain */
        entry->domain = protocol_get_single_data_block_quoted( data, &go_through );
        /* timestamp */
        entry->timestamp = protocol_get_single_data_block_quoted( data, &go_through );


	/* now receive the VFS function depending arguments */
	switch( entry->op_id) {
	case vfs_id_read:
	case vfs_id_pread: ;
		entry->filename = protocol_get_single_data_block_quoted( data, &go_through );
		entry->len = atol(protocol_get_single_data_block( data, &go_through));
		break;
	case vfs_id_write:
	case vfs_id_pwrite: ;
                entry->filename= protocol_get_single_data_block_quoted( data,&go_through );
                entry->len = atol(protocol_get_single_data_block( data,&go_through));
                break;
	case vfs_id_mkdir: ;
		entry->path = protocol_get_single_data_block_quoted( data,&go_through);
		entry->mode = protocol_get_single_data_block_quoted( data,&go_through);
		entry->result=protocol_get_single_data_block( data,&go_through);
		break;
	case vfs_id_chdir: ;
		entry->path = protocol_get_single_data_block_quoted( data,&go_through);
		entry->result = protocol_get_single_data_block( data,&go_through);
		break;
	case vfs_id_open: ;
		entry->filename = protocol_get_single_data_block_quoted(data,&go_through);
		entry->mode = protocol_get_single_data_block_quoted(data,&go_through);
		entry->result = protocol_get_single_data_block(data,&go_through);
		break;
	case vfs_id_close: ;
		entry->filename = protocol_get_single_data_block_quoted(data,&go_through);
		entry->result = protocol_get_single_data_block(data,&go_through);
		break;
	case vfs_id_rename: ;
		entry->source = protocol_get_single_data_block_quoted(data,&go_through);
		entry->destination = protocol_get_single_data_block_quoted(data,&go_through);
		entry->result = protocol_get_single_data_block(data,&go_through);
		break;

	}
	if (entry->filename == NULL) entry->filename = talloc_asprintf(data,"\"*\"");
	return 0;
}


char *cache_create_database_string(TALLOC_CTX *ctx,struct cache_entry *entry)
{
	/*
	 * create a database string from the given metadata in a cache entry
	 */
	char *retstr=NULL;
	switch( entry->op_id ) {
        case vfs_id_rename: ;
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "source, destination, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s,%s);",
                        entry->vfs_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->source,entry->destination,entry->result);
		break;
        case vfs_id_close: ;
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s);",
                        entry->vfs_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->result);
                break;
        case vfs_id_open: ;
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, mode, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s,%s);",
                        entry->vfs_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->mode,entry->result);
                break;
        case vfs_id_chdir: ;
                retstr = talloc_asprintf( ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "path, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s);",
                        entry->vfs_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->path,entry->result);
                break;
        case vfs_id_mkdir: ;
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "path, mode, result) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%s,%s);",
                        entry->vfs_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->path, entry->mode, entry->result);
                break;
        case vfs_id_write:
        case vfs_id_pwrite: ;
                if (entry->len == 0) {
                        retstr=NULL;
                        break;
                }
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, length) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%lu);",
                        entry->vfs_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->len);
                entry->mondata = entry->len;
                break;	
        case vfs_id_read:
        case vfs_id_pread: ;
                if (entry->len == 0) {
                        retstr=NULL;
                        break;
                }
                retstr = talloc_asprintf(ctx, "INSERT INTO %s ("
                        "username, usersid, share, domain, timestamp,"
                        "filename, length) VALUES ("
                        "%s,%s,%s,%s,%s,"
                        "%s,%lu);",
                        entry->vfs_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->len);
                entry->mondata = entry->len;
                break;
	default: ;
	}	
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

void do_db( struct configuration_data *config, char *dbstring)
{
	sqlite3 *database = config->dbhandle;
	int rc = -1;
	syslog(LOG_DEBUG,"%s",dbstring); 
	char *erg = 0;
	while (rc != SQLITE_OK) {
		rc = sqlite3_exec(database, dbstring,0,0,&erg);
		if (rc != SQLITE_OK) {
			syslog(LOG_DEBUG,"sqlite error: %s",erg);
			sqlite3_free(erg);
			sleep(1); // sleep a second then try again
		}
	}
	
}

void cleanup_cache( struct configuration_data *config,
	struct cache_entry *entry)
{
	char *dbstring;
	if (entry->other_ops != NULL) cleanup_cache(config, entry->other_ops);
	if (entry->left != NULL) cleanup_cache(config, entry->left);
	if (entry->right != NULL) cleanup_cache(config, entry->right);
	if (entry->down != NULL) cleanup_cache(config, entry->down);
	dbstring = cache_create_database_string(NULL,entry);
	do_db(config,dbstring);
	talloc_free(dbstring);
}	
	
	


/*
 * cache_manager runs as a detached thread. Every half second,
 * it's checking if there's data available to store to the DB.
 * in case of yes, it walks through the list of data, enables
 * a new cache, and stores the data in the DB
 */
void cache_manager(struct configuration_data *config )
{
	char *fnnames[] = {
		"write",
		"read",
		"close",
		"rename",
		"open",
		"chdir",
		"rmdir",
		"mkdir",
		NULL,
	};
	int maintenance_c_val = config->maintenance_seconds / 5;
	int maintenance_count = 0;
        pthread_detach(pthread_self());
	/* backup the start and make it possible to
	 * allocate a new cache beginning
	 */
	TALLOC_CTX *database_str = talloc_pool(NULL, 2000);
	while (1 == 1) {
		sleep(5);
		maintenance_count++;
        	pthread_mutex_lock(&cache_mutex);
		struct cache_entry *backup = cache_start;
       		cache_start = NULL;
        	cache_end = NULL;
        	pthread_mutex_unlock(&cache_mutex);
		if (backup != NULL) {
			/* store all existing entries into the database */
			do_db(config,"BEGIN TRANSACTION;");
			cleanup_cache( config,backup);
			do_db(config,"COMMIT;");
			TALLOC_FREE(backup);
		}

		if (maintenance_count == maintenance_c_val) {
			char String[400];
			char dbstring[300];
			struct tm *tm;
			int fncount = 0;
			do_db(config,"BEGIN TRANSACTION;");
			while (fnnames[fncount]!=NULL) {
			        time_t today=time(NULL);
			        time_t delete_date=today - config->maint_run_time;
			        tm = localtime ( &delete_date );


			        sprintf(String,"%04d-%02d-%02d %02d:%02d:%02d", \
                			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, \
                			tm->tm_hour, tm->tm_min, tm->tm_sec);


        			sprintf(dbstring,"delete from %s where timestamp < \"",
					fnnames[fncount]);
        			strcat(dbstring,String);
        			strcat(dbstring,"\";");
				do_db(config,dbstring);
				fncount++;
			}
			do_db(config,"COMMIT;");
			maintenance_count = 0;
		}
				


			
		
	}
}
