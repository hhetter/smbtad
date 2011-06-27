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
int cache_add( char *data, int len,struct configuration_data *config ) {
	pthread_mutex_lock(&cache_mutex);
	struct cache_entry *entry = talloc(cache_start, struct cache_entry);
	entry->left = NULL;
	entry->right = NULL;
	entry->down = NULL;
	entry->other_ops = NULL;
	entry->data = talloc_steal( entry, data);
	struct cache_entry *gotr = cache_start;
	struct cache_entry *backup;
	cache_prepare_entry( entry, entry);
	/**
	 * cache_prepare_entry already called the monitors.
	 * If we don't handle databases, we can go out here!
	 */
	if (config->use_db == 0) {
		talloc_free(entry);
		pthread_mutex_unlock(&cache_mutex);
		return 0;
	}
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
		/* if the first element is a */
		/* operation not using a filename */
		/* then create a fake filename for */
		/* the sorting */
		if (entry->op_id == vfs_id_mkdir ||
		    entry->op_id == vfs_id_chdir ||
		    entry->op_id == vfs_id_rename)
			entry->filename=talloc_strdup(entry,"\"*\"");
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
			struct cache_entry *base_entry = gotr;
			while (gotr != NULL) {
				/*
			 	 * This could be a fitting entry, check it
			 	 */
				if ( entry->op_id == gotr->op_id
					&& config->precision != 0
					&& strncmp(entry->share,
						gotr->share, strlen(entry->share)) == 0
					&& strncmp(entry->filename,
						gotr->filename, strlen(entry->filename)) == 0
					&& strncmp(entry->username,
						gotr->username, strlen(entry->username)) == 0
					&& strncmp(entry->domain,
						gotr->domain, strlen(entry->domain)) == 0) {
					/*
				 	 * entry fits, add the value
				 	 */
					DEBUG(5) syslog(LOG_DEBUG,"cache : adding value.");
					gotr->len = gotr->len + entry->len;
					talloc_free(entry);
					pthread_mutex_unlock(&cache_mutex);
					return 0;
				} else {
				/*
			 	 * That wasn't the right entry, it can be
			 	 * the next down from this entry.
			 	 */
				gotr=gotr->down;
				DEBUG(5) syslog(LOG_DEBUG,"cache: going down on that entry...");
				}
			}
			/*
			 * This entry wasn't yet found, it's being added
			 * to this list
			 */
			DEBUG(5) syslog(LOG_DEBUG,"cache: entry created after moving down");
			backup = base_entry->down;
			base_entry->down = entry;
			entry->down = backup;
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
	return 0;		
}

void cache_update_monitor(struct cache_entry *entry)
{
	monitor_list_update( entry->op_id,
		entry->username,
		entry->usersid,
		entry->share,
		entry->filename,
		entry->domain,
		entry->len,
		entry->timestamp);
}


int cache_prepare_entry( TALLOC_CTX *data,struct cache_entry *entry)
{
	char *go_through = entry->data;
	char *str = NULL;
	char *dummy = NULL;
	entry->len = 0;
	int t;
	/* Nullify all data in an entry */
	entry->username = NULL;
	entry->usersid = NULL;
	entry->share = NULL;
	entry->filename = NULL;
	entry->domain = NULL;
	entry->len = 0;
	entry->timestamp = NULL;

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

	/* in case we received a vfs_id that we don't support, return NULL */
	if (entry->op_id < 0 || entry->op_id >  vfs_id_max -1 ) {
		syslog(LOG_DEBUG,"Unsupported VFS function!");
		TALLOC_FREE(data);
		return -2;
	}
        /* username */
        entry->username = protocol_get_single_data_block( data, &go_through );
        /* user's SID */
        entry->usersid = protocol_get_single_data_block( data, &go_through );
        /* share */
        entry->share = protocol_get_single_data_block( data, &go_through );
        /* domain */
        entry->domain = protocol_get_single_data_block( data, &go_through );
        /* timestamp */
        entry->timestamp = protocol_get_single_data_block( data, &go_through );
	
	/**
	 * In case the protocol transfers more common data blocks
	 * ignore them if we don't support them yet. This has recently
	 * happened in the Samba master and 3.6.0 branch as we have
	 * added support for the IP address of the client to the protocol
	 */
	for ( t=0; t < common_blocks_num-6; t++) {
		dummy = protocol_get_single_data_block( data, &go_through);
		if (dummy == NULL) {
			syslog(LOG_DEBUG,"Fatal: Expected more common data but\n"
				"received NULL!");
			TALLOC_FREE(data);
			return -2;
		}
	}


	/* now receive the VFS function depending arguments */
	switch( entry->op_id) {
	case vfs_id_read:
	case vfs_id_pread: ;
		entry->filename = protocol_get_single_data_block( data, &go_through );
		entry->len = atol(protocol_get_single_data_block( data, &go_through));
		break;
	case vfs_id_write:
	case vfs_id_pwrite: ;
                entry->filename= protocol_get_single_data_block( data,&go_through );
                entry->len = atol(protocol_get_single_data_block( data,&go_through));
                break;
	case vfs_id_mkdir: ;
		entry->path = protocol_get_single_data_block( data,&go_through);
		entry->mode = protocol_get_single_data_block( data,&go_through);
		entry->result=protocol_get_single_data_block( data,&go_through);
		break;
	case vfs_id_chdir: ;
		entry->path = protocol_get_single_data_block( data,&go_through);
		entry->result = protocol_get_single_data_block( data,&go_through);
		break;
	case vfs_id_open: ;
		entry->filename = protocol_get_single_data_block(data,&go_through);
		entry->mode = protocol_get_single_data_block(data,&go_through);
		entry->result = protocol_get_single_data_block(data,&go_through);
		break;
	case vfs_id_close: ;
		entry->filename = protocol_get_single_data_block(data,&go_through);
		entry->result = protocol_get_single_data_block(data,&go_through);
		break;
	case vfs_id_rename: ;
		entry->source = protocol_get_single_data_block(data,&go_through);
		entry->destination = protocol_get_single_data_block(data,&go_through);
		entry->result = protocol_get_single_data_block(data,&go_through);
		break;

	}
	/* With the encoded entry, we call the monitors to action */
	cache_update_monitor(entry);
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
                retstr = talloc_asprintf(ctx, "INSERT INTO data ("
                        "vfs_id, username, usersid, share, domain, timestamp,"
                        "string1, string2, result) VALUES ("
                        "%i, '%s','%s','%s','%s',"
                        "'%s','%s','%s','%s');",
                        entry->op_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->source,entry->destination,entry->result);
		break;
        case vfs_id_close: ;
                retstr = talloc_asprintf(ctx, "INSERT INTO data ("
                        "vfs_id, username, usersid, share, domain, timestamp,"
                        "string1, result) VALUES ("
                        "%i,'%s','%s','%s','%s',"
                        "'%s','%s','%s');",
                        entry->op_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->result);
                break;
        case vfs_id_open: ;
                retstr = talloc_asprintf(ctx, "INSERT INTO data ("
                        "vfs_id, username, usersid, share, domain, timestamp,"
                        "string1, string2, result) VALUES ("
                        "%i,'%s','%s','%s','%s',"
                        "'%s','%s','%s','%s');",
                        entry->op_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->mode,entry->result);
                break;
        case vfs_id_chdir: ;
                retstr = talloc_asprintf( ctx, "INSERT INTO data ("
                        "vfs_id, username, usersid, share, domain, timestamp,"
                        "string1, result) VALUES ("
                        "%i,'%s','%s','%s','%s',"
                        "'%s','%s','%s');",
                        entry->op_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->path,entry->result);
                break;
        case vfs_id_mkdir: ;
                retstr = talloc_asprintf(ctx, "INSERT INTO data ("
                        "vfs_id, username, usersid, share, domain, timestamp,"
                        "string1, string2, result) VALUES ("
                        "%i,'%s','%s','%s','%s',"
                        "'%s','%s','%s','%s');",
                        entry->op_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->path, entry->mode, entry->result);
                break;
        case vfs_id_write:
        case vfs_id_pwrite: ;
                if (entry->len == 0) {
                        retstr=NULL;
                        break;
                }
                retstr = talloc_asprintf(ctx, "INSERT INTO data ("
                        "vfs_id, username, usersid, share, domain, timestamp,"
                        "string1, length) VALUES ("
                        "%i,'%s','%s','%s','%s','%s',"
                        "'%s',%lu);",
                        entry->op_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->len);
                break;	
        case vfs_id_read:
        case vfs_id_pread: ;
                if (entry->len == 0) {
                        retstr=NULL;
                        break;
                }
                retstr = talloc_asprintf(ctx, "INSERT INTO data ("
                        "vfs_id, username, usersid, share, domain, timestamp,"
                        "string1, length) VALUES ("
                        "%i,'%s','%s','%s','%s','%s',"
                        "'%s',%lu);",
                        entry->op_id,entry->username,entry->usersid,entry->share,entry->domain,entry->timestamp,
                        entry->filename,entry->len);
                break;
	default: ;
	}	
	return retstr;
}

void do_db( struct configuration_data *config, char *dbstring)
{
	int rc;
	int try;
	const char *error;
	dbi_result result;
	/** 
	 * Check if the connection is alive. We try ten times
	 * to restore the connection if not
	 */
	for (try = 0; try < 10; try++) {
		rc = dbi_conn_ping( config->DBIconn );
		if (rc == 1) {
			result = dbi_conn_query(config->DBIconn, dbstring);
			if (result == NULL) {
				dbi_conn_error(config->DBIconn,&error);
				syslog(LOG_DEBUG,"DBI ERROR: %s",error);
				rc = 0;}
			else break;
		}
	}
	if (rc == 0 ) {
		syslog(LOG_DEBUG, "ERROR: Database handling error!");
		return;
	}
	dbi_result_free(result);
}

void cleanup_cache( TALLOC_CTX *ctx,struct configuration_data *config,
	struct cache_entry *entry)
{
	char *dbstring;
	struct cache_entry *go = entry;
	struct cache_entry *backup;
	struct cache_entry *backup2 = NULL;
	struct cache_entry *down =NULL;
	// left
	go = go->left;
	while (go != NULL) {
		backup = go->left;
		dbstring = cache_create_database_string(ctx,go);
		do_db(config,dbstring);
		talloc_free(dbstring);
		// go down
		down = go->down;
		while (down != NULL) {
			backup2 = down->down;
			dbstring = cache_create_database_string(ctx,down);
			do_db(config,dbstring);
			talloc_free(dbstring);
			talloc_free(down);
			down = backup2;
		}
		talloc_free(go);
		go = backup;
	}
	// right
	go = entry->right;
        while (go != NULL) {
                backup = go->right;
                dbstring = cache_create_database_string(ctx,go);
                do_db(config,dbstring);
		talloc_free(dbstring);
                // go down
                down = go->down;
                while (down != NULL) { 
                        backup2 = down->down;
                        dbstring = cache_create_database_string(ctx,down);
                        do_db(config,dbstring);
                        talloc_free(dbstring);
                        talloc_free(down);
                        down = backup2;
                }
                talloc_free(go);
                go = backup;
        }
	// other_ops
	go = entry->other_ops;
	while (go != NULL) {
		backup = go->other_ops;
		dbstring = cache_create_database_string(ctx,go);
		do_db(config,dbstring);
		talloc_free(dbstring);
		talloc_free(go);
		go = backup;
	}
	// delete tree begin
	dbstring = cache_create_database_string(ctx,entry);
	do_db(config,dbstring);
	if (dbstring != NULL) talloc_free(dbstring);
	talloc_free(entry);
}	
	
	


/*
 * cache_manager runs as a detached thread. Every half second,
 * it's checking if there's data available to store to the DB.
 * in case of yes, it walks through the list of data, enables
 * a new cache, and stores the data in the DB
 */
void cache_manager(struct configuration_data *config )
{
	int maintenance_c_val;
	if (config->precision>0)
		maintenance_c_val = config->maintenance_seconds / config->precision;
	else maintenance_c_val = config->maintenance_seconds;

	int maintenance_count = 0;
        pthread_detach(pthread_self());
	/* backup the start and make it possible to
	 * allocate a new cache beginning
	 */
	while (1 == 1) {
		if (config->precision != 0) sleep(config->precision);
		else sleep(5);
		maintenance_count++;
        	pthread_mutex_lock(&cache_mutex);
		struct cache_entry *backup = cache_start;
		TALLOC_CTX *dbpool = talloc_pool(NULL,2048);
       		cache_start = NULL;
        	cache_end = NULL;
		pthread_mutex_unlock(&cache_mutex);
		if (backup != NULL) {
			/* store all existing entries into the database */
			do_db(config,"BEGIN TRANSACTION;");
			cleanup_cache( dbpool,config,backup);
			do_db(config,"COMMIT;");
		}
		talloc_free(dbpool);

		if (maintenance_count == maintenance_c_val) {
			char String[400];
			char dbstring[300];
			struct tm *tm;
			do_db(config,"BEGIN TRANSACTION;");
		        time_t today=time(NULL);
		        time_t delete_date=today - config->maint_run_time;
		        tm = localtime ( &delete_date );


		        sprintf(String,"%04d-%02d-%02d %02d:%02d:%02d", \
               			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, \
               			tm->tm_hour, tm->tm_min, tm->tm_sec);


       			strcpy(dbstring,"delete from data where timestamp < '");
       			strcat(dbstring,String);
       			strcat(dbstring,"';");
			do_db(config,dbstring);
			do_db(config,"COMMIT;");
			maintenance_count = 0;
		}
				


			
		
	}
}
