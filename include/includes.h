#include "version.h"
#define _XOPEN_SOURCE
#define _SVID_SOURCE || _BSD_SOURCE || _XOPEN_SOURCE >= 500

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <syslog.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <sqlite3.h>
#include <talloc.h>

#include "aes.h"
#include "rijndael-alg-fst.h"
#include "../iniparser3.0b/src/iniparser.h"
#include "vfs_smb_traffic_analyzer.h"
#include "configuration.h"
#include "help.h"
#include "daemon.h"
#include "log.h"
#include "connection_list.h"
#include "network.h"
#include "protocol.h"
#include "cache.h"
#include "database.h"

/**
 * Debug Levels:
 * 0 -> only fatal errors which lead to terminate stad
 * 1 -> protocol debug messages
 * 2 -> network debug messages
 * The global integer _DGB will be set by the configuration
 * to a value and will be queried when a debug message
 * has to be send out.
 */
#define DEBUG(x) if ( x >= _DBG ) 
int _DBG;


