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
struct query_entry *query_start = NULL;
struct query_entry *query_end = NULL;


pthread_mutex_t query_mutex;


/*
 * For query_list, we don't use talloc. We
 * need to remove single elements from the list,
 * making the use of malloc easier in this case
 */



/*
 * init the cache system */
void query_init( ) {
        pthread_mutex_init(&query_mutex, NULL);
}



/*
 * runs a query from the query list or returns -1 in int *body_lentgh
 * should no query be waiting.
 */
char *query_list_run_query( sqlite3 *database, int *body_length, int *sock) {

	sqlite3_stmt *stmt = NULL;
	int o, columns, rowcount, colcount=0;
	const char *zErrmsg = NULL;
	char *z;
	char *FullAlloc;

	/* To transfer the number of rows
	 * we leave space with the allocation
	 * and send the number of rows as the very
	 * first item.
	 */	
	char *RowString = malloc(sizeof(char) * strlen("0006000000"));;
	unsigned int FullLength=strlen(RowString)+1;


	if ( query_start == NULL) {
		*body_length = -1;
		return NULL;
	}
	struct query_entry *backup;
	backup = query_start;
	pthread_mutex_lock(&query_mutex);
	query_start = query_start->next;
	pthread_mutex_unlock(&query_mutex);
	/* we fetched the first item, run a SQL query on it */
	o = sqlite3_prepare(database,
		backup->data,
		strlen(backup->data),
		&stmt,
		&zErrmsg);			
	columns = sqlite3_column_count( stmt );
	FullAlloc = (char *) malloc(sizeof(char));
	while(sqlite3_step(stmt) == SQLITE_ROW) {
		while (colcount < columns) {
			z=(char *) sqlite3_column_text(stmt, colcount);
			if (z == NULL) break;
			FullAlloc = (char *) realloc(FullAlloc, sizeof(char) *
				(FullLength + strlen(z) + strlen("0000") + 2));
			char lenstr[5];
			sprintf(lenstr,"%04i", (int) strlen(z));
			memcpy(FullAlloc+FullLength, lenstr, 4);
                        int x = 0;
                        while (x <= strlen(z)) {
                                FullAlloc[FullLength+x+ 4] = z[x];
                                x++;
                        }
			FullLength=FullLength + strlen(z) + 1 +4;
		colcount = colcount + 1;
		}			
	colcount = 0;
	rowcount ++;
	}
	sqlite3_finalize( stmt );
	*sock = backup->sock;
	free(backup->data);
	free(backup);
	sprintf(RowString,"0006%06i",rowcount);
	memcpy(FullAlloc,RowString,10);
	/* When the result was NULL, we return an identifier */
	if (FullLength == 0) {
		char *str = strdup("No Results.");
		char *strg = malloc(sizeof(char)*100);
		sprintf(strg, "%04i%s",(int) strlen(str),str);
		*body_length= 4+ strlen(str)+1;
		free(FullAlloc);
		return(strg);
	}

	*body_length = FullLength;
	return FullAlloc;
}


/*
 * adds an entry to the querylist
 * returns -1 in case of an error
 */
int query_add( char *data, int len, int sock ) {

	
        struct query_entry *entry;	
	pthread_mutex_lock(&query_mutex);
	if (query_start == NULL) {
		query_start = (struct query_entry *) malloc( sizeof( struct query_entry));
		entry = query_start;
		entry->data = strdup(data);
		entry->length = len;
		query_start = entry;
		entry->next = NULL;
		query_end = entry;
		entry->sock = sock;
		pthread_mutex_unlock(&query_mutex);
		return 0;
	}
	entry = (struct query_entry *) malloc(sizeof(struct query_entry));
	query_end->next = entry;
	entry->next = NULL;
	entry->data = strdup(data);
	entry->length = len;
	query_end = entry;
	entry->sock = sock;
	pthread_mutex_unlock(&query_mutex);
	return 0;
}


