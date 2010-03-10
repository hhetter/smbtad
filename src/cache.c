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

int cache_add( char *data, int len ) {
	
	if (cache_start == NULL) {
		struct cache_entry *entry =
			malloc(sizeof(struct cache_entry));
		entry->data = data;
		entry->length = len;
		cache_start = entry;
		entry->next = NULL;
		cache_end = entry;
		return 0;
	}
	struct cache_entry *entry = malloc(sizeof(struct cache_entry));
	cache_end->next = entry;
	entry->next = NULL;
	entry->data = data;
	entry->length = len;
	cache_end = entry;
	return 0;
}

int cache_store( )
{
	/* backup the start and make it possible to
	 * allocate a new cache beginning
	 */
	struct cache_entry *begin = cache_start;
	cache_start = NULL;
	cache_end = NULL;

	while (begin != NULL) {
		char *a = cache_make_database_string( begin );
		cache_commit_to_db( a );
		struct cache_entry *dummy = begin;
		begin = begin->next;
		free(dummy->data);
		free(dummy);
	}
}
