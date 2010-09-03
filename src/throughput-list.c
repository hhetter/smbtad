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
#include <sys/time.h>
pthread_mutex_t throughput_lock;


/*
 * init the cache system */
void throughput_list_init( ) {
        pthread_mutex_init(&throughput_lock, NULL);
}


int throughput_list_add( struct throughput_list_base *list,
	unsigned long int value,
	char *timestr)
{
	pthread_mutex_lock(&throughput_lock);
	/* create a local unqouted copy of the timestr */
        char utimestr[strlen(timestr)];
        strncpy(utimestr,timestr+1,strlen(timestr)-4);

	struct tm current;
	struct throughput_list_item *entry = (struct throughput_list_item *) malloc(
		sizeof(struct throughput_list_item));
	entry->next = NULL;
	entry->value = value;

	if (strptime( utimestr, "%Y-%m-%d %T",&current) == NULL) syslog(LOG_DEBUG,"ERROR STRPTIME! %s",utimestr);
	entry->timestamp = mktime(&current);
	entry->milliseconds = atoi(timestr + strlen(timestr) - 4);
	DEBUG(1) syslog(LOG_DEBUG,"throughput_list_add: saved milliseconds: %i",entry->milliseconds);
	DEBUG(1) syslog(LOG_DEBUG,"throughput_list_add: adding value %lui",value);
	if (list->begin == NULL) {
		list->begin = entry;
		list->end = entry;
	} else {
		list->end->next = entry;
		list->end = entry;
	}
	pthread_mutex_unlock(&throughput_lock);
	return 0;

}

unsigned long int throughput_list_throughput_per_second(
        struct throughput_list_base *list) {

	pthread_mutex_lock(&throughput_lock);
	struct timeval tv;
	gettimeofday(&tv,NULL);
	int milliseconds = tv.tv_usec / 1000;
	
	unsigned long int add = 0;
	time_t now = time(NULL);
	struct throughput_list_item *entry =
		list->begin;
	struct throughput_list_item *backup =
		entry;
	double a=0;
	while ( entry != NULL) {
		a=difftime(now,entry->timestamp);
		if ( (a == 0) ||
			( a == 1 && milliseconds <= entry->milliseconds)) {
			add = add + entry->value;
			backup = entry;
			DEBUG(1) syslog(LOG_DEBUG,"adding value %i",
				entry->value);
			entry = entry->next;
		} else { /* remove this entry */
			if (entry == list->begin) {
				list->begin = backup->next;
				list->end = backup->next;
				free(backup);
				entry = list->begin;
				backup = entry;
			} else if (entry == list->end) {
				backup->next = NULL;
				free(entry);
				list->end = backup;
				entry = NULL;
			} else {
				backup->next = entry->next;
				free(entry);
				entry = backup->next;
				
			}
		}

	}
	pthread_mutex_unlock(&throughput_lock);

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

		

