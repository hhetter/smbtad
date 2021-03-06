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

#include <talloc.h>

#define PROTOCOL_SUBRELEASE 0
#define SMBTAD_COMMON_DATA_BLOCKS 6


enum header_states {
	HEADER_CHECK_OK,
	HEADER_CHECK_FAIL,
	HEADER_CHECK_NULL,
	HEADER_CHECK_INCOMPLETE,
	HEADER_CHECK_VERSION_MISMATCH,
};

enum header_states protocol_check_header (char *header);

/**
 * The following functions assume the header is correct
 *
 */
int protocol_get_data_block_length( char *header );
int protocol_get_subversion( char *header );
int protocol_is_anonymized( char *header );
int protocol_is_encrypted( char *header );
char *protocol_get_single_data_block( TALLOC_CTX *ctx, char **go );
char *protocol_get_single_data_block_quoted( TALLOC_CTX *ctx, char **go );
char *protocol_decrypt( TALLOC_CTX *ctx, char *body, int len, const unsigned char *thekey);
char *protocol_encrypt( TALLOC_CTX *ctx,
        const char *akey, const char *str, size_t *len);
int protocol_common_blocks( char *data );
