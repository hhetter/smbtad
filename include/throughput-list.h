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

struct throughput_item {
	unsigned long int value;
	time_t timestamp;
	struct throughput_item *next;
};


struct throughput_list_base {
	struct throughput_item *begin;
	struct throughput_item *end;
};	

int throughput_list_add( struct throughput_list_base *list,
	unsigned long int value,
	char *timestr);


unsigned long int throughput_list_throughput_per_second(
	struct throughput_list_base *list);

void throughput_list_free( struct throughput_list_base *list);


