/* [sample] list.c :: file listing (-l) functions for sample.
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


/* lists a user if their is a ~/.sample in their home directory. (if they made it this far) */
unsigned int sample_list_user(struct passwd pwd, unsigned int num, unsigned char type) {
	unsigned int ret;
	glob_t g;
	char *sample_file, *sample_spooldir, *ptr;

	/* is the shell in /etc/shells? default=unsupported. */
	switch(type) {
		case SAM_FALSE:
			ptr="yes";
			break;
		case SAM_TRUE: 
			ptr="NO";
			break;
		default:
			ptr="(?)";
			break;
	}

	/* make temporary ~/.sample file to look for. */
	if(!(sample_file = (char *)malloc(strlen(pwd.pw_dir) + strlen(SAM_FILENAME) + 2)))
		sample_error(SAM_ERR, "Sample failed to allocate memory for creating ~/" SAM_FILENAME " path.");
	memset(sample_file, 0, strlen(pwd.pw_dir) + strlen(SAM_FILENAME) + 2);
	sprintf(sample_file, "%s/%s", pwd.pw_dir, SAM_FILENAME);


	/* make the ~/.sample_spool/ path. */
	if(!(sample_spooldir = (char *)malloc(strlen(pwd.pw_dir) + strlen(SAM_SPOOLDIR) + 4)))
		sample_error(SAM_ERR, "Samples failed to allocate memory for creating sample spool dir.");
	memset(sample_spooldir, 0, strlen(pwd.pw_dir) + strlen(SAM_SPOOLDIR) + 4);
	sprintf(sample_spooldir, "%s/" SAM_SPOOLDIR "/*", pwd.pw_dir);

	g.gl_pathc = 0;
	glob(sample_spooldir, 0, NULL, &g);
	free(sample_spooldir);

	ret = num;

	/* only show users with ~/.sample files. */
	if(!access(sample_file, F_OK) || g.gl_pathc) {

		/* sample_file will always be larger than 2 bytes, just by having ".sample" */
		if(access(sample_file, F_OK)) {
			sample_file[0] = '-';
			sample_file[1] = 0;
		}

		if(!num) printf("%-15s | %-15s | %-10s | %-6s | %s\n", "USERNAME (-u)", "SHELL", "REAL SHELL", "SPOOLS", "SAMPLE FILE");
		printf("%-15s | %-15s | %-10s | %-6u | %s\n", pwd.pw_name, pwd.pw_shell, ptr, (unsigned int)g.gl_pathc, sample_file);
		ret++;
	}

	free(sample_file);
	globfree(&g);

	return(ret);
}

/* run through all the /etc/passwd or getpwent() to find users with possibly executable ~/.sample files */
void sample_list_samples() {
	unsigned int num;
	struct passwd pwd;
#ifdef SAM_STRICT_PASSWD
	unsigned int i;
	char buf[SAM_BUFSIZE_LARGE + 1], *ptr, *lptr;
	FILE *fp;
#else
	struct passwd *gpwd;
#endif

	/* open /etc/shells, to compare with. */
#ifdef HAVE_ETC_SHELLS
	FILE *fps;
	fps = sample_shells_open();
#endif
	num = 0;

/* strict /etc/passwd method, nothing else. */
#ifdef SAM_STRICT_PASSWD
	if((fp = fopen(SAM_PASSWD, "r")) == NULL)
		sample_error(SAM_ERR, "Failed to open passwd file: " SAM_PASSWD);
	memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
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

		if(i > 6 && !sample_entry_match(pwd.pw_dir) && !sample_entry_match(pwd.pw_name)) {

			/* add the entry so we don't hit it again. */
			sample_entry_add(pwd.pw_dir);
			sample_entry_add(pwd.pw_name);

			num = sample_list_user(pwd, num,
#ifdef HAVE_ETC_SHELLS
			sample_shells_match(fps, pwd.pw_shell)
#else
			SAM_UNSUPPORTED
#endif
			);
		}

		memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
	}
	fclose(fp);

/* !SAM_STRICT_PASSWD, getpwent() will get network users/etc, wheres as the /etc/passwd method is strict. */
#else
	setpwent();
	while((gpwd = getpwent()) != NULL) {

		/* gpwd=&pwd essentially. */
		pwd.pw_name = gpwd->pw_name;
		pwd.pw_uid = gpwd->pw_uid;
		pwd.pw_gid = gpwd->pw_gid;
		pwd.pw_gecos = gpwd->pw_gecos;
		pwd.pw_dir = gpwd->pw_dir;
		pwd.pw_shell = gpwd->pw_shell;

		if(!sample_entry_match(pwd.pw_dir) && !sample_entry_match(pwd.pw_name)) {

			/* add the entry so we don't hit it again. */
			sample_entry_add(pwd.pw_dir);
			sample_entry_add(pwd.pw_name);

			num = sample_list_user(pwd, num,
#ifdef HAVE_ETC_SHELLS
			sample_shells_match(fps, pwd.pw_shell)
#else
			SAM_UNSUPPORTED
#endif
			);
		}
	}
	endpwent();
#endif

#ifdef HAVE_ETC_SHELLS
	fclose(fps);
#endif

	/* free any entries, handles nulled situations. */
	sample_entry_free();

	/* no active ~/.sample files in use on the system. */
	if(!num) sample_error(SAM_WRN, "No users found have ~/" SAM_FILENAME " or ~/" SAM_SPOOLDIR "/ file(s) that are being executed.");

	return;	
}

