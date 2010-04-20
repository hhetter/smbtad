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


struct query_entry *query_start = NULL;
struct query_entry *query_end = NULL;


pthread_mutex_t query_mutex;


/*
 * init the cache system */
void query_init( ) {
        pthread_mutex_init(&query_mutex, NULL);
}




/*
 * adds an entry to the querylist
 * returns -1 in case of an error
 */
int query_add( char *data, int len ) {

	
        struct query_entry *entry;	
	pthread_mutex_lock(&query_mutex);
	if (query_start == NULL) {
		query_start = talloc(NULL, struct query_entry);
		entry = query_start;
		entry->data = talloc_steal( query_start, data);
		entry->length = len;
		query_start = entry;
		entry->next = NULL;
		query_end = entry;
		pthread_mutex_unlock(&query_mutex);
		return 0;
	}
	entry = talloc(query_start, struct query_entry);
	query_end->next = entry;
	entry->next = NULL;
	entry->data = talloc_steal( query_start, data);
	entry->length = len;
	query_end = entry;
	pthread_mutex_unlock(&query_mutex);
	return 0;
}


