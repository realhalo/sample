/* [sample/samples/sampled] shared.c :: functions used throughout.
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

/* globals. */
char **sample_entries = 0;
unsigned int sample_entries_size = 0;


/* see if sample or sampled is set*id, which should never be. */
unsigned char sample_is_privileged() {
	if(getuid() != geteuid() || getgid() != getegid()) return(SAM_TRUE);
	return(SAM_FALSE);
}

/* like basename(), except no conflicts on different systems. (no free()) */
char *sample_basename(char *file) {
	char *ptr;
	ptr = strrchr(file, '/');
	return(ptr && *(ptr + 1) ? ptr + 1 : file);
}

/* modify a given string to take out the trailing '\r' and '\n' */
void sample_chomp(char *str) {
	char *ptr;
	if((ptr = strchr(str, '\r'))) *ptr = 0;
	else if((ptr = strchr(str, '\n'))) *ptr = 0;
	return;
}

/* breaks apart a "1h1m"-style time string into seconds in integer/time_t form. */
unsigned int sample_parse_time(char *stime, unsigned int slen) {
	unsigned int i, v, t;
	char *stime_ptr, *tmp, *tmp_ptr;

	/* make it the same maximum size it could possibly be. */
	if(!(tmp = (char *)malloc(slen + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for parsing sampled time.");
	memset(tmp, 0, slen + 1);

	stime_ptr = stime;
	tmp_ptr = tmp;
	i = 0;
	
	while(*stime_ptr) {

		/* build up a series of digits into a string to be processed. */
		if(isdigit((unsigned char)*stime_ptr)) *tmp_ptr++ = *stime_ptr;

		/* no digit to process? ignore. */
		else if(tmp != tmp_ptr && (v = atoi(tmp)) > 0) {
			t = 0;
			switch(*stime_ptr) {
				case 's': /* seconds. */
				case 'S':
					t = SAM_SECOND;
					break;
				case 'm': /* minutes. */
					t = SAM_MINUTE;
					break;
				case 'h': /* hours. */
				case 'H':
					t = SAM_HOUR;
					break;
				case 'd': /* days. */
				case 'D':
					t = SAM_DAY;
					break;
				case 'w': /* weeks. */
				case 'W':
					t = SAM_WEEK;
					break;
				case 'M': /* months. */
					t = SAM_MONTH;
					break;
				case 'y': /* years. */
				case 'Y':
					t = SAM_YEAR;
					break;
			}

			/* add seconds to the return value, if it doesn't overflow. */
			if(t && v + (i / t) <= (SAM_TIMESIZE_MAX / t)) i += v * t;

			memset(tmp, 0, slen + 1);
			tmp_ptr = tmp;
		}
		stime_ptr++;
	}
	free(tmp);
	return(i);
}

/* breaks apart a "1M1K"-style filesize strings into integer (byte) form. */
unsigned int sample_parse_size(char *ssize, unsigned int slen) {
	unsigned int i, v, t;
	char *ssize_ptr, *tmp, *tmp_ptr;

	/* make it the same maximum size it could possibly be. */
	if(!(tmp = (char *)malloc(slen + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for parsing sampled time.");
	memset(tmp, 0, slen + 1);

	ssize_ptr = ssize;
	tmp_ptr = tmp;
	i = 0;
	
	while(*ssize_ptr) {

		/* build up a series of digits into a string to be processed. */
		if(isdigit((unsigned char)*ssize_ptr)) *tmp_ptr++ = *ssize_ptr;

		/* no digit to process? ignore. */
		else if(tmp != tmp_ptr && (v = atoi(tmp)) > 0) {
			t = 0;
			switch(*ssize_ptr) {
				case 'b': /* bytes. */
				case 'B':
					t = SAM_BYTE;
					break;
				case 'k': /* bytes. */
				case 'K':
					t = SAM_KILOBYTE;
					break;
				case 'm': /* bytes. */
				case 'M':
					t = SAM_MEGABYTE;
					break;
				case 'g': /* bytes. */
				case 'G':
					t = SAM_GIGABYTE;
					break;
			}

			/* add seconds to the return value, if it doesn't overflow. */
			if(t && v + (i / t) <= (SAM_FILESIZE_MAX / t)) i += v * t;

			memset(tmp, 0, slen + 1);
			tmp_ptr = tmp;
		}
		ssize_ptr++;
	}
	free(tmp);
	return(i);
}

/* gets the mode_t of a file/directory, if it fails 0 is used. (don't depend on this function) */
mode_t sample_path_mode(char *path) {
	struct stat s;
	if(stat(path, &s)) s.st_mode = 0;
	return(s.st_mode);
}

/* returns a converted '~' to a specified home directory. */
char *sample_parse_home_string(char *str, char *home, unsigned int hlen) {
	unsigned int i;
	char *buf, *ptr, *bptr;

	i = 0;

	/* count all the '~''s */
	ptr = str;
	while((ptr = strchr(ptr, '~'))) {
		ptr++;
		i++;
	}

	/* calculate the new buffer size. */
	i *= (hlen - 1);
	i += strlen(str);
	if(!(buf = (char *)malloc(i + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for parsing sampled home directory.");
	memset(buf, 0, i + 1);

	/* set up pointers to read/write. */
	ptr = str;
	bptr = buf;
	while(*ptr) {
		if(*ptr == '~') {
			memcpy(bptr, home, hlen);
			bptr += hlen;
		}
		else *bptr++ = *ptr;
		ptr++;
	}
	*bptr = 0;
	return(buf);
}

/* open /etc/shell. */
#ifdef HAVE_ETC_SHELLS
FILE *sample_shells_open() {
	FILE *fp;
	if((fp = fopen(SAM_SHELLS, "r")) == NULL)
		sample_error(SAM_ERR, "Failed to open shells file: " SAM_SHELLS);
	return(fp);
}

/* check for a user's shell in /etc/shell. */
unsigned char sample_shells_match(FILE *fp, char *shell) {
	char buf[SAM_BUFSIZE_LARGE + 1];

#ifdef HAVE_REWIND
	rewind(fp);
#else
	fseek(fp, 0, SEEK_SET);
#endif

	memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
	while(fgets(buf, SAM_BUFSIZE_LARGE, fp) != NULL) {
		if(buf[0] == '#' || buf[0] == ';' || buf[0] == '\r' || buf[0] =='\n') continue;

		sample_chomp(buf);

		if(buf[0] && !strcmp(buf, shell)) return(SAM_RETURN_SUCCESS);
		memset(buf, 0, SAM_BUFSIZE_LARGE + 1);
	}
	return(SAM_RETURN_FAIL);
}
#endif /* HAVE_ETC_SHELLS */

/* is the entry in **sample_entries? */
unsigned char sample_entry_match(char *entry) {
	unsigned int i;

	/* we have nothing. */
	if(!sample_entries_size) return(SAM_FALSE);

	for(i = 0; i < sample_entries_size; i++) {
		if(!strcmp(sample_entries[i], entry))
			 return(SAM_TRUE);
	}
	return(SAM_FALSE);
}

/* create array if needed, and add new element. */
void sample_entry_add(char *entry) {
	if(!sample_entries_size) {
		if(!(sample_entries = (char **)malloc(sizeof(char *) * 2)))
			sample_error(SAM_ERR, "Failed to allocate memory for pwd entry monitoring.");
	}
	else {
		if(!(sample_entries = (char **)realloc(sample_entries, sizeof(char *) * (sample_entries_size + 2))))
			sample_error(SAM_ERR, "Failed to re-allocate memory for pwd entry monitoring.");

	}
	if(!(sample_entries[sample_entries_size] = (char *)malloc(strlen(entry) + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for pwd entry.");
	memset(sample_entries[sample_entries_size], 0, strlen(entry) + 1);
	strcpy(sample_entries[sample_entries_size++], entry);

	sample_entries[sample_entries_size] = 0;
	return;
}

/* free() array and elements if they exist. */
void sample_entry_free() {
	unsigned int i = 0;

	/* free all entries. */
	if(sample_entries_size) {
		for(i = 0; i < sample_entries_size; i++)
			free(sample_entries[i]);
		free(sample_entries);
	}

	/* free()'d, let it be known. */
	sample_entries = 0;
	sample_entries_size = 0;
	return;
}
