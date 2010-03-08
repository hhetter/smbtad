struct cache_entry *protocol_parse_data_block( char *data_pointer )
{
        char *go_through = data_pointer;
        int i;
        struct cache_entry *cache_en= malloc(sizeof(struct cache_entry));

        /* first check how many common data blocks will come */
        int common_blocks_num = atoi(
                protocol_get_single_data_block( go_through ));

        /**
         * don't run a newer smbtad with an older VFS module
         */
        if (common_blocks_num < atoi(SMBTA_COMMON_DATA_COUNT)) {
                DEBUG(0) syslog(LOG_DEBUG, "Protocol error! Too less common data blocks!\n");
                exit(1);
        }

        /* vfs_operation_identifier */
        cache_en->vfs_op_id = atoi(
                protocol_get_single_data_block( go_through ));

        /* username */
        cache_en->username = protocol_get_single_data_block( go_through );
        /* user's SID */
        cache_en->usersid = protocol_get_single_data_block( go_through );
        /* share */
        cache_en->share = protocol_get_single_data_block( go_through );
        /* domain */
        cache_en->domain = protocol_get_single_data_block( go_through );
        /* timestamp */
        cache_en->timestamp = protocol_get_single_data_block( go_through );

        /* now check if there are more common data blocks to come */
        /* we will ignore them, if we don't handle more common data */
        /* in this version of the protocol */
        for (i = 0; i < (common_blocks_num - atoi(SMBTA_COMMON_DATA_COUNT)); i++) {
                char *dump = protocol_get_single_data_block( go_through );
                free(dump);
        }

        /* depending on the VFS identifier, we now fill the data */
        switch(cache_en->vfs_op_id) {
        case vfs_id_pread:
        case vfs_id_pwrite:
        case vfs_id_read:
        case vfs_id_write: ;
                struct rw_data *data = malloc(sizeof(struct rw_data));
                data->filename = protocol_get_single_data_block( go_through );
                data->len = atoi(protocol_get_single_data_block( go_through ));
                cache_en->data = data;
                return cache_en;
                break;
        }

        /* oops, this should never be reached
         * return NULL in error.
         */
        return NULL;
}


