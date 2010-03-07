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

int cache_add( struct cache_entry *entry ) {
	
	if (cache_start == NULL) {
		cache_start = entry;
		entry->next = NULL;
		cache_end = entry;
		return 0;
	}

	cache_end->next = entry;
	entry->next = NULL;
	cache_end = entry;
	return 0;
}

