/* [sample] sample.c :: core functions for sample.
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

static const char id[] = "$Id: sample,v " SAM_VERSION " " SAM_CMPTIME " fakehalo Exp $";

/* ~/.sample template made if no ~/.sample already exists, and '-n' isn't used. */
static char sample_example[] =
	"# << Auto-generated \"helper\" sample file to be processed by sampled >>\n"
	"#\n"
	"# Sampling job-per-line format (with tabs between):\n"
	"#  [TYPE]\t[TIME|SIZE]\t[PATH]\t\t\t[EXEC]\n"
	"#  AaCcMmSs\tsmhdwMY|BKMG\tpath:path:...\t\tshell command\n"
	"#\n"
	"# Extended sampling field formating information:\n"
	"#  [TYPE]\t(only use one)\t\t\t[TIME]\t(any combination)\n"
	"#  A\t=\t[TIME] Older than last access\ts\t=\tSecond(s)\n"
	"#  a\t=\t[TIME] Newer than last access\tm\t=\tMinute(s)\n"
	"#  C\t=\t[TIME] Older than last change\th\t=\tHour(s)\n"
	"#  c\t=\t[TIME] Newer than last change\td\t=\tDay(s)\n"
	"#  M\t=\t[TIME] Older than last modify\tw\t=\tWeek(s)\n"
	"#  m\t=\t[TIME] Newer than last modify\tM\t=\tMonth(s)\n"
	"#  S\t=\t[SIZE] Larger than filesize\tY\t=\tYear(s)\n"
	"#  s\t=\t[SIZE] Smaller than filesize\t[SIZE]\t(any combination)\n"
	"#  [EXEC]\t(format replacement values)\tB\t=\tByte(s)\n"
	"#  %s\t=\tFull matched file path\t\tK\t=\tKiloboyte(s)\n"
	"#  %f\t=\tMatched filename only\t\tM\t=\tMegabyte(s)\n"
	"#  %p\t=\tMatched path only\t\tG\t=\tGigabyte(s)\n"
	"#  %n\t=\tMatched file's fizesize\t\t[PATH]\n"
	"#  (see \"sample\" manpage for more formats...)\t*.* shell-wildcards supported.\n"
	"#\n"
	"# Example (would update index.htm using mindex.pl every 1 hour and 30 minutes):\n"
	"#  M\t\t1h30m\t~/public_html/index.htm\t\tperl ~/mindex.pl > %s\n"
	"#\n"
	"# Example (would move 6 month old unread .txt/.rtf files in \"~\" into \"~/old/\"):\n"
	"#  A\t\t6M\t~/*.txt:~/*.rtf\t\t\tmv -f -- %s ~/old/%f\n"
	"#\n"
	"# Example (would delete 100 meg or larger files in your home directory):\n"
	"#  S\t\t100M\t~/*\t\t\t\trm -f -- %s\n"
	"\n";


/* defacto $PATH, if no $PATH is set. */
static char *sample_paths = "/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin:/usr/local/sbin";


/* execution start. */
int main(signed int argc, char **argv) {
	unsigned char example, remove, tab;
	signed int chr;
	char *editor, *sample_file, *ptr;
	uid_t uid;
	struct passwd *pwd;

	/* see if we're set*id, which shouldn't be. */
	if(sample_is_privileged())
		sample_error(SAM_ERR, "sample appears to be set*id, which should never be.");

	/* defaults. */
	example = SAM_TRUE;
	remove = SAM_FALSE;
	tab = SAM_FALSE;

	editor = 0;
	uid = getuid();

	while((chr = getopt(argc, argv, "vrltu:e:nh")) != EOF) {
		switch(chr) {
			case 'v': /* version. */
				printf("%s: version " SAM_VERSION "\n", argv[0]);
				exit(SAM_EXIT_SUCCESS);
				break;
			case 'n': /* don't create example comments in ~/.sample for new files. */
				example = SAM_FALSE;
				break;
			case 'r': /* remove your (or -u specified) ~/.sample file. */
				remove = SAM_TRUE;
				break;				
			case 'l': /* (root) list users on the system with ~/.sample files. */
				if(getuid()) sample_error(SAM_ERR, "'-l' option can only be used by root.");
				sample_list_samples();
				exit(SAM_EXIT_SUCCESS);
				break;				
			case 't': /* (root) switch to /etc/sampletab/<username>, instead of ~/.sample */
				if(getuid()) sample_error(SAM_ERR, "'-t' option can only be used by root.");
				tab = SAM_TRUE;
				break;				
			case 'u': /* (root) specify other user to edit. */
				if(getuid()) sample_error(SAM_ERR, "'-u' option can only be used by root.");
				if(!(pwd = getpwnam(optarg)))
					sample_error(SAM_ERR, "'-u' option argument is not a valid username.");
				else uid = pwd->pw_uid;
				break;
			case 'e': /* specify alternate editor. */
				if(!access(optarg, X_OK) && (sample_path_mode(optarg) & S_IFREG)) {
					if(editor) free(editor);
					if(!(editor = (char *)malloc(strlen(optarg) + 1)))
						sample_error(SAM_ERR, "Sample failed to allocate memory for creating editor path.");
					memset(editor, 0, strlen(optarg) + 1);
					strcpy(editor, optarg);
				}

				/* no '/'? maybe it wants us to find the path. */
				else if(!strchr(optarg, '/')) {

					/* try $PATH first. */
					if((ptr = getenv("PATH"))) editor = sample_get_editor(ptr, optarg);

					/* nothing? try built-in paths. */
					if(!editor) editor = sample_get_editor(sample_paths, ptr);

				}

				if (!editor) sample_error(SAM_ERR, "'-e' option argument does not exist or isn't executable.");
				break;
			case 'h': /* brief help. */
			default:
				printf("usage: %s [-vnrlt] [-u user] [-e editor]\n", argv[0]);
				exit(SAM_EXIT_ERROR);
				break;
		}
	}


	/* sanity check. */
	if(!(pwd = getpwuid(uid)))
		sample_error(SAM_ERR, "No passwd information for this UID? this shouldn't happen.");
	if(!pwd->pw_dir || !strlen(pwd->pw_dir))
		sample_error(SAM_ERR, "No home directory for this UID? this shouldn't happen.");

	/* make /etc/sampletab/<username> (-t option as root) or ~/.sample string. */
	if(tab) {

		/* make the tab directory if it doesn't exist. (we'll be root if we make it here) */
		if(access(SAM_TABDIR, F_OK)) mkdir(SAM_TABDIR, 0755);

		if(!(sample_file = (char *)malloc(strlen(SAM_TABDIR) + strlen(pwd->pw_name) + 2)))
			sample_error(SAM_ERR, "Sample failed to allocate memory for creating sample filename.");
		memset(sample_file, 0, strlen(SAM_TABDIR) + strlen(pwd->pw_name) + 2);
		sprintf(sample_file, SAM_TABDIR "/%s", pwd->pw_name);
	}
	else {
		if(!(sample_file = (char *)malloc(strlen(pwd->pw_dir) + strlen(SAM_FILENAME) + 2)))
			sample_error(SAM_ERR, "Sample failed to allocate memory for creating sample filename.");
		memset(sample_file, 0, strlen(pwd->pw_dir) + strlen(SAM_FILENAME) + 2);
		sprintf(sample_file, "%s/%s", pwd->pw_dir, SAM_FILENAME);
	}

	/* if we're removing, do it first and bail. */
	if(remove) {
		if(access(sample_file, F_OK)) sample_error(SAM_ERR, "%s's \"%s\" doesn't appear to exist.", pwd->pw_name, sample_file);
		else if(unlink(sample_file)) sample_error(SAM_ERR, "%s's \"%s\" failed to be removed.", pwd->pw_name, sample_file);
		else sample_error(SAM_WRN, "%s's \"%s\" has been removed successfully.", pwd->pw_name, sample_file);
		exit(SAM_EXIT_SUCCESS);
	}

	/* possibly set by the -e command-line argument. */
	if(!editor) {

		/* try user's choice first. ('-e' is still priority) */
		if((ptr = getenv("EDITOR"))) {

			/* direct path. */
			if(!access(ptr, X_OK)) {

				/* free()ing a getenv() pointer is no good, follow the mold. */
				if(!(editor = (char *)malloc(strlen(ptr) + 1)))
					sample_error(SAM_ERR, "Sample failed to allocate memory for creating direct editor path.");
				memset(editor, 0, strlen(ptr) + 1);
				strcpy(editor, ptr);
			}

			/* no '/' in $EDITOR? maybe it wants us to find the path. */
			else if(!strchr(ptr, '/')) {

				/* try $PATH first. */
				if(getenv("PATH")) editor = sample_get_editor(getenv("PATH"), ptr);

				/* nothing? try built-in paths. */
				if(!editor) editor = sample_get_editor(sample_paths, ptr);
			}

			/* oh well. */
			if (!editor) sample_error(SAM_ERR, "$EDITOR is set, but the file does not exist or isn't executable.");
		}

		/* generic editor. */
		else{

			/* try $PATH first. */
			if((ptr = getenv("PATH"))) editor = sample_get_editor(ptr, NULL);

			/* nothing? try built-in paths. */
			if(!editor) editor = sample_get_editor(sample_paths, NULL);
		}
	}

	/* nothing? too bad. */
	if(!editor) 
		sample_error(SAM_ERR, "No viable text editor found to edit ~/" SAM_FILENAME ".");

	/* see if we have a ~/.sample already, create with/without example if not. */
	sample_check_exist(sample_file, pwd, example, tab);

	/* start up the editor to edit the ~/.sample and wait. */	
	sample_run_editor(sample_file, editor);

	/* another back-up, different for a sampletab. */
	if(tab) {
		chmod(sample_file, 0640);
		chown(sample_file, 0, pwd->pw_gid);
	}
	else {
		chmod(sample_file, 0600);
		chown(sample_file, pwd->pw_uid, pwd->pw_gid);
	}

	/* validate the updated/new ~/.sample file. */
	sample_validate(sample_file, pwd->pw_dir);

	free(editor);
	free(sample_file);

	exit(SAM_EXIT_SUCCESS);
}

/* checks if ~/.sample exists for a given user, and makes the example ~/.sample if needed. */
void sample_check_exist(char *sample_file, struct passwd *pwd, unsigned char example, unsigned char tab) {
	signed int fd;

	/* already exists, abort. */
	if(!access(sample_file, F_OK)) return;

	if ((fd = open(sample_file, O_WRONLY | O_CREAT, (tab ? 0640 : 0600))) < 0)
		sample_error(SAM_ERR, "Sample failed to open: %s", sample_file);

#ifdef HAVE_FCHOWN
	fchown(fd, (tab ? 0 : pwd->pw_uid), pwd->pw_gid);
#endif
	/* write example if specified. */
	if(example) write(fd, sample_example, strlen(sample_example));

	close(fd);
#ifndef HAVE_FCHOWN
	chown(sample_file, (tab ? 0 : pwd->pw_uid), pwd->pw_gid);
#endif
	return;
}

/* tests the path for the number of files it matches. */
size_t sample_get_file_count(char *path, char *home) {
	signed int i, flags;
	size_t tot;
	char *homepath, *globpath, *pptr, *lptr, *ptr;
	glob_t g;

	tot = g.gl_pathc = 0;
	flags = 0;
	pptr = lptr = path;

	/* will break when it hits a null-byte. */
	while(SAM_TRUE) {

		/* "path:path:..." support. */
		if(!*pptr || *pptr == ':') {
			i = pptr - lptr;

			/* no path? */
			if(i < 1) {
				if(!*pptr) break;
				pptr++;
				lptr = pptr;
				continue;
			}

			/* need a possible extra 2 bytes on the end of the buffer, so copy it to modify it. */
			if(!(globpath = (char *)malloc(i + 3)))
				sample_error(SAM_ERR, "Failed to allocate memory for creating glob() path.");
			memset(globpath, 0, i + 3);
			strncpy(globpath, lptr, i);

			/* no directories. */
			if((sample_path_mode(globpath) & S_IFDIR)) {
				ptr = globpath + strlen(globpath) - 1;
				if(*ptr++ != '/') *ptr++ = '/';
				*ptr++ = '*';
				*ptr = 0;
			}

			/* parse ~ to the home dir of the user. */
			homepath = sample_parse_home_string(globpath, home, strlen(home));

			glob(homepath, flags, NULL, &g);

			free(homepath);
			free(globpath);

			/* if another path, append next time. */
			if(!flags) flags = GLOB_APPEND;

			if(!*pptr) break;
			lptr = pptr + 1;
		}
		pptr++;
	}
	tot = g.gl_pathc;
	globfree(&g);
	return(tot);
}
