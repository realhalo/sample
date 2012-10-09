/* [sampled] sampled.c :: core functions for sampled.
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

static const char id[] = "$Id: sampled,v " SAM_VERSION " " SAM_CMPTIME " fakehalo Exp $";

/* externs. */
extern char **environ;
extern char *optarg;

/* sampled externals. */
#ifndef NO_SETPROCTITLE
extern struct proctitle_struct sampled_proctitle;
#endif
extern struct config_struct sampled_config;
extern struct wpid_struct wpids;
extern struct sampled_cache_struct **sampled_cache;
extern unsigned int sampled_cache_size;
extern unsigned int sampled_env_size;
extern char **sampled_env;

/* globals. */
FILE *sampled_log;
pid_t root_proc = 0, child_proc = 0;

time_t last_passwd_mtime = 0;
#ifdef HAVE_ETC_SHELLS
time_t last_shells_mtime = 0;
#endif


/* execution start. */
int main(signed int argc, char **argv) {
	signed int chr;
	struct stat s;

	/* see if we're set*id, which shouldn't be. */
	if(sample_is_privileged())
		sample_error(SAM_ERR, "sampled appears to be set*id, which should never be.");

	/* for now just log to standard error. */
	sampled_log = stderr;

	/* handle any signals here. */
	signal(SIGHUP, sampled_signal_handler);
	signal(SIGUSR1, sampled_signal_handler);
	signal(SIGUSR2, sampled_signal_handler);
	signal(SIGSEGV, sampled_signal_handler);
	signal(SIGILL, sampled_signal_handler);
	signal(SIGBUS, sampled_signal_handler);

	/* this can happen with limit_cpu or in general, nothing is read from it as it goes by the hard limit to kill it. */
#ifdef SIGXCPU
	signal(SIGXCPU, SIG_IGN);
#endif

	/* no need for core files laying around the system even if sampled does fault. */
	/* if sampled did fault, it would show up in sample.log. */
#ifdef RLIMIT_CORE
	sampled_limit_no_core();
#endif

	/* setup default config values, explained in sample.h. */
	sampled_config.interval = SAM_DEF_INTERVAL;
#ifdef HAVE_ETC_SHELLS
	sampled_config.allow_noshell = SAM_DEF_ALLOW_NOSHELL;
#endif
	sampled_config.allow_setshell = SAM_DEF_ALLOW_SETSHELL;
	sampled_config.allow_multiple_processes = SAM_DEF_ALLOW_MULTIPLE_PROC;
	sampled_config.allow_multiple_user_processes = SAM_DEF_ALLOW_MULTIPLE_UPROC;
	sampled_config.daemon_priority = SAM_DEF_DAEMON_PRIORITY;
	sampled_config.user_priority = SAM_DEF_USER_PRIORITY;

	/* disable options. */
	sampled_config.disable_tabs = SAM_DEF_DISABLE_TABS;
	sampled_config.disable_sample = SAM_DEF_DISABLE_SAMPLE;
	sampled_config.disable_spools = SAM_DEF_DISABLE_SPOOLS;

	/* spooling defaults.  */
	sampled_config.spool_maxfiles = SAM_DEF_SPOOL_MAXFILES;
	sampled_config.spool_ttl_default = sample_parse_time(SAM_DEF_SPOOL_TTL_DEF, strlen(SAM_DEF_SPOOL_TTL_DEF));
	sampled_config.spool_ttl_max = sample_parse_time(SAM_DEF_SPOOL_TTL_MAX, strlen(SAM_DEF_SPOOL_TTL_MAX));

	/* so these won't be free()'d without being malloc()'d */
	sampled_config.log_file = 0;
	sampled_config.pid_file = 0;

	/* zero any resource limits so they won't be processed by default. */
#ifdef HAVE_SETRLIMIT
#ifdef RLIMIT_CPU
	sampled_config.limit.cpu = (rlim_t)0;
#endif
#ifdef RLIMIT_STACK
	sampled_config.limit.stack = (rlim_t)0;
#endif
#ifdef RLIMIT_DATA
	sampled_config.limit.data = (rlim_t)0;
#endif
#ifdef RLIMIT_NOFILE
	sampled_config.limit.files = (rlim_t)0;
#endif
#ifdef RLIMIT_NPROC
	sampled_config.limit.processes = (rlim_t)0;
#endif
#ifdef RLIMIT_RSS
	sampled_config.limit.rss = (rlim_t)0;
#endif
#ifdef RLIMIT_FSIZE
	sampled_config.limit.filesize = (rlim_t)0;
#endif
#endif /* HAVE_SETRLIMIT */

	/* set default log/pid files. */
	sampled_set_logfile(SAM_DEF_LOGFILE, strlen(SAM_DEF_LOGFILE));
	sampled_set_pidfile(SAM_DEF_PIDFILE, strlen(SAM_DEF_PIDFILE));

	/* see if "/etc/sample.conf" exists, and process. */
	(void)sampled_read_config(SAM_CONFFILE);

	/* grab any command-line options. */
	while((chr = getopt(argc, argv, "vc:l:d:i:p:P:h"
#ifdef HAVE_ETC_SHELLS
	"s"
#endif
	"SmM")) != EOF) {
		switch(chr) {
			case 'v': /* version. */
				printf("%s: version " SAM_VERSION "\n", argv[0]);
				exit(SAM_EXIT_SUCCESS);
				break;
			case 'c': /* alternate config file. (overrides "/etc/sample.conf" as well) */
				if(sampled_read_config(optarg))
					sample_error(SAM_ERR, "'-c' option returned an error processing: %s", optarg);
				break;
			case 'l': /* alternate log file. (overrides "/var/log/sample.log" as well) */
				sampled_set_logfile(optarg, strlen(optarg));
				break;
			case 'd': /* alternate log file. (overrides "/var/run/sampled.pid" as well) */
				sampled_set_pidfile(optarg, strlen(optarg));
				break;
			case 'i': /* intervals */
				if(!sampled_valid_interval(atoi(optarg)))
					sampled_config.interval = atoi(optarg);
				else
					sample_error(SAM_ERR, "'-i' option must be between 1 and 1440. (minutes)");
				break;
			case 'P': /* daemon priority */
				if(!sampled_valid_priority(atoi(optarg)))
					sampled_config.daemon_priority = atoi(optarg);
				else
					sample_error(SAM_ERR, "'-P' option must be between -20 and 20. (priority)");
				break;
			case 'p': /* user priority */
				if(!sampled_valid_priority(atoi(optarg)))
					sampled_config.user_priority = atoi(optarg);
				else
					sample_error(SAM_ERR, "'-p' option must be between -20 and 20. (priority)");
				break;
#ifdef HAVE_ETC_SHELLS
			case 's': /* allow users with no shell in /etc/shells to execute ~/.sample files. */
				sampled_config.allow_noshell = SAM_TRUE;
				break;
#endif
			case 'S': /* allow users to set their own shells in ~/.sample with SHELL="..." */
				sampled_config.allow_setshell = SAM_TRUE;
				break;
			case 'm': /* allow multiple processes for a single user. */
				sampled_config.allow_multiple_processes = SAM_TRUE;
				break;
			case 'M': /* allow multiple processes for a single user instance. (bad) */
				sampled_config.allow_multiple_user_processes = SAM_TRUE;
				break;
			case 'h': /* brief help. */
			default:
				printf("usage: %s [-v"
#ifdef HAVE_ETC_SHELLS
				"s"
#endif
				"SmM] [-i minutes] [-p|-P priority] [-c conf_file] [-l log_file] [-d pid_file]\n", argv[0]);
				exit(SAM_EXIT_ERROR);
				break;
		}
	}

	/* sanity check. */
	if(sampled_config.disable_tabs && sampled_config.disable_sample && sampled_config.disable_spools)
		sample_error(SAM_ERR, "Everything is disabled, no need be running then. (bailing)");

#ifndef NO_SETPROCTITLE
	/* done with args and env does not change anywheres in sampled from daemon->user fork(), so might as well do this here. */
	initsetproctitle(argc, argv, environ);
	setproctitle("Daemon");
#endif
	/* sampled must be root. */
	if(getuid()) sample_error(SAM_ERR, "Sampled must be ran as root.");
	else if(getgid() && setgid(0)) sample_error(SAM_ERR, "Sampled failed to setgid=0.");

	/* set up our log stream. (for children also) */
	/* do it before the fork() because we will just output it to stderr if it fails. */
	sampled_log = sampled_log_open();

	/* set our daemon process priority. */
	setpriority(PRIO_PROCESS, 0, sampled_config.daemon_priority);

	/* stay in the root directory if possible, for the root sampled process. */
	chdir("/");

	/* daemonize ourself. */
	switch(fork()) {
		case -1:
			sample_error(SAM_ERR, "Forking into daemon failed.");
			break;
		case 0:

			/* detach / new group leader. */
#ifdef HAVE_SETSID
			setsid();
#endif
			/* identify ourself as the root process. (0 = child) */
			root_proc = getpid();

			/* write who we are to /var/run/sampled.pid, or other specified. */
			/* if another sampled instance has locked the pid file it will exit. */
			sampled_write_pid(root_proc);

			/* write to /var/log/sample.log too, or other specified. (sample_error() will do) */
			sample_error(SAM_REG, "--- sampled daemon started: %u ---\n", root_proc);

			/* don't log if we're background and the log file still equals stderr. */
			if(sampled_log == stderr) sampled_log = 0;

			/* no standard in/out/error, this is a backgrounded daemon. */
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);

			/* will stop defunct processors, only if we are allowing multiples and will not be monitored. */
			if(sampled_config.allow_multiple_processes) signal(SIGCHLD, SIG_IGN);

			/* mark the original times to know if we need to reload the users. */
			if(!stat(SAM_PASSWD, &s)) last_passwd_mtime = s.st_mtime;
#ifdef HAVE_ETC_SHELLS
			if(!stat(SAM_SHELLS, &s)) last_shells_mtime = s.st_mtime;
#endif
			/* work forever. */
			while(SAM_TRUE) {
				sampled_pwd_cycle();
				sampled_sleep();
			}
			break;
	}

	exit(SAM_EXIT_SUCCESS);
}

/* sleep()s until a minute marker ??:??:00.  */
void sampled_sleep() {
	signed int ws, x;
	unsigned int i;
	unsigned long s;
	struct timeval tv;

	gettimeofday(&tv, NULL); 
	s = tv.tv_sec % (60 * sampled_config.interval);

	/* will break inside. */
	while(SAM_TRUE) {
		sleep(1);

		/* if allowing multiple processors, don't monitor. */
		if(!sampled_config.allow_multiple_processes) {

			/* reap child pids while we wait for another round, avoid defuncts. */
			for(i = 0; i < wpids.size; i++) {

				/* make sure to only check existent children pids. */
				/* if waitpid returns an error OR returns the pid it does not matter, do not restrict if it errors out. */
				if(wpids.entries[i]->pid && 
#ifdef HAVE_WAITPID
				waitpid(wpids.entries[i]->pid, &ws, WNOHANG)
#else
				wait4(wpids.entries[i]->pid, &ws, WNOHANG, NULL)
#endif
				) {
					/* was it signaled/terminated, if so log it. */
					if(WIFSIGNALED(ws)) {
#ifdef WTERMSIG
						x = WTERMSIG(ws);
						if(x) sample_error(SAM_WRN, "Sampled processor was terminated(%s): %u [user='%s']", sampled_signal_value(x), wpids.entries[i]->pid, wpids.entries[i]->user);
						else
#endif
							 sample_error(SAM_WRN, "Sampled processor was signaled: %u [user='%s']", wpids.entries[i]->pid, wpids.entries[i]->user);
					}
					wpids.entries[i]->pid = 0;
				}
			}
		}

		gettimeofday(&tv, NULL); 

		/* must have hit :00, break the sleep. */
		if(s > tv.tv_sec % (60 * sampled_config.interval)) break;

		s = tv.tv_sec % (60 * sampled_config.interval);
	}
	return;
}

/* executes programs. */
void sampled_exec(char *shell, char *exec) {
	signed int ws, x;
	pid_t pid;

	if(!shell || !strlen(shell) || !exec || !strlen(exec)) return;

	switch((pid = fork())) {
		case -1:
			sample_error(SAM_WRN, "fork() failed.");
			return;
			break;
		case 0:
#ifdef HAVE_SETRLIMIT
			/* these we're already set at the beginning of this processor instance, set again as it's no extra effort. */
			sampled_limit();
#endif

			/* no standard in/out/error, this is a backgrounded daemon. */
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);

			/* run it. */
			execle(shell, sample_basename(shell), "-c", exec, (char *)0, (sampled_env_size ? sampled_env : NULL));

			/* shouldn't make it here, bad shell. */
			exit(SAM_EXIT_ERROR);
			break;
		default:

			/* for SIGHUP to kill(), if we are allowing multiple processes we won't be tracking this pid, as we aren't wait()ing for it. */
			/* it will remain child_proc=0, and no kill() will be done on it. */
			if(!sampled_config.allow_multiple_user_processes) child_proc = pid;

			ws = 0;

			/* wait for a return, if it errors let it go. */
			while(!sampled_config.allow_multiple_user_processes &&
#ifdef HAVE_WAITPID
			!waitpid(pid, &ws, WNOHANG)
#else
			!wait4(pid, &ws, WNOHANG, NULL)
#endif
			)
#ifdef HAVE_USLEEP
				usleep(100000);
#else
				sleep(1);
#endif

			/* it's gone.  */
			child_proc = 0;

			/* if we're allowing multiple user processes, don't monitor -- too much (potential) overhead. */
			if(!sampled_config.allow_multiple_user_processes) {

			/* was it signaled/terminated, if so log it, and possibly exit. */
				if(WIFSIGNALED(ws)) {
#ifdef WTERMSIG
					x = WTERMSIG(ws);
					if(x) {
						sample_error(SAM_WRN, "Sampled processor sub-process was terminated(%s): %u [shell='%s', cmd='%s']", sampled_signal_value(x), pid, shell, exec);
						if(x == SIGKILL
#ifdef SIGXCPU
						|| x == SIGXCPU
#endif
						) {
							sample_error(SAM_WRN, "Sampled processor sub-process(%u) was killed%s, processor following suit: %u", pid, (
#ifdef SIGXCPU
							x == SIGXCPU ? " for excess cpu usage" : 
#endif
							""), getpid());
							exit(SAM_EXIT_SUCCESS); /* not really an error. */
						}
					}
					else
#endif
						/* not having WTERMSIG could cause some repeating cpu-usage, not desired. */
						sample_error(SAM_WRN, "Sampled processor sub-process was signaled: %u [shell='%s', cmd='%s']", pid, shell, exec);
				}
			}
			break;
	}
	return;
}

/* for users who have a ~/.sample file, it read/parse it. (dropped privs) */
unsigned char sampled_handle_user_rc(struct sampled_cache_struct *sc, char *sample_file) {
	unsigned char r, type;
	unsigned int i, j, offset, rnd, ttl, tot;
	char buf[SAM_BUFSIZE_GIANT + 1], *ptr, *tptr[4], *cmd, *path, *shell;
	glob_t g;
	time_t orig_time, now;
	struct stat s;
	FILE *fp;

	/* did it match anything? (for spool deletion, will be set SAM_TRUE if something runs) */
	r = SAM_FALSE;

	/* stat() failing on our own sample file? can't be good. */
	if(access(sample_file, R_OK) || stat(sample_file, &s)) return(r);

	/* for sample spools (~/.sample_spool/), see if the TTL has passed, return success/delete if so. */
	ptr = sample_basename(sample_file);
	if(sscanf(ptr, "%u.%u.", &rnd, &ttl) == 2) {
		now = time(0);

		/* something is wrong with the timestamps, don't delete/ignore in this case. */
		if(now > s.st_mtime) {

			/* 0 = lets sampled assume the 'default' time-to-live. */
			if(ttl == 0) ttl = sampled_config.spool_ttl_default;

			/* is the spooled sample file older than the specified ttl OR internal maximum limit? delete if so. */
			if(now - s.st_mtime > ttl || now - s.st_mtime > sampled_config.spool_ttl_max) return(SAM_TRUE);
		}
	}

	/* shell to execute with, none by default. (specified by SHELL=...) */
	shell = 0;

	if((fp = fopen(sample_file, "r")) == NULL)
		sample_error(SAM_ERR, "Failed to open sample file: %s", sample_file);
	memset(buf, 0, SAM_BUFSIZE_GIANT + 1);

	/* store the original time. (race-condition check, will stop ~/.sample processing) */
	orig_time = sampled_file_mod_diff(sample_file, 0);

	/* all matched files. */
	tot = 0;

	/* get next line and check for to see if ~/.sample changed. */
	while(fgets(buf, SAM_BUFSIZE_GIANT, fp) != NULL) {
		if(buf[0] == '#' || buf[0] == ';' || buf[0] == '\r' || buf[0] =='\n') continue;

		sample_chomp(buf);

		if(!buf[0]) continue;

		/* no tabs in the line and an '='? assume its to set an evironmental variable. */
		/* sampled_putenv() will handle invalid variables and overwrites. */
		if(!strchr(buf, '\t') && (ptr = strchr(buf, '='))) {

			/* config option: allow users to define/override their own shells using SHELL="..."?  */
			/* allow users to specify an alternate shell to execute the string. */
			if(sampled_config.allow_setshell && !strncmp("SHELL=", buf, ptr - buf + 1) && *(ptr + 1)) {
				ptr++;

				/* it is possible/allowed to reset this. */
				if(shell) free(shell);

				if(!(shell = (char *)malloc(strlen(ptr) + 1)))
					sample_error(SAM_ERR, "Failed to allocate memory for user-defined shell.");
				strcpy(shell, ptr);
			}
			 sampled_putenv(buf);
		}
		/* must be a sample time. */
		else {
			tptr[1] = tptr[2] = tptr[3] = 0;
			tptr[0] = ptr = buf;
			i = 1;
			while(*ptr) {

				/* allow consecutive tabs, makes for tidy ~/.sample files. */
				/* tptr[0] = TYPE, tptr[1] = TIME, tptr[2] = PATH, tptr[3] = CMD */
				if(*ptr == '\t') {
					*ptr = 0;
					if(*(ptr + 1) && *(ptr + 1) != '\t') {
						tptr[i] = ptr + 1;
						if(i >= 3) break;
						i++;
					}
				}
				ptr++;
			}

			/* we only need the first character to find out what type of time to check. */
			type = (unsigned char)tptr[0][0];

			/* appears to be a valid ~/.sample line entry, including the time/size value > 0, use it. */
			if(i >= 3 && tptr[0] && tptr[1] && tptr[2] && tptr[3] && (offset = (type != 's' && type != 'S' ? sample_parse_time(tptr[1], strlen(tptr[1])) : sample_parse_size(tptr[1], strlen(tptr[1])))) > 0) {

				/* make sure there is a home directory to read from. */
				if(sc->pw_dir && strlen(sc->pw_dir) > 0) {
					/* convert '~' to the users home directory. */
					path = sample_parse_home_string(tptr[2], sc->pw_dir, strlen(sc->pw_dir));

					/* glob() what we can get. */
					g = sampled_get_files(path);

					free(path);
				}
				else g = sampled_get_files(tptr[2]);

				for(j = 0; j < g.gl_pathc; j++) {

					/* don't allow filenames with non-printables.  ie. '\r' and '\n' would only exist to be evil. */
					if(sampled_bad_file(g.gl_pathv[j])) continue;

					/* can't write to the file? shouldn't be messing with it then. */
					if(access(g.gl_pathv[j], W_OK)) continue;

					/* has the allotted time passed yet? or the filesize limit hit? */
					if(sampled_file_diff(g.gl_pathv[j], offset, type)) continue;

					/* only regular files. (no directories/devices, should follow links) */
					if(!(sample_path_mode(g.gl_pathv[j]) & S_IFREG)) continue;

					/* don't process the user's (current/active) ~/.sample file if it's matched. */
					if(!strcmp(g.gl_pathv[j], sample_file)) continue;

					/* ~/.sample modification time changed since opening? race-condition situation.  this is the only important place to stop it if it occurs. */
					/* weither it be by accident or malicious it could potentially result in a problem.  'break' as all to follow will have the same result. */
					if(sampled_file_mod_diff(sample_file, orig_time)) break;

					/* make the command to run. (%s/%f/%p/etc parsing) */
					cmd = sampled_system_string_format(tptr[3], g.gl_pathv[j]);

					sampled_exec((shell ? shell : sc->pw_shell), cmd);

					free(cmd);

					/* found something. (deletes if it is in the ~/.sample_spool dir) */
					r = SAM_TRUE;
				}

				tot += g.gl_pathc;

				globfree(&g);
			}
		}
		memset(buf, 0, SAM_BUFSIZE_GIANT + 1);
	}

	/* no matched file? delete if it's a spooled sample file. */
	if(!tot) r = SAM_TRUE;

	if(shell) free(shell);
	fclose(fp);
	return(r);
}

/* main routine for handling each users ~/.sample, ~/.sample_spool/, and /etc/sampletab/<username> */
void sampled_handle_user(struct sampled_cache_struct *sc) {
	unsigned int i, j;
	pid_t pid;
	glob_t g;

	/* no (accessible) ~/.sample file or ~/.sample_spool dir? save the time of going any further. */
	/* config option: allow multiple processes per user? stop if one is still going from previous run. */
	if((access(sc->sample_file, R_OK) && access(sc->sample_spooldir, F_OK) && access(sc->sample_tab, R_OK)) || (!sampled_config.allow_multiple_processes && sampled_wpid_get(sc->pw_name))) {
		return;
	}

	/* a fork() for each user who has a ~/.sample, or the like. (user 'processor' starts here) */
	switch((pid = fork())) {
		case -1:
			sample_error(SAM_WRN, "fork() failed on user: %s", sc->pw_name);
			break;
 		case 0:
			/* only the root daemon handles these. */
			signal(SIGUSR1, SIG_IGN);
			signal(SIGUSR2, SIG_IGN);

			/* we're a child now. (sampled processor) */
			root_proc = 0;

			/* this will be filled in when a processor fork()s off to execute a command. */
			child_proc = 0;

			/* handled differently for processors; incase sampled gets "reset", kill our forked process too. */
			signal(SIGHUP, sampled_signal_child_handler);

			/* bring it back. */
#ifdef SIGXCPU
			signal(SIGXCPU, SIG_DFL);
#endif
			/* will stop defunct user processes, only if we are allowing multiples and will not be monitored. */
			if(sampled_config.allow_multiple_user_processes) signal(SIGCHLD, SIG_IGN);

			/* incase of being ignored from sampled_config.allow_multiple_processes */
			else signal(SIGCHLD, SIG_DFL);

			/* set our user process priority. */
			setpriority(PRIO_PROCESS, 0, sampled_config.user_priority);

#ifdef HAVE_SETRLIMIT
			/* set /etc/sample.conf resource limits, if any. */
			sampled_limit();
#endif

			/* switch from root to the user before even opening the ~/.sample file. */
			if(sampled_set_permissions(sc))
				sample_error(SAM_ERR, "Failed to change permissions to user: %s", sc->pw_name);

#ifndef NO_SETPROCTITLE
			/* initsetproctitle() was called in main(), this should be the same size args/env. */
			setproctitle("Processor [%s/%u]", sc->pw_name, getuid());
#endif

			/* switch to the users homedir, no big deal if it fails. */
			if(sc->pw_dir && strlen(sc->pw_dir) > 0) {
				chdir(sc->pw_dir);
				sampled_setenv("HOME", sc->pw_dir);
			}

			/* set up the environment more appropriately. */
			if(sc->pw_name && strlen(sc->pw_name) > 0) {
				sampled_setenv("USER", sc->pw_name);
				sampled_setenv("LOGNAME", sc->pw_name);
			}
			if(sc->pw_shell && strlen(sc->pw_shell) > 0) sampled_setenv("SHELL", sc->pw_shell);

			/* handle the rc file containing the info to run through. (returns from inside if it doesn't exist) */
			/* config: don't process if we're instructed to only process /etc/sampletab/ */
			if(!sampled_config.disable_sample) (void)sampled_handle_user_rc(sc, sc->sample_file);

			/* now handle the rc file containing the directories to run through. (returns from inside if it doesn't exist) */
			if(!sampled_config.disable_tabs) (void)sampled_handle_user_rc(sc, sc->sample_tab);

			/* now go through ~/.sample_spool/. (would return 0 from the glob, but save the effort)  */
			/* config: don't process if we're instructed to only process /etc/sampletab/ */
			if(!sampled_config.disable_spools && !access(sc->sample_spooldir, F_OK)) {
				g = sampled_get_files(sc->sample_spooldir);

				/* run through all the files found in the ~/.sample_spool/ directory. */
				/* config: if the number of files are larger than spool_maxfiles it will randomly pull an area of files until the spool_maxfiles limit is hit. */
				for(i = j = sampled_rand_offset(sampled_config.spool_maxfiles, g.gl_pathc); (!sampled_config.spool_maxfiles || i - j < sampled_config.spool_maxfiles) && i < g.gl_pathc; i++) {

					/* run the (spooled) sample file. */
					if(sampled_handle_user_rc(sc, g.gl_pathv[i])) {

						/* it ran something against the file(s) OR the time-to-live passed, delete the spool entry. */
						if(!access(g.gl_pathv[i], F_OK) && unlink(g.gl_pathv[i]))

							/* failed to delete, make it known. */
							sample_error(SAM_WRN, "Spooled sample file \"%s\" could not be deleted, this may process infinitely.", g.gl_pathv[i]);
					}
				}

				/* no files in the spooldir? attempt to delete the directory for unnecessary future scans. */
				/* the "samples" command will re-create the directory if needed. */
				if(!g.gl_pathc) rmdir(sc->sample_spooldir);

				globfree(&g);
			}

			exit(SAM_EXIT_SUCCESS);
			break;
		default:
			/* if allowing multiple processors, don't monitor. */
			if(!sampled_config.allow_multiple_processes)

				/* mark the child pid so we can check on it during sampled_sleep(). */
				sampled_wpid_entry(sc->pw_name, pid);
			break;
	}
	return;
}

/* run through all the /etc/passwd or getpwent() entries to check for users ~/.sample. */
/* this runs on every ??:??:00 minute marker, by default. */
void sampled_pwd_cycle() {
	unsigned int i;
	struct stat s;
#ifdef SAM_STRICT_PASSWD
	char buf[SAM_BUFSIZE_LARGE + 1], *ptr, *lptr;
	time_t orig_time;
	FILE *fp;
	struct passwd pwd;
#else
	struct passwd *pwptr;
#endif
#ifdef HAVE_ETC_SHELLS
	FILE *fps;
#endif

	/* see if /etc/passwd or /etc/shells has been modified, delete the cache if so. */
	if(sampled_cache_size) {
		if(last_passwd_mtime && !stat(SAM_PASSWD, &s) && last_passwd_mtime != s.st_mtime) {
			sampled_cache_free();
			last_passwd_mtime = s.st_mtime;
			sample_error(SAM_WRN, "\"" SAM_PASSWD "\" has been modified, removing user cache.");
		}
#ifdef HAVE_ETC_SHELLS
		else if(last_shells_mtime && !stat(SAM_SHELLS, &s) && last_shells_mtime != s.st_mtime) {
			sampled_cache_free();
			last_shells_mtime = s.st_mtime;
			sample_error(SAM_WRN, "\"" SAM_SHELLS "\" has been modified, removing user cache.");
		}
#endif
	}

	/* no cache? either first starting or the cache was free()d. */
	if(!sampled_cache_size) {
		sample_error(SAM_WRN, "No user cache found in memory, creating new user cache.");

		/* open /etc/shells, to compare with. */
#ifdef HAVE_ETC_SHELLS
		fps = sample_shells_open();
#endif

/* strict /etc/passwd method, nothing else. */
#ifdef SAM_STRICT_PASSWD
		if((fp = fopen(SAM_PASSWD, "r")) == NULL)
			sample_error(SAM_ERR, "Failed to open passwd file: " SAM_PASSWD);
		memset(buf, 0, SAM_BUFSIZE_LARGE + 1);

		/* potential stream race-condition check. */
		orig_time = sampled_file_mod_diff(SAM_PASSWD, 0);

		while(fgets(buf, SAM_BUFSIZE_LARGE, fp) != NULL) {
			lptr = ptr = buf;
			i = 0;
			while(*ptr) {
				if(*ptr == ':' || *ptr == '\r' || *ptr == '\n') {
					*ptr = 0;
					switch(i++) {
						case 0: pwd.pw_name = lptr;
						case 1: pwd.pw_passwd = lptr;
						case 2: pwd.pw_uid = atoi(lptr);
						case 3: pwd.pw_gid = atoi(lptr);
						case 4: pwd.pw_gecos = lptr;
						case 5: pwd.pw_dir = lptr;
						case 6: pwd.pw_shell = lptr;
					}
					if(i > 6) break;

					lptr = ptr + 1;
				}
				ptr++;
			}

			if(i > 6
#ifdef HAVE_ETC_SHELLS
			&& (sampled_config.allow_noshell || !sample_shells_match(fps, pwd.pw_shell))
#endif
			/* is the user (dis)allowed to use sample? */
			/* already been inside this directory for this run? */
			&& sampled_users_allowed(pwd.pw_name) && !sample_entry_match(pwd.pw_dir) && !sample_entry_match(pwd.pw_name)) {

				/* this is the only important place to care if a race-condition situation happened.  'break' and wait for the next cycle. */
				if(sampled_file_mod_diff(SAM_PASSWD, orig_time)) {

					/* clear it so we can check next cycle, otherwise the cache will be partial. (if none it can handle it) */
					sampled_cache_free();

					break;
				}
				/* add the entry so we don't hit it again. (username or home directory) */
				sample_entry_add(pwd.pw_dir);
				sample_entry_add(pwd.pw_name);

				sampled_cache_add_pw(pwd);
			}
			memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
		}
		fclose(fp);

/* !SAM_STRICT_PASSWD, getpwent() will get network users/etc, where the /etc/passwd method is strict. */
#else
		setpwent();
		while((pwptr = getpwent()) != NULL) {

			/* is the user (dis)allowed to use sample? */
			/* already been inside this directory/user for this run? */
			if(sampled_users_allowed(pwptr->pw_name) && !sample_entry_match(pwptr->pw_dir) && !sample_entry_match(pwptr->pw_name)) {

				/* add the entry so we don't hit it again. (username or home directory) */
				sample_entry_add(pwptr->pw_dir);
				sample_entry_add(pwptr->pw_name);
#ifdef HAVE_ETC_SHELLS
				if(sampled_config.allow_noshell || !sample_shells_match(fps, pwptr->pw_shell))
#endif
					sampled_cache_add_pwptr(pwptr);
			}

		}
		endpwent();
#endif

#ifdef HAVE_ETC_SHELLS
		fclose(fps);
#endif
		/* free any entries, handles nulled situations. */
		sample_entry_free();
	}

	/* run through the cached users. */
	for(i = 0; i < sampled_cache_size; i++) {
		sampled_handle_user(sampled_cache[i]);
	}

	return;	
}
