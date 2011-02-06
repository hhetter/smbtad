/* 
 * stad 
 * capture transfer data from the vfs_smb_traffic_analyzer module, and store
 * the data via various plugins
 *
 * Copyright (C) Holger Hetterich, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

struct cache_entry {
	char *data;
	int length;
	struct cache_entry *left;
	struct cache_entry *right;
	struct cache_entry *down;
	/* only relevant in the cache_start */
	/* vfs ops other than read/write will be */
	/* placed here */
	struct cache_entry *other_ops;


	char *username;
	char *domain;
	char *share;
	char *timestamp;
	char *usersid;
	char *filename;
	char *mode;
	char *path;
	char *result;
	char *destination;
	char *mondata;
	unsigned long int len;
	char *vfs_id;
	char *source;
	int op_id;
};

void cache_init();
int cache_add( char *data, int len );
void cache_manager( struct configuration_data *config);
void cache_query_thread(struct configuration_data *config);
