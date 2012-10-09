/* [sample/samples/sampled] sample.h :: global do-it-all include file.
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

/* sample/samples/sampled version. */
#define SAM_VERSION			"1.3"
#define SAM_CMPTIME			"2007/09/16 21:52:43 EDT"

/* internal bool use. */
#define SAM_FALSE			0
#define SAM_TRUE			1
#define SAM_UNSUPPORTED			2

/* default configuration values. */
#define SAM_DEF_INTERVAL		1			/* ?? minute(s). */
#define SAM_DEF_ALLOW_NOSHELL		SAM_FALSE		/* allow users without shells in /etc/shell. */
#define SAM_DEF_ALLOW_SETSHELL		SAM_FALSE		/* allow users to set their shells. (SHELL=...) */
#define SAM_DEF_ALLOW_MULTIPLE_PROC	SAM_FALSE		/* allow multiple processes for a single user at a given time. */
#define SAM_DEF_ALLOW_MULTIPLE_UPROC	SAM_FALSE		/* allow multiple processes for a single user instance. (bad) */
#define SAM_DEF_DAEMON_PRIORITY		0			/* daemon process priority. (niceness) */
#define SAM_DEF_USER_PRIORITY		10			/* user processes priority. (niceness) */
#define SAM_DEF_DISABLE_TABS		SAM_FALSE		/* disabling processing of /etc/sampletab/ */
#define SAM_DEF_DISABLE_SAMPLE		SAM_FALSE		/* disabling processing of ~/.sample */
#define SAM_DEF_DISABLE_SPOOLS		SAM_FALSE		/* disabling processing of ~/.sample_spools/ */
#define SAM_DEF_SPOOL_MAXFILES		128			/* number of ~/.sample_spool/ files to process in a sitting. */
#define SAM_DEF_SPOOL_TTL_DEF		"6M"			/* default time before deleting un-executed spooled files. */
#define SAM_DEF_SPOOL_TTL_MAX		"1Y"			/* maximum time before deleting un-executed spooled files. */

/* all-purpose buffer sizes. */
#define SAM_BUFSIZE_TINY		32
#define SAM_BUFSIZE_SMALL		256
#define SAM_BUFSIZE_MEDIUM		1024
#define SAM_BUFSIZE_LARGE		4096
#define SAM_BUFSIZE_GIANT		8192

/* exit() codes. */
#define SAM_EXIT_SUCCESS		0
#define SAM_EXIT_ERROR			1

/* return() types. */
#define SAM_RETURN_SUCCESS		0
#define SAM_RETURN_FAIL			1

/* multiple uses for these types. */
#define SAM_REG				0
#define SAM_WRN				1
#define SAM_ERR				2

/* what to escape when passing filenames to a shell. (add '\' before character) */
/* make sure to remember and not escape '/', it can't be in a filename anyways. */
#define SAM_ESCAPE_CHARS "\"`'!#$%&*\\? |;()<>{}[]~"

/* ~/.filename to look for for each user on the system. (static file sampling) */
#define SAM_FILENAME			".sample"

/* ~/.dir to look in for spooled sample files. (deletes file at first find) */
#define SAM_SPOOLDIR			".sample_spool"

/* mark out time specifications. */
#define SAM_SECOND			1
#define SAM_MINUTE			(SAM_SECOND * 60)
#define SAM_HOUR			(SAM_MINUTE * 60)
#define SAM_DAY				(SAM_HOUR * 24)
#define SAM_WEEK			(SAM_DAY * 7)
#define SAM_MONTH			(SAM_DAY * 30) /* has to be averaged. */
#define SAM_YEAR			(SAM_DAY * 365)

/* assumed maximum size for times. */
#define SAM_TIMESIZE_MAX		0xFFFFFFFF

/* mark out size specifications. */
#define SAM_BYTE			1
#define SAM_KILOBYTE			(SAM_BYTE * 1024)
#define SAM_MEGABYTE			(SAM_KILOBYTE * 1024)
#define SAM_GIGABYTE			(SAM_MEGABYTE * 1024)

/* assumed maximum size for sizes. */
#define SAM_FILESIZE_MAX		0xFFFFFFFF

/* sampled_get_file_attr() types. */
#define SAM_GETATTR_UID			1
#define SAM_GETATTR_STRUID		2
#define SAM_GETATTR_GID			3
#define SAM_GETATTR_STRGID		4
#define SAM_GETATTR_MOD			5
#define SAM_GETATTR_STRMOD		6
#define SAM_GETATTR_SIZE		7
#define SAM_GETATTR_INODE		8

/* only allow certain users or ban certain users.  */
#define SAM_USERMODE_NONE		0
#define SAM_USERMODE_ALLOW		1
#define SAM_USERMODE_DENY		2

/* maximum number of spool files to try for samples. */
#define SAM_SPOOL_ATTEMPTS		256

/* samples operations. */
#define SAM_SPOOL_OP_ADD		1
#define SAM_SPOOL_OP_DEL		2
#define SAM_SPOOL_OP_WIPE		3

/* ./configure defines/etc. */
#include "../config.h"

/* global includes. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif


/* what i will do for setproctitle() */
#define INT_SETPROCTITLE
#ifndef __APPLE_CC__
#ifndef __linux__
#undef INT_SETPROCTITLE
#endif
#endif
#ifdef INT_SETPROCTITLE
#ifndef LINEBUFFER
#define LINEBUFFER 4096
#endif
#endif
#ifndef INT_SETPROCTITLE
#ifndef HAVE_SETPROCTITLE
#define NO_SETPROCTITLE
#endif
#endif

/* more compatability. */
#ifdef HAVE_SETRLIMIT
#ifndef RLIMIT_NOFILE
#ifdef RLIMIT_OFILE
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
#endif
#endif

/* they say this isn't everywheres, make it if not. */
#ifndef S_IFMT
#define S_IFMT 00170000
#endif

/* child pids that are monitored. (wpids) */
struct wpid_entry {
	char *user;
	pid_t pid;
};
struct wpid_struct {
	struct wpid_entry **entries;
	int size;
};

/* config value struct. */
struct config_struct {
	unsigned short interval;
#ifdef HAVE_ETC_SHELLS
	unsigned char allow_noshell;
#endif
	unsigned char allow_setshell;
	unsigned char allow_multiple_processes;
	unsigned char allow_multiple_user_processes;
	unsigned char disable_tabs;
	unsigned char disable_sample;
	unsigned char disable_spools;
	signed int daemon_priority;
	signed int user_priority;
	unsigned int spool_maxfiles;
	unsigned int spool_ttl_default;
	unsigned int spool_ttl_max;
	char *log_file;
	char *pid_file;
	char *tab_dir;

#ifdef HAVE_SETRLIMIT
	/* custom (user) processor resource limits. */
	struct {
#ifdef RLIMIT_CPU
		rlim_t cpu;
#endif
#ifdef RLIMIT_STACK
		rlim_t stack;
#endif
#ifdef RLIMIT_DATA
		rlim_t data;
#endif
#ifdef RLIMIT_NOFILE
		rlim_t files;
#endif
#ifdef RLIMIT_NPROC
		rlim_t processes;
#endif
#ifdef RLIMIT_FSIZE
		rlim_t filesize;
#endif
#ifdef RLIMIT_RSS
		rlim_t rss;
#endif
	} limit;
#endif /* HAVE_SETRLIMIT */
};

/* pseudo-setproctitle struct. */
#ifndef NO_SETPROCTITLE
struct proctitle_struct {
	char **argv;
	char *largv;
	char *name;
};
#endif

/* sampled user caching struct. */
struct sampled_cache_struct {
	char *pw_name;
	uid_t pw_uid;
	gid_t pw_gid;
	char *pw_dir;
	char *pw_shell;
	char *sample_file;
	char *sample_spooldir;
	char *sample_tab;
};

/* shared prototypes. */
unsigned char sample_is_privileged();
char *sample_basename(char *);
void sample_chomp(char *);
mode_t sample_path_mode(char *);
unsigned int sample_parse_time(char *, unsigned int);
unsigned int sample_parse_size(char *, unsigned int);
char *sample_parse_home_string(char *, char *, unsigned int);
#ifdef HAVE_ETC_SHELLS
FILE *sample_shells_open();
unsigned char sample_shells_match(FILE *, char *);
#endif
unsigned char sample_entry_match(char *);
void sample_entry_add(char *);
void sample_entry_free();

/* sample/samples and sampled have different functions for this, same prototype. */
void sample_error(unsigned char, char *, ...);

/* sampled prototypes. */
#ifndef NO_SETPROCTITLE
void initsetproctitle(signed int, char **, char **);
#ifndef HAVE_SETPROCTITLE
void setproctitle(const char *, ...);
#endif
#endif
void sampled_sleep();
char *sampled_system_string_format_numeric(char *, unsigned char);
char *sampled_system_string_format(char *, char *);
char *sampled_escape_string(char *);
unsigned char sampled_bad_file(char *);
unsigned char sampled_users_allowed(char *);
void sampled_users_makelist(char *, unsigned char);
unsigned char sampled_read_config(char *);
unsigned char sampled_valid_interval(signed int);
unsigned char sampled_valid_priority(signed int);
char *sampled_signal_value(signed int);
void sampled_signal_handler(signed int);
void sampled_signal_child_handler(signed int);
void sampled_setenv(char *, char *);
void sampled_putenv(char *);
void sampled_wpid_entry(char *user, pid_t);
pid_t sampled_wpid_get(char *);
void sampled_exec(char *, char *);
unsigned char sampled_set_permissions(struct sampled_cache_struct *);
unsigned char sampled_file_diff(char *, unsigned int, unsigned char);
void sampled_set_tabdir(char *, unsigned int);
void sampled_set_logfile(char *, unsigned int);
void sampled_set_pidfile(char *, unsigned int);
time_t sampled_file_mod_diff(char *, time_t);
char *sampled_get_file_attr(char *, unsigned char);
unsigned int sampled_rand_offset(unsigned int, unsigned int);
void sampled_write_pid(pid_t);
FILE *sampled_log_open();
#ifdef HAVE_SETRLIMIT
rlim_t sampled_limit_value(char *);
struct rlimit sampled_limit_max(rlim_t);
#ifdef RLIMIT_CORE
void sampled_limit_no_core();
#endif
void sampled_limit_unlimit();
void sampled_limit();
#endif
void sampled_cache_add(struct passwd *, struct passwd);
void sampled_cache_add_pwptr(struct passwd *);
void sampled_cache_add_pw(struct passwd);
void sampled_cache_free();
glob_t sampled_get_files(char *);
unsigned char sampled_handle_user_rc(struct sampled_cache_struct *, char *);
void sampled_handle_user(struct sampled_cache_struct *);
void sampled_pwd_cycle();

/* sample prototypes. */
unsigned int sample_list_user(struct passwd, unsigned int, unsigned char);
void sample_list_samples();
void sample_run_editor(char *, char *);
char *sample_get_editor(char *, char *);
void sample_check_exist(char *, struct passwd *, unsigned char, unsigned char);
size_t sample_get_file_count(char *, char *);
void sample_validate_printf(unsigned char, char *, unsigned int, char *, ...);
void sample_validate(char *, char *);

/* samples prototypes. */
void samples_usage(char *);
void samples_spool_allowed_file(char *, uid_t);
char *samples_spool_make_file(char *, char *, unsigned int, unsigned int);
void samples_spool_handle_add(signed int, char **, unsigned int, char *, uid_t, struct passwd *);
void samples_spool_handle_del(char *, char *, unsigned char);

