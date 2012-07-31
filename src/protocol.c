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
 */
enum header_states protocol_check_header( char *header )
{
	enum header_states status;

	if (strlen(header) < 26) {
		DEBUG(1) syslog(LOG_DEBUG,
			"protocol_check_header: received header is only %u"
			"bytes long. Assuming we haven't received it "
			"completely.\n", (unsigned int) strlen(header));
		status = HEADER_CHECK_INCOMPLETE;
		return status;
	}
	/* exit the process if we are about to receive 0 bytes */
	if ( protocol_get_data_block_length(header) == 0) {
		syslog(LOG_DEBUG, "ERROR: 0 bytes of data are about to"
				"be received. closing connection.");
		status = HEADER_CHECK_NULL;
		return status;
	}
	/* exit the process if we don't receive a V2 header */	
	if (strncmp( header, "V2.", 3) != 0) {
		syslog(LOG_DEBUG, "ERROR: Not a V2 protocol header! "
				"smbtad is exiting. closing connection.");
		status = HEADER_CHECK_FAIL;
		return status;
	}

	if ( protocol_get_subversion(header) != PROTOCOL_SUBRELEASE ) {
		DEBUG(1) syslog(LOG_DEBUG,
			"protocol_check_header: subrelease mismatch !\n ");
		status = HEADER_CHECK_VERSION_MISMATCH;
		return status;
	}

	return HEADER_CHECK_OK;
}


/**
 * return the number of common blocks to come
 */
int protocol_common_blocks( char *data )
{
	/* the very first data block tells the number of blocks */
	/* to come */
	char *str;
	str = protocol_get_single_data_block( data, &data);
	int common_blocks_num = atoi(str);
	return common_blocks_num;
}


/**
 * AES decrypt a data block.
 * returns the encrypted data block, and frees the given block
 */
char *protocol_decrypt( TALLOC_CTX *ctx, char *body, int len, const unsigned char *thekey)
{
	AES_KEY key;
	int i;
	if (thekey == NULL) return NULL;
	AES_set_decrypt_key(thekey, 128, &key);
	char *data = talloc_array( ctx, char, len+4); // malloc(sizeof( char ) * (len +4));
	int s1 = ( len / 16 );
	for ( i = 0; i < s1; i++) {
		AES_decrypt((unsigned char *) body + (i*16), (unsigned char *) data+(i*16), &key);
	}
	free(body);
	data[len+1]= '\0';
	return data;
}

/**
 * Encryption of a data block with AES
 * TALLOC_CTX *ctx      Talloc context to work on
 * const char *akey     128bit key for the encryption
 * const char *str      Data buffer to encrypt, \0 terminated
 * int *len             Will be set to the length of the
 *                      resulting data block
 * The caller has to take care for the memory
 * allocated on the context.
 */
char *protocol_encrypt( TALLOC_CTX *ctx,
        const char *akey, const char *str, size_t *len)
{
        int s1,s2,h,d;
        AES_KEY key;
        unsigned char filler[17]= "................";
        char *output;
        unsigned char crypted[18];
        if (akey == NULL) return NULL;
        AES_set_encrypt_key((unsigned char *) akey, 128, &key);
        s1 = strlen(str) / 16;
        s2 = strlen(str) % 16;
        for (h = 0; h < s2; h++) *(filler+h)=*(str+(s1*16)+h);
        DEBUG(9) syslog(LOG_DEBUG, "smb_traffic_analyzer_send_data_socket: created %s"
                " as filling block.\n", filler);
        output = talloc_array(ctx, char, (s1*16)+17 );
        for (h = 0; h < s1; h++) {
                AES_encrypt((unsigned char *) str+(16*h), crypted, &key);
                for (d = 0; d<16; d++) output[d+(16*h)]=crypted[d];
        }
        AES_encrypt( (unsigned char *) str+(16*h), filler, &key );
        for (d = 0;d < 16; d++) output[d+(16*h)]=*(filler+d);
        *len = (s1*16)+16;
        return output;
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
 * The talloc context is included for compatibility reasons, older
 * versions of this function created a newly allocated string
 */
char *protocol_get_single_data_block( TALLOC_CTX *ctx, char **data_pointer )
{
	/* get the length of the coming block first */
	char backup = *(*data_pointer + 4);
	char *toreturn = *data_pointer + 4;
	*(*data_pointer + 4) = '\0';
	int l = atoi(*data_pointer+1);
	DEBUG(9) syslog(LOG_DEBUG,
		"protocol_get_single_data_block: Length: %i",l);
	/* now recreate string */
	*(*data_pointer +4) = backup;
	*(*data_pointer + 4 + l) = '\0';
	*data_pointer=*data_pointer + l + 4;
	DEBUG(9) syslog(LOG_DEBUG,
		"protocol_get_single_data_block: Returning >%s<",toreturn);
	return toreturn;
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

