/* [sample] validate.c :: validation functions for ~/.sample files.
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

/* globals */
unsigned int num_errors = 0;
unsigned int num_warnings = 0;


/* verbose messages for sample_validate(). */
void sample_validate_printf(unsigned char type, char *sample_file, unsigned int l, char *fmt, ...) {
	char buf[SAM_BUFSIZE_LARGE + 1];
	va_list ap;
	memset(buf, 0, SAM_BUFSIZE_LARGE);
	va_start(ap, fmt);
	vsnprintf(buf, SAM_BUFSIZE_LARGE, fmt, ap);
	va_end(ap);
	printf("%s:%u: %s: %s\n", sample_file, l, (type == SAM_ERR ? "error" : "warning"),  buf);
	if(type == SAM_ERR) num_errors++;
	else num_warnings++;
	return;
}

/* validates the edited ~/.sample file, just displays errors. */
void sample_validate(char *sample_file, char *home) {
	unsigned char type;
	unsigned int i, l;
	char buf[SAM_BUFSIZE_GIANT + 1], *ptr, *tptr[4];
	FILE *fp;

	if((fp = fopen(sample_file, "r")) == NULL)
		sample_error(SAM_ERR, "Failed to open sample file for validation: %s", sample_file);
	memset(buf, 0, SAM_BUFSIZE_GIANT + 1);

	l = 0;

	while(fgets(buf, SAM_BUFSIZE_GIANT, fp) != NULL) {
		l++;
		if(buf[0] == '#' || buf[0] == ';' || buf[0] == '\r' || buf[0] =='\n') continue;

		sample_chomp(buf);

		if(!buf[0]) continue;

		if(!strchr(buf, '\t') && (ptr = strchr(buf, '='))) {
			if(buf[0] == '=')
				sample_validate_printf(SAM_ERR, sample_file, l, "[ENV] setting blank environmental variable name.");
			if(!*(ptr + 1))
				sample_validate_printf(SAM_WRN, sample_file, l, "[ENV] setting environmental variable to nothing?");
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

			/* appears to be a valid ~/.sample line entry, including the time value > 0, use it. */
			if(i >= 3 && tptr[0] && tptr[1] && tptr[2] && tptr[3]) {

				/* we only need the first character to find out what type of time to check. */
				type = (unsigned char)tptr[0][0];

				/* [TYPE] checks. */
				if(strlen(tptr[0]) > 1)
					sample_validate_printf(SAM_WRN, sample_file, l, "[TYPE] value is more than a single character, appended character(s) are ignored.");
				else if(!strchr("AaCcMmSs", type))
					sample_validate_printf(SAM_ERR, sample_file, l, "[TYPE] value is invalid, should be 'A', 'a', 'C', 'c', 'M', 'm', 'S' or 's'");

				/* [TIME|SIZE] checks. */
				if(type != 's' && type != 'S') {
					if(!sample_parse_time(tptr[1], strlen(tptr[1])))
						sample_validate_printf(SAM_ERR, sample_file, l, "[TIME] value is invalid, time period must be at least 1 second.");
					else if(sample_parse_time(tptr[1], strlen(tptr[1])) > SAM_YEAR * 5)
						sample_validate_printf(SAM_WRN, sample_file, l, "[TIME] value is larger than 5 years, is this right?");
				}
				else{
					if(!sample_parse_size(tptr[1], strlen(tptr[1])))
						sample_validate_printf(SAM_ERR, sample_file, l, "[SIZE] value is invalid, filesize must be at least 1 byte.");
					else if(sample_parse_size(tptr[1], strlen(tptr[1])) > SAM_GIGABYTE)
						sample_validate_printf(SAM_WRN, sample_file, l, "[SIZE] value is larger than a gigabyte, is this right?");
				}

				/* [PATH] checks. */
				if(!sample_get_file_count(tptr[2], home))
					sample_validate_printf(SAM_WRN, sample_file, l, "[PATH] path pattern currently matches no files.");
				else if(sample_get_file_count(tptr[2], home) > 1000)
					sample_validate_printf(SAM_WRN, sample_file, l, "[PATH] path pattern matches over 1000 files.");

				/* [EXEC] checks. */
				if(!strstr(tptr[3], "%s"))
					sample_validate_printf(SAM_WRN, sample_file, l, "[EXEC] contains no '%%s' match replacement variable to update the file.");
			}
			else if(i > 1) sample_validate_printf(SAM_ERR, sample_file, l, "line has %u tab separated value(s), 3 are needed.", i - 1);
			else sample_validate_printf(SAM_ERR, sample_file, l, "line matches neither a sample time or an environmental variable.");
		}
		memset(buf, 0, SAM_BUFSIZE_GIANT + 1);
	}
	fclose(fp);

	/* verbose info. */
	printf("%s: sample scheme is %s. (%u errors/%u warnings)\n", sample_file, (num_errors ? "BAD" : "GOOD"), num_errors, num_warnings);

	return;
}

