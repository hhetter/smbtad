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
struct monitor_item *monlist_start = NULL;
struct monitor_item *monlist_end = NULL;

int monitor_id = 0;

/*
 * adds an entry to the monitor list
 * returns -1 in case of an error
 */
int monitor_list_add( char *data,int sock) {
        struct monitor_item *entry;
        if (monlist_start == NULL) {
                monlist_start = (struct monitor_item *) malloc( sizeof( struct monitor_item));
		if (monlist_start == NULL) {
			syslog(LOG_DEBUG,"ERROR: could not allocate memory!");
			exit(1);
		}
                entry = monlist_start;
                entry->data = strdup(data);
                monlist_start = entry;
                entry->next = NULL;
                monlist_end = entry;
                entry->sock = sock;
		entry->id = monitor_id;
		
		entry->param = NULL;
		entry->username = NULL;
		entry->usersid = NULL;
		entry->function = 255;
		entry->share = NULL;
		entry->domain = NULL;
		entry->local_data = NULL;
		monitor_id ++;
		entry->state = MONITOR_IDENTIFY;
                return entry->id;
        }
        entry = (struct monitor_item *) malloc(sizeof(struct monitor_item));
	if (entry == NULL) {
		syslog(LOG_DEBUG,"ERROR: could not allocate memory!");
		exit(1);
	}
        monlist_end->next = entry;
        entry->next = NULL;
        entry->data = strdup(data);
        monlist_end = entry;
        entry->sock = sock;
	entry->id = monitor_id;
        entry->param = NULL;
        entry->username = NULL;
        entry->usersid = NULL;
        entry->function = 255;
        entry->share = NULL;
        entry->domain = NULL;

	monitor_id++;
	entry->state = MONITOR_IDENTIFY;
        return entry->id;
}

int monitor_list_delete( int id ) {
	struct monitor_item *entry = monlist_start;
	struct monitor_item *before = monlist_start;
	if (entry == NULL) return -1;
	while (entry != NULL) {
		if (entry->id == id) {
			free(entry->data);
			before->next=entry->next;
			free(entry);
			return 0;
		}
		entry=entry->next;
	}
	return -1; // id was not found
}

void monitor_list_delete_all() {
	struct monitor_item *entry = monlist_start;
	struct monitor_item *n = monlist_start;
	while (entry != NULL) {
		n=entry->next;
		free(entry->data);
		free(entry);
		entry = n;
	}
}




void monitor_list_list() {
	struct monitor_item *entry = monlist_start;
	syslog(LOG_DEBUG,"------------ monitor list ----------");
	if (entry == NULL) {
		syslog(LOG_DEBUG,"Empty Monitor list.");
		return;
	}
	while (entry!=NULL) {
		syslog(LOG_DEBUG,"%s",entry->data);
		entry=entry->next;
	}
}

char *monitor_list_parse_argument( char *data, int *pos)
{
	char convdummy[10];
	char value[1000];
	int l;
        strncpy(convdummy,data+*pos,4);
        l = atoi(convdummy);
        if (l == 0 || l > 999) {
                syslog(LOG_DEBUG,"ERROR: failure argument in monitor request");
                exit(1);
        }
	strncpy(value,data+*pos+4,l);
	*pos = *pos + 4 + l;
	return strdup(value);
}




int monitor_list_parse( struct monitor_item *entry)
{
	int c = 2;
	entry->function = atoi(
		monitor_list_parse_argument( entry->data, &c));
	entry->param =
		monitor_list_parse_argument( entry->data, &c);
	entry->username =
		monitor_list_parse_argument( entry->data, &c);
	entry->usersid =
		monitor_list_parse_argument( entry->data, &c);
	entry->share =
		monitor_list_parse_argument( entry->data, &c);
	entry->domain =
		monitor_list_parse_argument( entry->data, &c);

	/*
	 * initialize a memory block with the specific local data
	 * the monitor requires
	 */
	switch(entry->function) {
	case MONITOR_ADD: ;
		entry->local_data = (struct monitor_local_data_adder *)
			 malloc( sizeof(struct monitor_local_data_adder));
		break;
	default: ;
		syslog(LOG_DEBUG,"ERROR: Wrong monitor state while parsing!\n");
		exit(1);
	
	}

	return 0;	
}




struct monitor_item *monitor_list_get_next_by_socket(int sock,
		struct monitor_item *item)
{
	struct monitor_item *entry=item;
	while (entry != NULL) {
		if (entry->sock==sock) return entry;
		entry = entry->next;
	}
	return NULL;
}

/* returns 1 if the filter applies to the incoming data,
 * 0 if not
 */
int monitor_list_filter_apply( struct monitor_item *entry, 
	char *username,
	char *usersid,
	char *share,
	char *domain)
{
	if (strcmp(entry->username,"*") != 0) {
		if (strcmp(entry->username, username)!=0) return 0;
	}
	if (strcmp(entry->usersid,"*") != 0) {
		if (strcmp(entry->usersid, usersid)!=0) return 0;
	}
	if (strcmp(entry->share,"*") != 0) {
		if (strcmp(entry->share, share)!=0) return 0;
	}
	if (strcmp(entry->domain,"*") != 0) {
		if (strcmp(entry->domain, domain)!=0) return 0;
	}
	return 1;
}

void monitor_list_update( int op_id,
	char *username,
	char *usersid,
	char *share,
	char *domain)
{
	struct monitor_item *entry = monlist_start;

	while (entry != NULL) {
		if (monitor_list_filter_apply(entry,
			username,
			usersid,
			share,
			domain) == 1) {
			/* processing monitor */
			switch(entry->function) {
			case MONITOR_ADD: ;
				struct monitor_local_data_adder
					*data = entry->local_data;

				/* simply add, for testing */
				data->sum = data->sum +1;
				break;
			default: ;

			}
		}
	}
}
			

void monitor_list_process(int sock) {
	struct monitor_item *entry = monlist_start;

	if (monlist_start==NULL) return;

	entry = monitor_list_get_next_by_socket(sock,
		entry);

	switch(entry->state) {
	case MONITOR_IDENTIFY: ;
		/* Identification: send the id to the client */
		char *data;
		data = talloc_asprintf(NULL,"0010%010i",entry->id);
		sendlist_add(data,entry->sock,strlen(data));
		talloc_free(data);
		entry->state = MONITOR_INITIALIZE;
		break;
	case MONITOR_INITIALIZE: ;
		/* do everything required to correctly run the */
		/* monitor. */
		monitor_list_parse( entry );
		entry->state = MONITOR_PROCESS;
		break;
	case MONITOR_PROCESS: ;
		/* process the monitor, create a result and send */
		/* the result. This is done when receiving data */
		break;
	case MONITOR_STOP: ;
		/* delete a monitor from the monitor_list */
		break;
	case MONITOR_ERROR: ;
		/* send an error message to the client, and delete */
		/* the monitor from the list */
		break;
/*	case default:
		syslog(LOG_DEBUG,"ERROR: Unkown monitor state!");
		exit(1);
		break;
*/
	}

}
