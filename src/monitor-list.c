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

int monitor_id = 1;

/*
 * adds an entry to the monitor list
 * returns -1 in case of an error
 */
int monitor_list_add( char *data,int sock) {
        struct monitor_item *entry;
	DEBUG(1) syslog(LOG_DEBUG,"Adding monitor Item %s ",data);
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
		DEBUG(1) syslog(LOG_DEBUG,"returned id %i",entry->id);
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
	DEBUG(1) syslog(LOG_DEBUG,"returned id %i",entry->id);
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



/**
 * delete any monitors associated to the given
 * socket (int sock)
 */
void monitor_list_delete_by_socket( int sock ) {
	struct monitor_item *entry = monlist_start;
	struct monitor_item *before = monlist_start;
	if (entry == NULL) return;
	while (entry != NULL) {
		if (entry->sock==sock) {
			DEBUG(1) syslog(LOG_DEBUG,
				"monitor_list_delete_by_socket: "
				"Removing monitor on socket %i",
				entry->sock);
			free(entry->data);
			before->next=entry->next;
			free(entry);
			entry=before->next;
			continue;
		}
		entry=entry->next;
	}
}


/**
 * delete any running monitors
 */
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



/**
 * list all running monitor for debugging
 */
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


/**
 * parses an argument from a monitor definition
 * sets *pos to the current position
 */
char *monitor_list_parse_argument( char *data, int *pos)
{
	char convdummy[10];
	char value[1000];
	int l;
        strncpy(convdummy,data+*pos,4);
	convdummy[4]='\0';

	DEBUG(1) syslog(LOG_DEBUG,"monitor_list_parse_argument: "
		"convdummy: %s",convdummy);

        l = atoi(convdummy);
        if (l == 0 || l > 999) {
                syslog(LOG_DEBUG,"ERROR: failure argument in monitor request");
                exit(1);
        }
	strncpy(value,data+*pos+4,l);
	value[l] = '\0';
	*pos = *pos + 4 + l;
	return strdup(value);
}



/**
 * fills the strings of the monitor struct with data
 */
int monitor_list_parse( struct monitor_item *entry)
{
	int c = 2;
	DEBUG(1) syslog(LOG_DEBUG,"monitor_list_parse: trying to parse %s",
		entry->data);
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
	DEBUG(1) syslog(LOG_DEBUG,"monitor_list_parse: parsed "
		"id %i, function = %i, param = %s, username = %s,"
		"usersid = %s, share = %s, domain = %s",
		entry->id,
		entry->function,
		entry->param,
		entry->username,
		entry->usersid,
		entry->share,
		entry->domain);

	/*
	 * initialize a memory block with the specific local data
	 * the monitor requires
	 */
	switch(entry->function) {
	case MONITOR_ADD: ;
		entry->local_data = (struct monitor_local_data_adder *)
			 malloc( sizeof(struct monitor_local_data_adder));
		break;
	case MONITOR_TOTAL: ;
		entry->local_data = (struct monitor_local_data_total *)
			malloc( sizeof(struct monitor_local_data_total));
		break;
	default: ;
		syslog(LOG_DEBUG,"ERROR: Wrong monitor state while parsing!\n");
		exit(1);
	
	}

	return 0;	
}


/**
 * get a monitor_item from the monitor list by
 * given id
 */
struct monitor_item *monitor_list_get_by_id(int id)
{
        struct monitor_item *entry=monlist_start;
        while (entry != NULL) {
                if (entry->id == id) return entry;
                entry = entry->next;
        }
        return NULL;
}


/**
 * get a monitor_item from the monitor_list by socket
 */
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

	/* create local unqouted copies of the data */
	char uusername[strlen(username)];
	strncpy(uusername,username+1,strlen(username)-2);
	char uusersid[strlen(usersid)];
	strncpy(uusersid,usersid+1,strlen(usersid)-2);
	char ushare[strlen(share)];
	strncpy(ushare,share+1,strlen(share)-2);
	char udomain[strlen(domain)];
	strncpy(udomain,domain+1,strlen(domain)-2);

        DEBUG(1) syslog(LOG_DEBUG, "monitor_list_apply: entry data:"
                "entry->username : |%s|vs|%s|, "
                "entry->usersid  : |%s|vs|%s|, "
                "entry->share    : %s, "
                "entry->domain   : %s. ",
                entry->username,
		uusername,
		entry->usersid, 
		uusersid,
		entry->share,
		entry->domain);


	if (strcmp(entry->username,"*") != 0) {
		if (strcmp(entry->username, uusername)!=0) return 0;
	}
	if (strcmp(entry->usersid,"*") != 0) {
		if (strcmp(entry->usersid, uusersid)!=0) return 0;
	}
	if (strcmp(entry->share,"*") != 0) {
		if (strcmp(entry->share, ushare)!=0) return 0;
	}
	if (strcmp(entry->domain,"*") != 0) {
		if (strcmp(entry->domain, udomain)!=0) return 0;
	}
	DEBUG(1) syslog(LOG_DEBUG, "monitor_list_apply: "
		"monitor applied succesfully.");
	return 1;
}



/**
 * create a SQL condition string based on the pattern of the
 * given monitor
 */
char *monitor_list_create_sql_cond( struct monitor_item *entry)
{
	char *ret = NULL;
	if (strcmp(entry->username,"*") != 0)
		asprintf(&ret,"username = '%s' and ",entry->username);
	if (strcmp(entry->usersid,"*") != 0)
		asprintf(&ret,"%s usersid = '%s' and ",ret,entry->usersid);
	if (strcmp(entry->share,"*") != 0)
		asprintf(&ret,"%s share = '%s' and ",ret,entry->share);
	if (strcmp(entry->domain,"*") != 0)
		asprintf(&ret,"%s domain = '%s' and ",ret,entry->domain);
	asprintf(&ret,"%s 1 = 1;",ret);
	return ret;
}



/**
 * do everything required before a monitor is going to run
 * - initialize monitor local data variables
 * - in case needed, make SQL requests to look into the past
 */
void monitor_initialize( struct monitor_item *entry)
{
	switch(entry->function) {
	case MONITOR_ADD: ;
		((struct monitor_local_data_adder *) entry->local_data)->sum = 0;
		entry->state = MONITOR_PROCESS;
		break;
	case MONITOR_TOTAL: ;
		((struct monitor_local_data_total *) entry->local_data)->sum = 0;
		char *request;
		if (strcmp(entry->param,"R")==0) {
			asprintf(&request,
				"select sum(length) from read where ");
		} else if (strcmp(entry->param,"W")==0) {
			asprintf(&request,
				"select sum(length) from write where ");
		} else if (strcmp(entry->param,"RW")==0) {
			asprintf(&request,
				"select sum(a.length) + sum(b.length) "
				"from write a, read b where ");
		} else { // FIXME! We have to remove this monitor now !
			}
		char *cond = monitor_list_create_sql_cond(entry);
		asprintf(&request,"%s %s",request,cond);
		free(cond);
		DEBUG(1) syslog(LOG_DEBUG,"monitor_initizalize: "
			"created >%s< as request string!",request);
		query_add(request, strlen(request), entry->sock, entry->id);
		free(request);
		break;
	default: ;
	}
}

void monitor_send_result( struct monitor_item *entry)
{
	char *sendstr = NULL;
	char *idstr = NULL;
	char *tmpdatastr;
	asprintf(&idstr,"%i",entry->id);
	switch(entry->function) {
	case MONITOR_ADD: ;
		asprintf(&tmpdatastr,"%lu",
			((struct monitor_local_data_adder *)
				(entry->local_data))->sum);
		asprintf(&sendstr,"%04i%s%04i%s",
			(int) strlen(idstr),
			idstr,
			(int) strlen(tmpdatastr),
			tmpdatastr);
		sendlist_add(sendstr,entry->sock,strlen(sendstr));
		break;
	case MONITOR_TOTAL: ;
		asprintf(&tmpdatastr,"%lu",
			((struct monitor_local_data_total *)
				(entry->local_data))->sum);
                asprintf(&sendstr,"%04i%s%04i%s",
                        (int) strlen(idstr),
                        idstr,
                        (int) strlen(tmpdatastr),
                        tmpdatastr);
                sendlist_add(sendstr,entry->sock,strlen(sendstr));
                break;

	}
}	
			

void monitor_list_update( int op_id,
	char *username,
	char *usersid,
	char *share,
	char *domain, char *data)
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
				/* simply add on any  VFS op, for testing */
				((struct monitor_local_data_adder *)
					entry->local_data)->sum = 
					((struct monitor_local_data_adder *)
						entry->local_data)->sum + 1;
				monitor_send_result(entry);
				break;
			case MONITOR_TOTAL: ;
				if (op_id == vfs_id_read ||
					op_id == vfs_id_pread ||
					op_id == vfs_id_write ||
					op_id == vfs_id_pwrite ) {
					((struct monitor_local_data_total *)
						entry->local_data)->sum =
						((struct monitor_local_data_total *)entry->local_data)->sum +
						atoi(data);
					monitor_send_result(entry);
				}
				break;
			default: ;

			}
		}
		entry = entry -> next;
	}
}
			
void monitor_list_set_init_result(char *res, int monitorid) {
	/**
	 * we assume that internal queries only return
	 * single numbers at this point
	 */
	struct monitor_item *entry = monlist_start;
	if (monlist_start == NULL) {
		syslog(LOG_DEBUG,"ERROR: trying to initialize a"
			" monitor but the monitor list is empty!");
		exit(1);
	}
	entry = monitor_list_get_by_id(monitorid);
	switch(entry->function) {
	case MONITOR_ADD: ;
		entry->state = MONITOR_PROCESS;
		break;
	case MONITOR_TOTAL: ;
		struct monitor_local_data_total
			*data = entry->local_data;
		data->sum = atol( res+4 );
		DEBUG(1) syslog(LOG_DEBUG, "monitor_list_set_init_result:"
			" input: %s",res);
		entry->state = MONITOR_PROCESS;
		DEBUG(1) syslog(LOG_DEBUG, "monitor_list_set_init_result:"
			" total sum set to %lu",data->sum);
		break;
	}
}

		

void monitor_list_process(int sock) {
	struct monitor_item *entry = monlist_start;

	if (monlist_start==NULL) return;
        entry = monitor_list_get_next_by_socket(sock,
                entry);


	while (entry != NULL) {

		switch(entry->state) {
		case MONITOR_IDENTIFY: ;
			/* Identification: send the id to the client */
			char *data;
			data = talloc_asprintf(NULL,"0010%010i",entry->id);
			sendlist_add(data,entry->sock,strlen(data));
			talloc_free(data);
			DEBUG(1) syslog(LOG_DEBUG,"monitor_list_process: identification done.");
			entry->state = MONITOR_INITIALIZE;
			break;
		case MONITOR_INITIALIZE: ;
			/* do everything required to correctly run the */
			/* monitor. */
			monitor_list_parse( entry );
			monitor_initialize( entry );
			// entry->state = MONITOR_PROCESS;
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
	entry = entry->next;
        entry = monitor_list_get_next_by_socket(sock,
                entry);

	}

}
