/* [sampled] errord.c :: error / logging for sampled. (shared function name)
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
extern pid_t root_proc;
extern struct config_struct sampled_config;
extern FILE *sampled_log;


/* record errors, and exit if needed. (different for sample/samples) */
void sample_error(unsigned char type, char *fmt, ...) {
	char buf[SAM_BUFSIZE_LARGE + 1], tbuf[SAM_BUFSIZE_TINY + 1];
	time_t t;
	struct passwd *pwd;
	struct tm *m;
	va_list ap;

	/* should only happen if sampled is backgrounded and opening the log_file failed. */
	if(!sampled_log) return;

	/* vanishing log file? re-create it, and the parent/core sampled process. */
	if(access(sampled_config.log_file, F_OK) && root_proc && sampled_log != stderr) {
		fclose(sampled_log);
		sampled_log = sampled_log_open();
		sample_error(SAM_REG, "--- sampled daemon continue: %u ---\n", root_proc);
	}

	memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
	va_start(ap, fmt);
	vsnprintf(buf, SAM_BUFSIZE_LARGE, fmt, ap);
	va_end(ap);
	t = time(NULL);
	m = localtime(&t);
	strftime(tbuf, SAM_BUFSIZE_TINY, "%m/%d/%Y %I:%M:%S%p", m);

	/* get the active user when this warning/error occured. */
	if(!(pwd = getpwuid(getuid()))) pwd->pw_name = "(UNKNOWN)";

	/* raw dump(SAM_REG)? or structured error message(SAM_WRN|SAM_ERR)? */
	if(type == SAM_REG) fputs(buf, sampled_log);
	else fprintf(sampled_log, "%-7s :: %s :: %-16s :: %s\n", (type == SAM_ERR ? "ERROR" : "WARNING"), tbuf, pwd->pw_name, buf);

	fflush(sampled_log);
	if(type == SAM_ERR) {

		/* log that we exited with an error for the root daemon. */
		if (root_proc) sample_error(SAM_REG, "--- sampled daemon exited: %u (ERROR) ---\n", root_proc);

		/* could possibly be stderr.*/
		if(sampled_log && sampled_log != stderr) fclose(sampled_log);

		exit(SAM_EXIT_ERROR);
	}
	return;
}

