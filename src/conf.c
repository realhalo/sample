/* [sampled] conf.c :: configuration handling for sampled. (/etc/sample.conf)
** Copyright (C) 2007 fakehalo [v9@fakehalo.us]
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**/

#include "sample.h"

/* globals. */
struct config_struct sampled_config;


/* reads and processes a file of sampled config options. */
unsigned char sampled_read_config(char *conf_file) {
	unsigned char v;
	char buf[SAM_BUFSIZE_LARGE + 1], *ptr;
	FILE *fp;

	/* don't worry if it doesn't open, still have defaults to fall back on. */
	if((fp = fopen(conf_file, "r")) == NULL) return(SAM_RETURN_FAIL);

	memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
	while(fgets(buf, SAM_BUFSIZE_LARGE, fp) != NULL) {
		if(buf[0] == '#' || buf[0] == ';' || buf[0] == '\r' || buf[0] =='\n') continue;

		sample_chomp(buf);

		if(!buf[0]) continue;

		/* "option=something" style line entry, a specified value. */
		if((ptr = strchr(buf, '=')) && buf != ptr && *(ptr + 1)) {
			*ptr++ = 0;
			if(!strcmp(buf, "interval") && !sampled_valid_interval(atoi(ptr))) sampled_config.interval = atoi(ptr);
			else if(!strcmp(buf, "daemon_priority") && !sampled_valid_priority(atoi(ptr))) sampled_config.daemon_priority = atoi(ptr);
			else if(!strcmp(buf, "user_priority") && !sampled_valid_priority(atoi(ptr))) sampled_config.user_priority = atoi(ptr);
			else if(!strcmp(buf, "log_file")) sampled_set_logfile(ptr, strlen(ptr));
			else if(!strcmp(buf, "pid_file")) sampled_set_pidfile(ptr, strlen(ptr));
#ifdef HAVE_SETRLIMIT
#ifdef RLIMIT_CPU
			else if(!strcmp(buf, "limit_cpu")) sampled_config.limit.cpu = sampled_limit_value(ptr);
#endif
#ifdef RLIMIT_STACK
			else if(!strcmp(buf, "limit_stack")) sampled_config.limit.stack = sampled_limit_value(ptr);
#endif
#ifdef RLIMIT_DATA
			else if(!strcmp(buf, "limit_data")) sampled_config.limit.data = sampled_limit_value(ptr);
#endif
#ifdef RLIMIT_NOFILE
			else if(!strcmp(buf, "limit_files")) sampled_config.limit.files = sampled_limit_value(ptr);
#endif
#ifdef RLIMIT_NPROC
			else if(!strcmp(buf, "limit_processes")) sampled_config.limit.processes = sampled_limit_value(ptr);
#endif
#ifdef RLIMIT_FSIZE
			else if(!strcmp(buf, "limit_filesize")) sampled_config.limit.filesize = sampled_limit_value(ptr);
#endif
#ifdef RLIMIT_RSS
			else if(!strcmp(buf, "limit_rss")) sampled_config.limit.rss = sampled_limit_value(ptr);
#endif
#endif /* HAVE_SETRLIMIT */

			/* if more than one is set, the 2nd will be ignored. */
			else if(!strcmp(buf, "users_allow")) sampled_users_makelist(ptr, SAM_USERMODE_ALLOW);
			else if(!strcmp(buf, "users_deny")) sampled_users_makelist(ptr, SAM_USERMODE_DENY);

			/* spooling options. (samples) */
			else if(!strcmp(buf, "spool_maxfiles")) sampled_config.spool_maxfiles = atoi(ptr);
			else if(!strcmp(buf, "spool_ttl_default")) sampled_config.spool_ttl_default = sample_parse_time(ptr, strlen(ptr));
			else if(!strcmp(buf, "spool_ttl_max")) sampled_config.spool_ttl_max = sample_parse_time(ptr, strlen(ptr));
		}

		/* "option" style line entry, a "turn on" value. */
		else if((buf[0] == '+' || buf[0] == '-') && buf[1]) {
			if(buf[0] == '+') v = SAM_TRUE;
			else v = SAM_FALSE;
			if(!strcmp(buf+1, "allow_setshell")) sampled_config.allow_setshell = v;
#ifdef HAVE_ETC_SHELLS
			else if(!strcmp(buf+1, "allow_noshell")) sampled_config.allow_noshell = v;
#endif
			else if(!strcmp(buf+1, "allow_multiple_processes")) sampled_config.allow_multiple_processes = v;
			else if(!strcmp(buf+1, "allow_multiple_user_processes")) sampled_config.allow_multiple_user_processes = v;
			else if(!strcmp(buf+1, "disable_tabs")) sampled_config.disable_tabs = v;
			else if(!strcmp(buf+1, "disable_sample")) sampled_config.disable_sample = v;
			else if(!strcmp(buf+1, "disable_spools")) sampled_config.disable_spools = v;
		}

		memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
	}

	memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
	fclose(fp);
	return(SAM_RETURN_SUCCESS);
}

/* valid sampled minute interval? */
unsigned char sampled_valid_interval(signed int v) {
	if(v >= 1 && v <= 1440) return(SAM_RETURN_SUCCESS);
	return(SAM_RETURN_FAIL);
}

/* valid sampled process priority? */
unsigned char sampled_valid_priority(signed int v) {
	if(v >= -20 && v <= 20) return(SAM_RETURN_SUCCESS);
	return(SAM_RETURN_FAIL);
}

/* free/set the log file to use. */
void sampled_set_logfile(char *file, unsigned int flen) {
	if(sampled_config.log_file) free(sampled_config.log_file);
	if(!(sampled_config.log_file = (char *)malloc(flen + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for log file path.");
	memset(sampled_config.log_file, 0, flen + 1);
	strcpy(sampled_config.log_file, file);
	return;
}

/* free/set the pid file to save to. */
void sampled_set_pidfile(char *file, unsigned int flen) {
	if(sampled_config.pid_file) free(sampled_config.pid_file);
	if(!(sampled_config.pid_file = (char *)malloc(flen + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for pid file path.");
	memset(sampled_config.pid_file, 0, flen + 1);
	strcpy(sampled_config.pid_file, file);
	return;
}
