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

#include "../include/includes.h"


/**
 * protcol_check_header is a void function. It will exit the whole
 * process, as if there is not a correct header found, we may have
 * a man in the middle attack.
 */
void protocol_check_header( char *header )
{
	if (strncmp( header, "V2.", 3) != 0) {
		syslog(LOG_DEBUG, "ERROR: Not a V2 protocol header!");
		exit (1);
	}
}


/**
 * Return the length of the data block to come given a header
 * as input.
 */
int protocol_get_data_block_length( char *header )
{
	int retval;
	retval = atoi( header+11 );
	return retval;
}


/**
 * Return the sub-release number of the protocol in the V2
 * familiy used from the VFS module.
 */
int protocol_get_subversion( char *header )
{
	int retval;
	char conv[4];
	conv[0] = header[3];
	conv[1] = '\0';
	retval = atoi(conv);
	return retval;
}

/**
 * Return 1 if the data is anonymized.
 */
int protocol_is_anonymized( char *header )
{
	if ( *(header+4)=='A' ) return 1; else return 0;
}


/**
 * Return 1 if the data block is encrypted.
 */
int protocol_is_encrypted( char *header )
{
	if ( *(header+5)=='E' ) return 1; else return 0;
}

