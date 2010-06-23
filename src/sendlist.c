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

struct sendlist_item *sendlist_start = NULL;
struct sendlist_item *sendlist_end = NULL;


/*
 * adds an entry to the send list
 * returns -1 in case of an error
 */
int sendlist_add( char *data,int sock) {
        struct sendlist_item *entry;
        if (sendlist_start == NULL) {
                sendlist_start = (struct sendlist_item *) malloc( sizeof( struct sendlist_item));
		if (sendlist_start == NULL) {
			syslog(LOG_DEBUG,"ERROR: could not allocate memory!");
			exit(1);
		}
                entry = sendlist_start;
                entry->data = strdup(data);
                sendlist_start = entry;
                entry->next = NULL;
                sendlist_end = entry;
                entry->sock = sock;
                return 0;
        }
        entry = (struct sendlist_item *) malloc(sizeof(struct sendlist_item));
	if (entry == NULL) {
		syslog(LOG_DEBUG,"ERROR: could not allocate memory!");
		exit(1);
	}
        sendlist_end->next = entry;
        entry->next = NULL;
        entry->data = strdup(data);
        sendlist_end = entry;
        entry->sock = sock;
        return 0;
}


void sendlist_list() {
	struct sendlist_item *entry = sendlist_start;
	syslog(LOG_DEBUG,"------------ sendlist ----------");
	if (entry == NULL) {
		syslog(LOG_DEBUG,"Empty sendlist.");
		return;
	}
	while (entry!=NULL) {
		syslog(LOG_DEBUG,"%s",entry->data);
		entry=entry->next;
	}
}


