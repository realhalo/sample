/* [samples] samples.c :: core functions for samples. (sample spooler)
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

static const char id[] = "$Id: samples,v " SAM_VERSION " " SAM_CMPTIME " fakehalo Exp $";


/* execution start. */
int main(signed int argc, char **argv) {
	unsigned char op;
	unsigned int argc_offset, ttl;
	char *sample_spooldir;
	uid_t uid;
	struct passwd *pwd;

	/* see if we're set*id, which shouldn't be. */
	if(sample_is_privileged())
		sample_error(SAM_ERR, "samples appears to be set*id, which should never be.");

	ttl = 0;
	uid = getuid();

	/* need at least 1 arguments for anything beyond here. */
	if(argc < 2) samples_usage(argv[0]);

	/* set our argument offset, and check for a specified user to run as. */
	argc_offset = 1;
	if(argc > 2 && strlen(argv[2]) > 1 && argv[2][0] == '-') {
		if(uid) sample_error(SAM_ERR, "'%s' user specification option can only be used by root.", argv[2]);
		if(!(pwd = getpwnam(argv[2] + 1)))
			sample_error(SAM_ERR, "'%s' user specification option is not a valid username.", argv[2]);
		else uid = pwd->pw_uid;
		argc_offset++;
	}

	/* user sanity check. */
	if(!(pwd = getpwuid(uid)))
		sample_error(SAM_ERR, "No passwd information for this UID? this shouldn't happen.");
	if(!pwd->pw_dir || !strlen(pwd->pw_dir))
		sample_error(SAM_ERR, "No home directory for this UID? this shouldn't happen.");

	/* make the ~/.sample_spool/ path. */
	if(!(sample_spooldir = (char *)malloc(strlen(pwd->pw_dir) + strlen(SAM_SPOOLDIR) + 3)))
		sample_error(SAM_ERR, "Samples failed to allocate memory for creating sample spool dir.");
	memset(sample_spooldir, 0, strlen(pwd->pw_dir) + strlen(SAM_SPOOLDIR) + 3);
	sprintf(sample_spooldir, "%s/" SAM_SPOOLDIR "/", pwd->pw_dir);

	/* what is our operation? */
	/* see if '-v' is there, need a way to version samples. */
	if(!strcmp(argv[1], "-v")) {
		free(sample_spooldir);
		printf("%s: version " SAM_VERSION "\n", argv[0]);
		exit(SAM_EXIT_SUCCESS);
	}
	else if(!strncmp(argv[1], "add", 3) && argc - argc_offset > 4) op = SAM_SPOOL_OP_ADD;
	else if(!strncmp(argv[1], "del", 3) && argc - argc_offset > 1) op = SAM_SPOOL_OP_DEL;
	else if(!strncmp(argv[1], "wipe", 4)) op = SAM_SPOOL_OP_WIPE;
	else {
		free(sample_spooldir);
		samples_usage(argv[0]);
	}

	/* add monitoring to file. */
	if(op == SAM_SPOOL_OP_ADD) samples_spool_handle_add(argc, argv, argc_offset, sample_spooldir, uid, pwd);

	/* delete monitoring to file. */
	else if(op == SAM_SPOOL_OP_DEL) {
		for(argc_offset++; argc_offset < argc; argc_offset++) {
			if(access(argv[argc_offset], F_OK))
				sample_error(SAM_WRN, "Non-existent file to remove sample spool(s) on: %s", argv[argc_offset]);
			else
				samples_spool_handle_del(sample_spooldir, argv[argc_offset], SAM_FALSE);
		}

	}

	/* wipe all monitoring of files. */
	else if(op == SAM_SPOOL_OP_WIPE) {
		if(access(sample_spooldir, F_OK))
			sample_error(SAM_ERR, "Non-existent sample spool directory: %s", sample_spooldir);
		samples_spool_handle_del(sample_spooldir, NULL, SAM_TRUE);
	}

	exit(SAM_EXIT_SUCCESS);
}

/* usage for samples. */
void samples_usage(char *prog) {
	printf(
	"usage: %s add [-<user>] <file> <type> <time|size> <exec> [ttl] [shell]\n"
	"usage: %s del [-<user>] <file> ...\n"
	"usage: %s wipe [-<user>]\n", prog, prog, prog);

	exit(SAM_EXIT_ERROR);
}
