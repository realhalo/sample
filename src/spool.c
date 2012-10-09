/* [samples] samples.c :: helper functions for samples. (sample spooler)
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


/* check if samples has access to the file to monitor.  */
void samples_spool_allowed_file(char *file, uid_t uid) {
	struct stat s;

	/* see if the file is even there. */
	if(access(file, F_OK))
		sample_error(SAM_ERR, "Sampling file does not appear to exist or is not accessible: %s", file);

	/* this would be odd to fail. */
	else if(stat(file, &s))
		sample_error(SAM_ERR, "Could not stat sampling file: %s", file);

	/* your file? if you're root, then let it go. */
	else if(uid != s.st_uid && uid != 0)
		sample_error(SAM_ERR, "Sampling file does not appear to be owned by you: %s (uid=%u/file uid=%u)", file, uid, s.st_uid);

	/* relative path from where "samples" is executed would be a problem. */
	else if(file[0] != '/')
		sample_error(SAM_ERR, "Sampling file is not an absolute path: %s", file);

	return;
}

/* makes up the sample spool filename, including path.  */
char *samples_spool_make_file(char *sample_spooldir, char *file, unsigned int ttl, unsigned int i) {
	unsigned int rnd;
	char *sample_spoolfile, *ptr;

	ptr = sample_basename(file);

	srand((unsigned int)time(0) * (i + 1));
	rnd = (rand() % 8999999) + 1000000;

	/* make filename: 7bytes = random size (above). 10bytes = maximum size of an int32. */
	/* ie. "~/.sample_spool/1234567.1234567890.basename" */
	if(!(sample_spoolfile = (char *)malloc(strlen(sample_spooldir) + 1 + 7 + 1 + 10 + 1 + strlen(ptr) + 1)))
		sample_error(SAM_ERR, "Samples failed to allocate memory for creating sample spool dir.");
	memset(sample_spoolfile, 0, strlen(sample_spooldir) + 1 + 7 + 1 + 10 + 1 + strlen(ptr) + 1);
	sprintf(sample_spoolfile, "%s%u.%u.%s", sample_spooldir, rnd, ttl, ptr);

	return(sample_spoolfile);
}

/* handles "add" operation. */
void samples_spool_handle_add(signed int argc, char **argv, unsigned int argc_offset, char *sample_spooldir, uid_t uid, struct passwd *pwd) {
	unsigned char type;
	unsigned int i, ttl;
	time_t t;
	char *sample_spoolfile, *value, *file, *cmd, *shell, tbuf[SAM_BUFSIZE_TINY];
	FILE *fp;

	/* setup easily referenced pointers. */
	file = argv[argc_offset + 1];
	type = argv[argc_offset + 2][0];
	value = argv[argc_offset + 3]; /* time|size. */
	cmd = argv[argc_offset + 4];
	if(argc - argc_offset > 5) ttl = sample_parse_time(argv[argc_offset + 5], strlen(argv[argc_offset + 5]));
	else ttl = 0;
	if(argc - argc_offset > 6) shell = argv[argc_offset + 6];
	else shell = 0;

	/* quick check: type. */
	if(!strchr("AaCcMmSs", type))
		sample_error(SAM_ERR, "Invalid sampling type: '%c' (valid: A, a, C, c, M, m, S or s)", type);

	/* quick check: time|size. */
	if(type != 'S' && type != 's') {
		if(!sample_parse_time(value, strlen(value)))
			sample_error(SAM_ERR, "Invalid sampling time: \"%s\"", value);
	}
	else if(!sample_parse_size(value, strlen(value)))
		sample_error(SAM_ERR, "Invalid sampling size: \"%s\"", value);

	/* quick check: file.  will exit from inside if an error occurs. */
	samples_spool_allowed_file(file, uid);

	/* quick check: cmd. (no validation for this, '%s' is not really strict for a one time call) */

	/* silently make ~/.sample_spool/, if it doesn't exist already. */
	if(access(sample_spooldir, F_OK)) {
		mkdir(sample_spooldir, 0700);

		/* make sure the selected/default user owns the newly created directory. */
		chown(sample_spooldir, pwd->pw_uid, pwd->pw_gid);
	}

	/* loop until we get a available ~/.sample_spool/ file. */
	for(i = 0; ;i++) {

		/* make sure the (random) sample filename isn't in use. */
		sample_spoolfile = samples_spool_make_file(sample_spooldir, file, ttl, i);
		if(access(sample_spoolfile, F_OK)) break;
		free(sample_spoolfile);

		/* have to bail at some point. */
		if(i > SAM_SPOOL_ATTEMPTS)
			sample_error(SAM_ERR, "Samples exhausted attempts (%u) to create a unique spool file in: %s", SAM_SPOOL_ATTEMPTS, sample_spooldir);
	}

	/* write the (new) spooled file. */
	if((fp = fopen(sample_spoolfile, "w")) == NULL)
		sample_error(SAM_ERR, "Failed to write to sample spool file: %s", sample_spoolfile);

	t = time(0);
	strftime(tbuf, SAM_BUFSIZE_TINY, "%m/%d/%Y %I:%M:%S%p", localtime(&t));

	fprintf(fp, "#?%s\n", file);
	fprintf(fp, "# %s: generated single-instance sample spool file. [%s]\n", argv[0], tbuf);
	if(shell) fprintf(fp, "SHELL=%s\n", shell);
	fprintf(fp, "%c\t%s\t%s\t%s\n", type, value, file, cmd);
	fclose(fp);

	/* make sure the specified/default user owns the newly created spool file. */
	if(chown(sample_spoolfile, pwd->pw_uid, pwd->pw_gid) || chmod(sample_spoolfile, 0600)) {
		unlink(sample_spoolfile);
		sample_error(SAM_ERR, "Failed to change owner/permissions on spool file %s", sample_spoolfile);
	}
	return;
}

/* handles "del" operation. (also handles "wipe") */
void samples_spool_handle_del(char *sample_spooldir, char *file, unsigned char wipe) { 
	unsigned int i, rnd, ttl, offset;
	glob_t g;
	char *globpath, *ptr, buf[SAM_BUFSIZE_LARGE + 1];
	struct stat s1, s2;
	FILE *fp;

	if(!(globpath = (char *)malloc(strlen(sample_spooldir) + 5 + 1)))
		sample_error(SAM_ERR, "Samples failed to allocate memory for creating sample spool dir.");
	memset(globpath, 0, strlen(sample_spooldir) + 5 + 1);
	sprintf(globpath, "%s%s", sample_spooldir, (wipe ? "*" : "*.*.*"));

	glob(globpath, 0, NULL, &g);
	free(globpath);

	for(i = 0; i < g.gl_pathc; i++) {

		/* if wiping, no need to check anything, just delete. */
		if(wipe) {
			if(unlink(g.gl_pathv[i]))
				sample_error(SAM_WRN, "Failed to remove spool file: %s", g.gl_pathv[i]);
			continue;
		}

		/* pluck out the file being sampled, ttl, and random number, or continue. */
		ptr = sample_basename(g.gl_pathv[i]);
		if(sscanf(ptr, "%u.%u.%n", &rnd, &ttl, &offset) != 2) continue;
		ptr += offset;

		/* the filename itself isn't the same, continue. */
		if(!*ptr || strcmp(ptr, sample_basename(file))) continue;

		/* open up the single-instance sample file. */
		if((fp = fopen(g.gl_pathv[i], "r")) == NULL) {
			sample_error(SAM_WRN, "Failed to open spool file: %s", g.gl_pathv[i]);
			continue;
		}

		/* find the exact path of the file being sampled. */
		memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
		if(fgets(buf, SAM_BUFSIZE_LARGE, fp) != NULL && strlen(buf) > 2 && !strncmp(buf, "#?", 2)) {
			sample_chomp(buf);
			ptr = buf + 2;

			/* not null. */
			if(*ptr) {

				/* both exist and same inode? must be it, delete. */
				if(!stat(ptr, &s1) && !stat(file, &s2) && s1.st_ino == s2.st_ino) {
					if(unlink(g.gl_pathv[i]))
						sample_error(SAM_WRN, "Failed to remove spool file: %s", g.gl_pathv[i]);
				}
			}
		}
		fclose(fp);
	}
	return;
}
