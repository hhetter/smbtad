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


void sendlist_init() {
	pthread_mutex_init(&sendlist_lock, NULL);
}


/*
 * adds an entry to the send list
 * returns -1 in case of an error
 */
int sendlist_add( char *data,int sock, int length) {
        struct sendlist_item *entry;
	pthread_mutex_lock(&sendlist_lock);
	DEBUG(1) syslog(LOG_DEBUG,
		"sendlist_add: Adding Item %s to the sendlist.",data);
        if (sendlist_start == NULL) {
                sendlist_start = (struct sendlist_item *)
			malloc( sizeof( struct sendlist_item));
		if (sendlist_start == NULL) {
			syslog(LOG_DEBUG,
				"ERROR: could not allocate memory!");
			exit(1);
		}
                entry = sendlist_start;
                entry->data = (char *) malloc(sizeof(char)* length +1);
		memcpy(entry->data, data,length);
                sendlist_start = entry;
                entry->next = NULL;
                sendlist_end = entry;
                entry->sock = sock;
		entry->len = length;
		entry->send_len = 0;
		entry->state = SENDLIST_STATUS_SEND_HEADER;
		entry->header = NULL; 
		pthread_mutex_unlock(&sendlist_lock);
               return 0;
        }
        entry = (struct sendlist_item *) malloc(sizeof(struct sendlist_item));
	if (entry == NULL) {
		syslog(LOG_DEBUG,"ERROR: could not allocate memory!");
		exit(1);
	}
        sendlist_end->next = entry;
        entry->next = NULL;
	entry->data = (char *) malloc(sizeof(char) * length +1 );
	memcpy(entry->data,data,length);
        sendlist_end = entry;
        entry->sock = sock;
	entry->len = length;
	entry->send_len = 0;
	entry->state = SENDLIST_STATUS_SEND_HEADER;
	entry->header = NULL;
	pthread_mutex_unlock(&sendlist_lock);
        return 0;
}

/*
 * get the first item of the sendlist, check if it's possible to send,
 * send the item and remove the first item
 */
int sendlist_send( fd_set *write_fd_set ) {
	struct sendlist_item *entry;
	pthread_mutex_lock(&sendlist_lock);
	entry = sendlist_start;
	if (entry == NULL || !FD_ISSET(entry->sock, write_fd_set)) {
		pthread_mutex_unlock(&sendlist_lock);
		return 0;
	}
	
	switch(entry->state) {

	case SENDLIST_STATUS_SEND_HEADER:
		entry->header = network_create_header(NULL,
                                "000000\0",
                                entry->len);		
		entry->send_len = send(entry->sock,
			entry->header + entry->send_len,
			strlen(entry->header)-entry->send_len,0);

                if (entry->send_len != strlen(entry->header)) {
                        /* Not transferred the full header yet */
                        entry->state = SENDLIST_STATUS_SEND_HEADER_ONGOING;
			pthread_mutex_unlock(&sendlist_lock);
                        return 0;
                }
                entry->state = SENDLIST_STATUS_SEND_DATA;
		pthread_mutex_unlock(&sendlist_lock);
		return 0;

	case SENDLIST_STATUS_SEND_HEADER_ONGOING:
		entry->send_len = entry->send_len + send(entry->sock,
			entry->header + entry->send_len,
			strlen(entry->header)-entry->send_len,0);
		if (entry->send_len != strlen(entry->header)) {
			pthread_mutex_unlock(&sendlist_lock);
			return 0;
		}
		entry->state = SENDLIST_STATUS_SEND_DATA;
		pthread_mutex_unlock(&sendlist_lock);
		return 0;

	case SENDLIST_STATUS_SEND_DATA:
		entry->send_len = 0;
		DEBUG(1) syslog(LOG_DEBUG,
			"sendlist_send: sending %i bytes... ",entry->len);
		entry->send_len =
			send(entry->sock, entry->data, entry->len,0);
		if (entry->send_len != entry->len) {
			entry->state = SENDLIST_STATUS_SEND_DATA_ONGOING;
			pthread_mutex_unlock(&sendlist_lock);
			return 0;
		}
		break;

	case SENDLIST_STATUS_SEND_DATA_ONGOING:
		entry->send_len = entry->send_len + 
			send(entry->sock,
			entry->data + entry->send_len,
			entry->len-entry->send_len,0);

		if (entry->send_len != entry->len) {
			pthread_mutex_unlock(&sendlist_lock);
			return 0;
		}
		break;
	}
	/* succesfully sent the data, we can remove the item */
	talloc_free(entry->header);
	free(entry->data);
	sendlist_start=entry->next;
	free(entry);
	pthread_mutex_unlock(&sendlist_lock);
	return 0;
}

void sendlist_list() {
        pthread_mutex_lock(&sendlist_lock);
	struct sendlist_item *entry = sendlist_start;
	syslog(LOG_DEBUG,"------------ sendlist ----------");
	if (entry == NULL) {
		syslog(LOG_DEBUG,"Empty sendlist.");
		pthread_mutex_unlock(&sendlist_lock);
		return;
	}
	int c = 0;
	while (entry!=NULL) {
		c++;
		entry=entry->next;
	}
	syslog(LOG_DEBUG,"%i entries in sendlist.",c);
	pthread_mutex_unlock(&sendlist_lock);
}


