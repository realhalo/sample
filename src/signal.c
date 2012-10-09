/* [sampled] signal.c :: signal handling for sampled.
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
extern pid_t child_proc;
extern struct wpid_struct wpids;
extern struct config_struct sampled_config;

/* get a string value for a signal integer. */
char *sampled_signal_value(signed int sig) {
	switch(sig) {
		case 1: return("SIGHUP");
		case 2: return("SIGINT");
		case 3: return("SIGQUIT");
		case 4: return("SIGILL");
		case 5: return("SIGTRAP");
		case 6: return("SIGABRT");
		case 7: return("SIGBUS");
		case 8: return("SIGFPE");
		case 9: return("SIGKILL");
		case 10: return("SIGUSR1");
		case 11: return("SIGSEGV");
		case 12: return("SIGUSR2");
		case 13: return("SIGPIPE");
		case 14: return("SIGALRM");
		case 15: return("SIGTERM");
		case 16: return("SIGSTKFLT");
		case 17: return("SIGCHLD");
		case 18: return("SIGCONT");
		case 19: return("SIGSTOP");
		case 20: return("SIGTSTP");
		case 21: return("SIGTTIN");
		case 22: return("SIGTTOU");
		case 23: return("SIGURG");
		case 24: return("SIGXCPU");
		case 25: return("SIGXFSZ");
		case 26: return("SIGVTALRM");
		case 27: return("SIGPROF");
		case 28: return("SIGWINCH");
		case 29: return("SIGIO");
		case 30: return("SIGPWR");
		case 31: return("SIGSYS");
		case 32: return("SIGRTMIN");
		default: return("SIG???");
	}
}

/* handles all defined signals. */
void sampled_signal_handler(signed int sig) {
	unsigned int i = 0;

	/* only the root daemon process processes this; kills any children reading/processing ~/.sample files. */
	/* SIGHUP clean's up processors, SIGUSR1 does the same but exits after exit. */
	if(sig == SIGHUP || sig == SIGUSR1) {

		/* turn off temporarily, in-case of multiples. */
		signal(SIGHUP, SIG_IGN);
		signal(SIGUSR1, SIG_IGN);

		if(sig == SIGHUP)
			sample_error(SAM_WRN, "Received SIGHUP signal to the root daemon, killing sampled user processors. (if any)");
		else
			sample_error(SAM_WRN, "Received SIGUSR1 signal to the root daemon, cleanly exiting.");

		for(i = 0; i < wpids.size; i++) {
			if(wpids.entries[i]->pid) {

				/* sampled_sleep() will pick up the defunct process. */
				kill(wpids.entries[i]->pid, SIGHUP);
			}
		}

		/* cleaned up, now exit. */
		if(sig == SIGUSR1) {
			sample_error(SAM_REG, "--- sampled daemon exited: %u ---\n", getpid());

			/* might as well, we're done. */
			unlink(sampled_config.pid_file);

			exit(SAM_EXIT_SUCCESS);
		}

		/* ...and back, if we're still alive. */
		signal(SIGHUP, sampled_signal_handler);
		signal(SIGUSR1, sampled_signal_handler);
	}

	/* remove the cache, and reload. */
	else if(sig == SIGUSR2) {
		signal(SIGUSR2, SIG_IGN);
		sample_error(SAM_WRN, "Received SIGUSR2 signal to the root daemon, removing user cache. (if any)");

		/* if no cache exists, it will return. */
		sampled_cache_free();

		signal(SIGUSR2, sampled_signal_handler);
	}

	/* memory fault related. */
	else if(sig == SIGSEGV || sig == SIGILL || sig == SIGBUS) {
		sample_error(SAM_ERR, "Sampled memory has been corrupted. (uid=%u/gid=%u)", getuid(), getgid());

		/* doesn't make it here. */
		exit(SAM_EXIT_ERROR);
	}

	return;
}

/* handles all defined processor (child) signals. */
void sampled_signal_child_handler(signed int sig) {

	/* if the root daemon has */
	if(sig == SIGHUP) {
		if (child_proc) {
			kill(child_proc, SIGKILL);
			child_proc = 0;
		}

		exit(SAM_EXIT_SUCCESS);
	}
}
