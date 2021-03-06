/* 
 * smbtad 
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

void help_show_help()
{
	printf("SMB Traffic Analyzer daemon version %s\n", STAD2_VERSION);
	printf("%s\n", SMBTA_LICENSE);
	printf("\n");
	printf("(C) 2008-2011 Holger Hetterich <ozzy@metal-district.de>	\n");
	printf("\n");
	printf("-S	--dbuser		Specifiy the user for the database.\n");
	printf("-H 	--dbhost		Specifiy the host of the database.\n");
	printf("-P	--dbpassword		Specifiy to password to access the db.\n");
	printf("-M	--dbdriver		Specify the libDBI driver to use.\n");
	printf("-N	--dbname		Specify the name of the database.\n");
	printf("-i	--inet-port		Specifiy the port to be used.	\n");
	printf("				Default: 3490.			\n");
	printf("-u	--unix-domain-socket	If this parameter is specified, \n");
	printf("				a unix domain socket at		\n");
	printf("				/var/tmp/stadsocket will be	\n");
	printf("				used.				\n");
	printf("-d      --debug-level		Specify the debug level (0-10).	\n");
	printf("				Default: 0.			\n");
	printf("-q	--query-port		Port to be used for clients.	\n");
	printf("-o	--interactive		Don't run as daemon.		\n");
	printf("				(Runs as daemon by default)	\n");
	printf("-c      --config-file		Use configuration file given.	\n");
	printf("-t --maintenance-timer <value>  specify the time intervall to\n");
	printf("                                to start the database \n");
	printf("                                maintenance routine. Format is\n");
	printf("                                HH:MM:SS\n");
	printf("                                Example: -m 00:30:00 will run\n");
	printf("                                the maintenance routine every\n");
	printf("                                half hour.\n");
	printf("                                Default: 01:00:00\n");
	printf("-m --maintenance-timer-config \n");
	printf("     <value>   			specify a number of days\n");
	printf("                                and a time. Every database\n");
	printf("                                entry which is older than the\n");
	printf("                                the specified number of days\n");
	printf("                                will be deleted by the\n");
	printf("                                maintenance routine.\n");
	printf("                                Format is: DAYS, HH:MM:SS\n");
	printf("                                Default: 1,00:00:00\n");
	printf("-k --keyfile			Keyfile for encryption to be used\n");
	printf("				between module and smbtad.\n");
	printf("-p --precision			Precision value for the build-in\n");
	printf("				cache. Default is 5.\n");
	printf("-U --use-db			Specify 0 or 1 as argument. If\n");
	printf("				this is 0, no database handling\n");
	printf("				will be done. Default is 1.\n");
	printf("-T --setup			Do the initial database setup and exit.\n");
	printf("-I --ip				Specify the ip-address smbtad should listen on.\n");


}
