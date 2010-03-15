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
 * protcol_check_header 
 * returns HEADER_CHECK_OK 			if the check is OK
 * returns HEADER_CHECK_INCOMPLETE 		if the check reveals that
 *						the header is not yet
 * 						complete.
 * returns HEADER_CHECK_VERSION_MISMATCH	if the header shows a
 *						different sub-release of
 *						the protocol
 *
 * the function exits the process if it doesn't see V2 at the beginning
 * of the header
 */
enum header_states protocol_check_header( char *header )
{
	enum header_states status;

	if (strlen(header) < 26) {
		DEBUG(1) syslog(LOG_DEBUG,
			"protocol_check_header: received header is only %i"
			"bytes long. Assuming we haven't received it "
			"completely.\n", strlen(header));
		status = HEADER_CHECK_INCOMPLETE;
		return status;
	}
	/* exit the process if we are about to receive 0 bytes */
	if ( protocol_get_data_block_length(header) == 0) {
		syslog(LOG_DEBUG, "ERROR: 0 bytes of data are about to"
				"be received. smbta is exiting.");
		exit (1);
	}
	/* exit the process if we don't receive a V2 header */	
	if (strncmp( header, "V2.", 3) != 0) {
		syslog(LOG_DEBUG, "ERROR: Not a V2 protocol header! "
				"smbtad is exiting.");
		exit (1);
	}

	if ( protocol_get_subversion(header) != PROTOCOL_SUBRELEASE ) {
		DEBUG(1) syslog(LOG_DEBUG,
			"protocol_check_header: we have subrelease number "
			"%i, the client has %i.\n");
		status = HEADER_CHECK_VERSION_MISMATCH;
		return status;
	}

	return HEADER_CHECK_OK;
}

/**
 * AES decrypt a data block.
 * returns the encrypted data block, and frees the given block
 */
char *protocol_decrypt( char *body, int len, const char *thekey)
{
	AES_KEY key;
	int i,y;
	if (thekey == NULL) return NULL;
	AES_set_decrypt_key(thekey, 128, &key);
	char *data = malloc(sizeof( char ) * (len +4));
	int s1 = ( len / 16 );
	for ( i = 0; i < s1; i++) {
		AES_decrypt(body + (i*16), data+(i*16), &key);
	}
	free(body);
	data[len+1]= '\0';
	syslog(LOG_DEBUG,"DATA DEC: %s\n",data);
	return data;
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
 * Parse a single data block.
 * char *data_pointer	This char shall point to the begin of the data block,
 *			it is set by the function to the beginning of the
 *			next data block.
 * The function returns a data block. The memory must be freed by the caller.
 */
char *protocol_get_single_data_block( char **data_pointer )
{
	char length[6];
	char *data;
	int l;
	memcpy( length, *data_pointer, 4);
	length[4] = '\0';
	l = atoi(length);
	data = malloc(sizeof(char) * (l+1));
	memcpy( data, *data_pointer + 4, l);
	data[ l ] = '\0';
        *data_pointer = *data_pointer + l + 4;
	return data;
}

/**
 * Parse a single data block and return a quoted string.
 * char *data_pointer   This char shall point to the begin of the data block,
 *                      it is set by the function to the beginning of the
 *                      next data block.
 * The function returns a data block. The memory must be freed by the caller.
 */
char *protocol_get_single_data_block_quoted( char **data_pointer )
{
        char length[6];
        char *data;
        int l;
        memcpy( length, *data_pointer, 4);
        length[4] = '\0';
        l = atoi(length);
        data = malloc(sizeof(char) * (l+4));
	data[0] = '\"';
        memcpy( data+1, *data_pointer + 4, l);
        data[ l+1] = '\"';
	data[ l+2] = '\0';
        *data_pointer = *data_pointer + l + 4;
        return data;
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
 * Return 1 if the data block is encrypted.
 */
int protocol_is_encrypted( char *header )
{
	if ( *(header+5)=='E' ) return 1; else return 0;
}

