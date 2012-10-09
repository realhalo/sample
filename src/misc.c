/* [sampled] misc.c :: miscellaneous functions for sampled.
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


/* switch uid/gid/groups to a specified user, it never returns to root. */
unsigned char sampled_set_permissions(struct sampled_cache_struct *sc) {
	if(initgroups(sc->pw_name, sc->pw_gid)) return(SAM_RETURN_FAIL);
	if(setgid(sc->pw_gid)) return(SAM_RETURN_FAIL);
	if(setuid(sc->pw_uid)) return(SAM_RETURN_FAIL);
	return(SAM_RETURN_SUCCESS);
}

/* compare the last modification of a file to now + offset, or filesize difference. */
unsigned char sampled_file_diff(char *path, unsigned int offset, unsigned char type) {
	unsigned int t, c;
	struct stat s;

	/* check properties our file. */
	if(stat(path, &s)) return(SAM_RETURN_FAIL);

	/* figure out what we are checking. */
	switch(type) {
		case 'a': /* last access. */
		case 'A':
			c = s.st_atime;
			break;
		case 'c': /* last status change. */
		case 'C':
			c = s.st_ctime;
			break;
		case 'm': /* last modification change. */
		case 'M':
			c = s.st_mtime;
			break;
		case 's': /* file size less/more than, instead of time. */
		case 'S':
			c = s.st_size;
			break;
		default: /* none of the above? */
			return(SAM_RETURN_FAIL);
			break;
	}

	/* it's a filesize check for smaller files. */
	if(type == 's') {

		/* the limit is still larger than the filesize. */
		if(offset < c) return(SAM_RETURN_FAIL);
	}

	/* it's a filesize check for larger files. */
	else if(type == 'S') {

		/* the limit is still smaller than the filesize. */
		if(offset > c) return(SAM_RETURN_FAIL);
	}

	/* it's a timestamp check. (both lower/upper case types handled here) */
	else{
		/* back to the future? */
		if(c > (t = time(NULL))) return(SAM_RETURN_FAIL);

		/* overflow? use the maximum value then. */
		if(c + offset < c) t = -1;

		/* offset hasn't passed yet. (uppercase type) */
		if(isupper((unsigned char)type) && (c + offset) > t) return(SAM_RETURN_FAIL);

		/* (reverse) offset hasn't passed yet. (lowercase type) */
		else if(islower((unsigned char)type) && (c + offset) < t) return(SAM_RETURN_FAIL);
	}

	/* offset has passed, run command. */
	return(SAM_RETURN_SUCCESS);
}

/* check to see if the modification time has changed on a given file.  pseudo-checks for race-conditions. */
/* 0 = no time change, else is a change. */
time_t sampled_file_mod_diff(char *path, time_t orig) {
	struct stat s;

	/* something non-zero, so it assumes failure. */
	if(stat(path, &s)) return(-1);

	return((s.st_mtime - orig));
}


/* do globbing.  add wildcards, if needed, to directories for glob(). (extra two bytes equated into maximum buffer size) */
glob_t sampled_get_files(char *path) {
	signed int i, flags;
	char *globpath, *pptr, *lptr, *ptr;
	glob_t g;

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
			if(sample_path_mode(globpath) & S_IFDIR) {
				ptr = globpath + strlen(globpath) - 1;
				if(*ptr++ != '/') *ptr++ = '/';
				*ptr++ = '*';
				*ptr = 0;
			}
			glob(globpath, flags, NULL, &g);
			free(globpath);

			/* if another path, append next time. */
			if(!flags) flags = GLOB_APPEND;

			if(!*pptr) break;
			lptr = pptr + 1;
		}
		pptr++;
	}

	return(g);
}

/* returns a string version for a variery of file attributes. */
char *sampled_get_file_attr(char *path, unsigned char g) {
	unsigned char ga, v;
	unsigned int i;
	char *str;
	struct stat s;
	struct passwd *pwd;
	struct group *grp;

	/* if it fails it should be picked up as default, will make "unknown" */
	if(stat(path, &s)) ga = 255;
	else ga = g;

	/* calculate our length. */
	switch(ga) {
		/* uid/gid in integer form shouldn't be longer than 5 characters. (short) */
		case SAM_GETATTR_UID:
		case SAM_GETATTR_GID:
			i = 5;
			break;

		/* arbitrary length. */
		case SAM_GETATTR_STRUID:
			if((pwd = getpwuid(s.st_uid))) i = strlen(pwd->pw_name);
			else i = 7; /* "unknown" */
			break;
		case SAM_GETATTR_STRGID:
			if((grp = getgrgid(s.st_gid))) i = strlen(grp->gr_name);
			else i = 7; /* "unknown" */
			break;

		/* ie. "0777" */
		case SAM_GETATTR_MOD:
			i = 4;
			break;

		/* ie. "-rwxrwxrwx" */
		case SAM_GETATTR_STRMOD:
			i = 10;
			break;

		/* size of file in bytes; 64bit max = 21 bytes, 32bit max = 10. */
		case SAM_GETATTR_SIZE:
#ifdef HAVE_LONG_LONG
			i = 21;
#else
			i = 10;
#endif
			break;

		case SAM_GETATTR_INODE:
			i = 10;
			break;
		default:
			/* "unknown" */
			i = 7;
			break;
	}

	/* create the correct buffer size. */
	if(!(str = (char *)malloc(i + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for file attribute data.");
	memset(str, 0, i + 1);

	/* write it out. */
	switch(ga) {
		case SAM_GETATTR_UID:
			snprintf(str, i, "%u", (unsigned int)s.st_uid);
			break;
		case SAM_GETATTR_GID:
			snprintf(str, i, "%u", (unsigned int)s.st_gid);
			break;
		case SAM_GETATTR_STRUID:
			/* pwd already exists from the size allocation. */
			if (pwd) strcpy(str, pwd->pw_name);
			else strcpy(str, "unknown");
			break;
		case SAM_GETATTR_STRGID:
			/* grp already exists from the size allocation. */
			if (grp) strcpy(str, grp->gr_name);
			else strcpy(str, "unknown");
			break;

		/* ie. "0777" */
		case SAM_GETATTR_MOD:

			/* X--- */
			v = 0;
			if (s.st_mode & S_ISUID) v += 4;
			if (s.st_mode & S_ISGID) v += 2;
			if (s.st_mode & S_ISVTX) v += 1;
			str[0] = (unsigned char)(v + 0x30);

			/* -X-- */
			v = 0;
			if (s.st_mode & S_IRUSR) v += 4;
			if (s.st_mode & S_IWUSR) v += 2;
			if (s.st_mode & S_IXUSR) v += 1;
			str[1] = (unsigned char)(v + 0x30);

			/* --X- */
			v = 0;
			if (s.st_mode & S_IRGRP) v += 4;
			if (s.st_mode & S_IWGRP) v += 2;
			if (s.st_mode & S_IXGRP) v += 1;
			str[2] = (unsigned char)(v + 0x30);

			/* ---X */
			v = 0;
			if (s.st_mode & S_IROTH) v += 4;
			if (s.st_mode & S_IWOTH) v += 2;
			if (s.st_mode & S_IXOTH) v += 1;
			str[3] = (unsigned char)(v + 0x30);
			break;

		/* ie. "-rwxrwxrwx" */
		case SAM_GETATTR_STRMOD:
			strcpy(str, "----------");

			/* X--------- */
			if((s.st_mode & S_IFMT) == S_IFDIR) str[0] = 'd';
			else if((s.st_mode & S_IFMT) == S_IFLNK) str[0] = 'l';
			else if((s.st_mode & S_IFMT) == S_IFCHR) str[0] = 'c';
			else if((s.st_mode & S_IFMT) == S_IFBLK) str[0] = 'b';
			else if((s.st_mode & S_IFMT) == S_IFIFO) str[0] = 'p';
			else if((s.st_mode & S_IFMT) == S_IFSOCK) str[0] = 's';

			/* -XXX------ */
			if(s.st_mode & S_IRUSR) str[1] = 'r';
			if(s.st_mode & S_IWUSR) str[2] = 'w';
			if(s.st_mode & S_IXUSR) {
				if(s.st_mode & S_ISUID) str[3] = 's';
				else str[3] = 'x';
			}
			else if(s.st_mode & S_ISUID) str[3] = 'S';

			/* ----XXX--- */
			if(s.st_mode & S_IRGRP) str[4] = 'r';
			if(s.st_mode & S_IWGRP) str[5] = 'w';
			if(s.st_mode & S_IXGRP) {
				if(s.st_mode & S_ISGID) str[6] = 's';
				else str[6] = 'x';
			}
			else if(s.st_mode & S_ISGID) str[6] = 'S';

			/* -------XXX */
			if(s.st_mode & S_IROTH) str[7] = 'r';
			if(s.st_mode & S_IWOTH) str[8] = 'w';
			if(s.st_mode & S_IXOTH) {
				if(s.st_mode & S_ISVTX) str[9] = 't';
				else str[9] = 'x';
			}
			else if(s.st_mode & S_ISVTX) str[9] = 'T';
			break;

		/* size of file in bytes. */
		case SAM_GETATTR_SIZE:
#ifdef HAVE_LONG_LONG
			snprintf(str, i, "%llu", (unsigned long long)s.st_size);
#else
			snprintf(str, i, "%u", (unsigned int)s.st_size);
#endif
			break;

		case SAM_GETATTR_INODE:
			snprintf(str, i, "%u", (unsigned int)s.st_ino);
			break;
		default:
			strcpy(str, "unknown");
			break;
	}

	return(str);
}

/* write pid to /var/run/sampled.pid, or other specified.  bail if locked. */
void sampled_write_pid(pid_t pid) {
	signed int fd;
	FILE *fp;

	/* where not available, try this. */
#ifndef LOCK_EX
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = pid;
#endif

	/* shouldn't happen, but why not be safe. */
	if(!sampled_config.pid_file) return;

	/* doesn't exist? attempt to create it with permissions "-rw-r--r--". */
	if ((fd = open(sampled_config.pid_file, O_RDWR | O_CREAT, 0644)) < 0 || !(fp = fdopen(fd, "r+")))
		return;

	/* make/check the lock on the file, bail if it's in use. */
#ifndef LOCK_EX
	else if (fcntl(fd, F_SETLK, &fl) < 0)
#else
	else if (flock(fd, LOCK_EX | LOCK_NB) < 0)
#endif
		exit(SAM_EXIT_ERROR);

	(void)fcntl(fd, F_SETFD, 1);

	/* write the pid. */
	fprintf(fp, "%u\n", (unsigned int)pid);
	fflush(fp);

	/* ...leave the file open, to keep the lock. */

	return;
}

/* open /var/log/sample.log, or other specified. */
FILE *sampled_log_open() {
	signed int fd;
	FILE *fp;

	/* doesn't exist? attempt to create it with permissions "-rw-------". */
	if ((fd = open(sampled_config.log_file, O_WRONLY | O_CREAT, 0600)) < 0 || !(fp = fdopen(fd, "a"))) {

		/* fallback. */
		fp = stderr;
		sample_error(SAM_ERR, "Failed to open sampled log file: %s", sampled_config.log_file);
	}
	return(fp);
}

/* make sure a file with non-printable characters isn't processed. */
unsigned char sampled_bad_file(char *str) {
	char *ptr;
	ptr = str;
	while(*ptr) {
		if(!isprint((unsigned char)*ptr)) return(SAM_TRUE);
		ptr++;
	}
	return(SAM_FALSE);
}

/* takes the total number, and chooses a random location that is: max+rand<total. */
/* max = maximum section, total = total number (of files). */
unsigned int sampled_rand_offset(unsigned int max, unsigned int total) {
	unsigned int r;
	if (!max || max >= total) return(0);
	srand((unsigned int)time(0));
	r = (rand() % (total - max + 1));
	return(r);
}

