/* [sampled] limit.c :: set resource limits for sampled and sampled processors.
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

/* externs. */
extern struct config_struct sampled_config;


#ifdef HAVE_SETRLIMIT

/* returns a rlimit value with parsed "infinity" */ 
rlim_t sampled_limit_value(char *rl) {
	if(!rl || !*rl) return((rlim_t)0);
	if(!strcmp(rl, "unlimited") || !strcmp(rl, "infinity")) return(RLIM_INFINITY);
	else return((rlim_t)atoi(rl));
}

/* returns a cur=max'd rlimit structure. */ 
struct rlimit sampled_limit_max(rlim_t rm) {
	struct rlimit ru;
	ru.rlim_cur = rm;
	ru.rlim_max = rm;
	return(ru);
}

/* stop potential core files that could appear just to take up space. */
#ifdef RLIMIT_CORE
void sampled_limit_no_core() {
	struct rlimit ru;
	ru = sampled_limit_max(0);
	setrlimit(RLIMIT_CORE, &ru);
}
#endif

/* makes all resources it can = unlimited, if RLIM_NLIMITS is supported. (this is currently unused) */ 
#ifdef RLIM_NLIMITS 
void sampled_limit_unlimit() {
	signed int r = 0;
	struct rlimit ru;
	ru = sampled_limit_max(RLIM_INFINITY);
	for(r = 0; r < RLIM_NLIMITS; r++)
		setrlimit(r, &ru);	
	return;
}
#endif

/* (attempt to) set config values to the active process. */
void sampled_limit() {
	struct rlimit ru;

	/* linux only (i think) resource, set it if it has it. */
#ifdef RLIMIT_NICE
	ru.rlim_cur = ru.rlim_max = (rlim_t)(20 - sampled_config.user_priority);
	setrlimit(RLIMIT_NICE, &ru);
#endif

	/* now for the /etc/sample.conf defines, if supported. */
#ifdef RLIMIT_CPU
	if(sampled_config.limit.cpu) {
		ru = sampled_limit_max(sampled_config.limit.cpu);
		setrlimit(RLIMIT_CPU, &ru);
	}
#endif
#ifdef RLIMIT_STACK
	if(sampled_config.limit.stack) {
		ru = sampled_limit_max(sampled_config.limit.stack);
		setrlimit(RLIMIT_STACK, &ru);
	}
#endif
#ifdef RLIMIT_DATA
	if(sampled_config.limit.data) {
		ru = sampled_limit_max(sampled_config.limit.data);
		setrlimit(RLIMIT_DATA, &ru);
	}
#endif
#ifdef RLIMIT_NOFILE
	if(sampled_config.limit.files) {
		ru = sampled_limit_max(sampled_config.limit.files);
		setrlimit(RLIMIT_NOFILE, &ru);
	}
#endif
#ifdef RLIMIT_NPROC
	if(sampled_config.limit.processes) {
		ru = sampled_limit_max(sampled_config.limit.processes);
		setrlimit(RLIMIT_NPROC, &ru);
	}
#endif
#ifdef RLIMIT_RSS
	if(sampled_config.limit.rss) {
		ru = sampled_limit_max(sampled_config.limit.rss);
		setrlimit(RLIMIT_RSS, &ru);
	}
#endif
#ifdef RLIMIT_FSIZE
	if(sampled_config.limit.filesize) {
		ru = sampled_limit_max(sampled_config.limit.filesize);
		setrlimit(RLIMIT_FSIZE, &ru);
	}
#endif
	return;
}
#endif /* HAVE_SETRLIMIT */
