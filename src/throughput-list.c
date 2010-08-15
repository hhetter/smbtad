/* 
 * smtbad 
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
#include <stdio.h>


int throughput_list_add( struct throughput_list_base *list,
	unsigned long int value,
	char *timestr)
{
	struct throughput_list_item *entry = malloc(
		sizeof(struct throughput_list));
	entry->next = NULL;
	entry->value = value;
	strptime( &timestr, "%Y-%m-%d %T",&entry->timestamp);
	if (list->begin == NULL) {
		list->begin = entry;
		list->end = entry;
	} else {
		list->end->next = entry;
		list->end = entry;
	}
	return 0;
}

unsigned long int throughput_list_throughput_per_second(
        struct throughput_list_base *list) {

	unsigned long int add = 0;
	time_t now = time(NULL);
	struct throughput_list_item *entry =
		list->begin;
	struct throughput_list_item *backup =
		entry;

	while ( entry != NULL) {
		if (difftime(now,entry->timestamp) == 0)
			add = add + entry->value;
		else { /* remove this entry */
			if (backup == list->begin) {
				list->begin = backup->next;
				free(backup);
			} else {
				backup->next = entry->next;
				free(entry);
				entry = backup;
			}
		}
		backup = entry;
		entry = entry->next;
	}
	return add;
}


void throughput_list_free( struct throughput_list_base *list) {
	struct throughput_list_item *entry =
		list->begin;
	struct throughput_list_item *backup =
		list->begin;
	while (entry != NULL) {
		entry=entry->next;
		free(backup);
		backup=entry;
	}
}

		

