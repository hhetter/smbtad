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
int monitor_timer_flag = 0;

void monitor_list_init()
{
}

/*
 * adds an entry to the monitor list
 * returns -1 in case of an error
 */
int monitor_list_add( char *data,int sock) {
        struct monitor_item *entry;
	DEBUG(1) syslog(LOG_DEBUG,"Adding monitor Item %s ",data);
        if (monlist_start == NULL) {
                monlist_start = 
			(struct monitor_item *)
			malloc( sizeof( struct monitor_item));
		if (monlist_start == NULL) {
			syslog(LOG_DEBUG,
				"ERROR: could not allocate memory!");
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

int monitor_list_delete_by_socket( int sock ) {
	struct monitor_item *entry = monlist_start;
	struct monitor_item *before = monlist_start;
	if (entry == NULL) return -1;
	while (entry != NULL) {
		if (entry->sock == sock) {
			free(entry->data);
			before->next = entry->next;
			if (entry==monlist_start)
				monlist_start = entry->next;
			free(entry);
			
			return 0;
		}
		before = entry;
		entry = entry->next;
	}
	return -1; // socket wasn't found
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
 * list all running monitors for debugging
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
	entry->file =
		monitor_list_parse_argument( entry->data, &c);
	entry->domain =
		monitor_list_parse_argument( entry->data, &c);
	DEBUG(1) syslog(LOG_DEBUG,"monitor_list_parse: parsed "
		"id %i, function = %i, param = %s, username = %s,"
		"usersid = %s, share = %s, file = %s, domain = %s",
		entry->id,
		entry->function,
		entry->param,
		entry->username,
		entry->usersid,
		entry->share,
		entry->file,
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
	case MONITOR_LOG: ;
		entry->local_data = (struct monitor_local_data_log *)
			malloc( sizeof(struct monitor_local_data_log));
		break;
	case MONITOR_READ: ;
		entry->local_data = (struct monitor_local_data_read *)
			malloc(sizeof(struct monitor_local_data_read));
		break;
	case MONITOR_WRITE: ;
		entry->local_data = (struct monitor_local_data_write *)
			malloc(sizeof(struct monitor_local_data_write));
		break;
	default: ;
		syslog(LOG_DEBUG,"ERROR: Wrong monitor state while parsing!\n");
		exit(1);
	
	}
	entry->state = MONITOR_INITIALIZE_INIT;
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
	char *file,
	char *domain)
{

        DEBUG(1) syslog(LOG_DEBUG, "monitor_list_apply: entry data:"
                "entry->username : |%s|vs|%s|, "
                "entry->usersid  : |%s|vs|%s|, "
                "entry->share    : %s, "
		"entry->file	 : %s, "
                "entry->domain   : %s. ",
                entry->username,
		username,
		entry->usersid, 
		usersid,
		entry->share,
		entry->file,
		entry->domain);

	if (entry->username == NULL || entry->usersid == NULL
		|| entry->share == NULL || entry->file == NULL
		|| entry->domain == NULL) return 0;

	if (strcmp(entry->username,"*") != 0) {
		if (strcmp(entry->username, username)!=0) return 0;
	}
	if (strcmp(entry->usersid,"*") != 0) {
		if (strcmp(entry->usersid, usersid)!=0) return 0;
	}
	if (strcmp(entry->share,"*") != 0) {
		if (strcmp(entry->share, share)!=0) return 0;
	}
	if (strcmp(entry->file,"*") != 0) {
		if (strcmp(entry->file, file)!=0) return 0;
	}
	if (strcmp(entry->domain,"*") != 0) {
		if (strcmp(entry->domain, domain)!=0) return 0;
	}
	DEBUG(1) syslog(LOG_DEBUG, "monitor_list_apply: "
		"monitor applied succesfully.");
	return 1;
}



/**
 * create a SQL condition string based on the pattern of the
 * given monitor
 */
char *monitor_list_create_sql_cond( TALLOC_CTX *ctx,struct monitor_item *entry)
{
	char *ret = NULL;
	char *username = NULL;
	char *usersid = NULL;
	char *share = NULL;
	char *file = NULL;
	char *domain = NULL;

	if (strcmp(entry->username,"*") != 0)
		username = talloc_asprintf(ctx,"username = '%s' and ",entry->username);
	else username = talloc_asprintf(ctx," ");
	if (strcmp(entry->usersid,"*") != 0)
		usersid = talloc_asprintf(ctx, "usersid = '%s' and ",entry->usersid);
	else usersid = talloc_asprintf(ctx," ");
	if (strcmp(entry->share,"*") != 0)
		share = talloc_asprintf(ctx,"share = '%s' and ",entry->share);
	else share = talloc_asprintf(ctx," ");
	if (strcmp(entry->file,"*") != 0)
		file = talloc_asprintf(ctx,"file = '%s' and ",entry->file);
	else file = talloc_asprintf(ctx," ");
	if (strcmp(entry->domain,"*") != 0)
		domain = talloc_asprintf(ctx,"domain = '%s' and ",entry->domain);
	else domain = talloc_asprintf(ctx," ");
	ret = talloc_asprintf(ctx,"%s %s %s %s %s 1 = 1;",
		username,
		usersid,
		share,
		file,
		domain);
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
		((struct monitor_local_data_adder *)
			entry->local_data)->sum = 0;
		entry->state = MONITOR_PROCESS;
		break;
	case MONITOR_TOTAL: ;
		((struct monitor_local_data_total *)
			entry->local_data)->sum = 0;
		char *request;
		if (strcmp(entry->param,"R")==0) {
			request = talloc_asprintf(NULL,
				"select sum(length) from read where ");
		} else if (strcmp(entry->param,"W")==0) {
			request = talloc_asprintf(NULL,
				"select sum(length) from write where ");
		} else if (strcmp(entry->param,"RW")==0) {
			request = talloc_asprintf(NULL,
				"select sum(length) from write UNION "
				"select sum(length) from read where ");
		} else { // FIXME! We have to remove this monitor now !
			}
		char *cond = monitor_list_create_sql_cond(NULL, entry);
		request = talloc_asprintf(NULL,"%s %s",request,cond);
		talloc_free(cond);
		DEBUG(1) syslog(LOG_DEBUG,"monitor_initizalize: "
			"created >%s< as request string!",request);
		query_add(request,
			strlen(request),
			entry->sock, entry->id);
		entry->state = MONITOR_INITIALIZE_INIT_PREP;
		talloc_free(request);
		break;
	case MONITOR_LOG:
		((struct monitor_local_data_log *)
			entry->local_data)->log = NULL;
		entry->state = MONITOR_PROCESS;
		break;
	case MONITOR_READ:
		((struct monitor_local_data_read *)
			entry->local_data)->read = 0;
		entry->state = MONITOR_PROCESS;
		break;
	case MONITOR_WRITE:
		((struct monitor_local_data_write *)
			entry->local_data)->write = 0;
		entry->state = MONITOR_PROCESS;
		break;
	default: ;
	}
}

void monitor_send_result( struct monitor_item *entry)
{
	char *sendstr = NULL;
	char *idstr = NULL;
	char *tmpdatastr;
	idstr = talloc_asprintf(NULL,"%i",entry->id);
	switch(entry->function) {
	case MONITOR_ADD: ;
		tmpdatastr = talloc_asprintf(NULL,"%lu",
			((struct monitor_local_data_adder *)
				(entry->local_data))->sum);
		sendstr = talloc_asprintf(tmpdatastr,"%04i%s%04i%s",
			(int) strlen(idstr),
			idstr,
			(int) strlen(tmpdatastr),
			tmpdatastr);
		network_send_data(sendstr,entry->sock,strlen(sendstr));
		talloc_free(tmpdatastr);
		break;
	case MONITOR_TOTAL: ;
		tmpdatastr = talloc_asprintf(NULL,"%lu",
			((struct monitor_local_data_total *)
				(entry->local_data))->sum);
                sendstr = talloc_asprintf(tmpdatastr,"%04i%s%04i%s",
                        (int) strlen(idstr),
                        idstr,
                        (int) strlen(tmpdatastr),
                        tmpdatastr);
                network_send_data(sendstr,entry->sock,strlen(sendstr));
		talloc_free(tmpdatastr);
                break;
	case MONITOR_LOG: ;
		tmpdatastr = talloc_asprintf(NULL,"%s",
			((struct monitor_local_data_log *)
				(entry->local_data))->log);
		sendstr = talloc_asprintf(tmpdatastr,"%04i%s%04i%s",
			(int) strlen(idstr),
			idstr,
			(int) strlen(tmpdatastr),
			tmpdatastr);
		network_send_data(sendstr,entry->sock,strlen(sendstr));
		talloc_free(tmpdatastr);
		break;
	case MONITOR_READ: ;
		tmpdatastr = talloc_asprintf(NULL,"%lu",
			((struct monitor_local_data_read *)
				(entry->local_data))->read);
		sendstr = talloc_asprintf(tmpdatastr,"%04i%s%04i%s",
			(int) strlen(idstr),
			idstr,
			(int) strlen(tmpdatastr),
			tmpdatastr);
		network_send_data(sendstr,entry->sock,strlen(sendstr));
		talloc_free(tmpdatastr);
		break;
        case MONITOR_WRITE: ;
                tmpdatastr = talloc_asprintf(NULL,"%lu",
                        ((struct monitor_local_data_write *)
                                (entry->local_data))->write);
                sendstr = talloc_asprintf(&tmpdatastr,"%04i%s%04i%s",
                        (int) strlen(idstr),
                        idstr,
                        (int) strlen(tmpdatastr),
                        tmpdatastr);
                network_send_data(sendstr,entry->sock,strlen(sendstr));
		talloc_free(tmpdatastr);
                break;
	default: ;
		break;
	}
	talloc_free(idstr);
}	
			

void monitor_list_update( int op_id,
	char *username,
	char *usersid,
	char *share,
	char *file,
	char *domain, unsigned long int data, char *montimestamp)
{
	char *fname;
	struct monitor_item *entry = monlist_start;
	char *op_id_str = NULL;
	while (entry != NULL) {
		if (monitor_list_filter_apply(entry,
			username,
			usersid,
			share,
			file,
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
				if ( (op_id == vfs_id_read ||
					op_id == vfs_id_pread) && (
					strcmp(entry->param,"R")== 0 ||
					strcmp(entry->param,"RW")==0 ))
					{
					((struct monitor_local_data_total *)
						entry->local_data)->sum =
						((struct monitor_local_data_total *)
						entry->local_data)->sum +
						data;
					monitor_send_result(entry);
					break;
				}
				if ( (op_id == vfs_id_write ||
					op_id == vfs_id_pwrite) &&
					(strcmp(entry->param,"W")==0 ||
					 strcmp(entry->param,"RW")==0))
					{
                                        ((struct monitor_local_data_total *)
                                                entry->local_data)->sum =
                                                ((struct monitor_local_data_total *)
                                                entry->local_data)->sum +
                                                data;
                                        monitor_send_result(entry);
                                        break;
				} 
				break;
			case MONITOR_READ:
				if ((op_id == vfs_id_read ||
					op_id == vfs_id_pread)) {
					((struct monitor_local_data_read *)
					entry->local_data)->read = data;
					monitor_send_result(entry);
					break;
				}
				break;
			case MONITOR_WRITE:
				if ((op_id == vfs_id_write ||
					op_id == vfs_id_pwrite)) { 
					((struct monitor_local_data_write *)
					entry->local_data)->write = data;
					monitor_send_result(entry);
					break;
				}
				break;
			case MONITOR_LOG:
				/**
				 * log protocol
				 * vfs_op_id,username,share,filename,domain,timestamp
				 * 
				*/
				
				op_id_str = talloc_asprintf( NULL, "%i",op_id);
				/**
				 * if the filename is NULL, for example in a chdir operation,
				 * create a "\0" based fake filename
				 */
				if (file == NULL) fname=talloc_asprintf(NULL," ");
				else fname = file;
				char *tres = talloc_asprintf(op_id_str,"%04i%s" // op id
                                	"%04i%s" // username
					"%04i%s" // share
					"%04i%s" // filename
					"%04i%s" // domain
					"%04i%s", // timestamp
					(int) strlen(op_id_str),
					op_id_str,
					(int) strlen(username),
					username,
					(int) strlen(share),
					share,
					(int) strlen(fname),
					fname,
					(int) strlen(domain),
					domain,
					(int) strlen(montimestamp),
					montimestamp);
				if (file == NULL) talloc_free(fname);
				if (tres == NULL) { // could'nd allocate
						talloc_free(op_id_str);
						return;
				}
				((struct monitor_local_data_log *) entry->local_data)->log =
					strdup(tres);
				DEBUG(1) syslog(LOG_DEBUG, "monitor_list_update: MONITOR_LOG:"
					"created infostring >%s<",
					((struct monitor_local_data_log *)
						entry->local_data)->log);
				talloc_free(op_id_str);
				monitor_send_result(entry);
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
		/**
		 * MONITOR_ADD doesn't need to do any preprocessing
		 */
		entry->state = MONITOR_PROCESS;
		break;
	case MONITOR_TOTAL: ;
		struct monitor_local_data_total
			*data = entry->local_data;
		char len[10];
		char sum1[100];
		char len2[10];
		char sum2[100];
		strncpy(len,res,4);
		strncpy(sum1,(res+4),atoi(len));
		strncpy(len2,(res+4+atoi(len)),4);
		strncpy(sum2,(res+4+atoi(len)+4),atoi(len2));
		
		data->sum = atoll( sum1 )+ atoll(sum2);
		DEBUG(1) syslog(LOG_DEBUG, "monitor_list_set_init_result:"
			" input: %s",res);
		entry->state = MONITOR_PROCESS;
		DEBUG(1) syslog(LOG_DEBUG, "monitor_list_set_init_result:"
			" total sum set to %lu",data->sum);
		break;
	case MONITOR_LOG: ;
		entry->state = MONITOR_PROCESS;
		break;
	default: ;
		break;
	}
}

		

void monitor_list_process(int sock) {
	struct monitor_item *entry = monlist_start;
	char *data = NULL;
	if (monlist_start==NULL) return;

	while (entry != NULL) {

		switch(entry->state) {
		case MONITOR_IDENTIFY: ;
			/* Identification: send the id to the client */
			data = talloc_asprintf(NULL,"0010%010i",entry->id);
			network_send_data(data,entry->sock,strlen(data));
			talloc_free(data);
			DEBUG(1) syslog(LOG_DEBUG,"monitor_list_process: identification done.");
			entry->state = MONITOR_INITIALIZE;
			break;
		case MONITOR_INITIALIZE: ;
			/* do everything required to correctly run the */
			/* monitor. */
			monitor_list_parse( entry );
			break;
		case MONITOR_INITIALIZE_INIT: ;
			monitor_initialize( entry );
			break;
		case MONITOR_INITIALIZE_INIT_PREP: ;
			break;
		case MONITOR_PROCESS: ;
			/* process the monitor, create a result and send */
			/* the result. This is done when receiving data */
			/* except for timer based monitors, which are */
			/* processed here */
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

	}

}
