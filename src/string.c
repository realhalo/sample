/* [sampled] string.c :: string related functions for sampled.
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


/* used for "%0-9" in system strings, to pluck out parts of the directory tree. */
char *sampled_system_string_format_numeric(char *str, unsigned char val) {
	unsigned char i;
	unsigned int len;
	char *buf, *ptr, *lptr;

	lptr = ptr = str;
	len = i = 0;

	while(*ptr) {
		if(*ptr == '/') {
			if(i++ == val + 1) {
				len = ptr - lptr;
				break;
			}
			lptr = ptr + 1;
		}
		ptr++;
	}

	/* nothing to pass back. */
	if(!len) return(NULL);

	/* calculate the new buffer size, and copy. */
	if(!(buf = (char *)malloc(len + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for parsing sampled home directory.");
	memset(buf, 0, len + 1);
	strncpy(buf, lptr, len);

	return(buf);
}

/* makes the string to be passed to /bin/sh, or other user-defined shell. (puts the filename/etc in) */
char *sampled_system_string_format(char *str, char *file) {
	unsigned int len, baselen, ebaselen, filelen, efilelen, symlen, esymlen;
	unsigned int uidlen, struidlen, gidlen, strgidlen, modlen, strmodlen, sizelen, inodelen;
	char *buf, *ptr, *aptr, *bptr, *baseptr, *efile, *ebaseptr, *sym, *esym;
	char *uid, *struid, *gid, *strgid, *mod, *strmod, *size, *inode;

	/* for "%F", and used to calculate "%P". */
	baseptr = sample_basename(file);

	/* %F/%S just call these once and save the processing. */
	filelen = strlen(file);
	baselen = strlen(baseptr);

	/* for "%f", and used to calculate "%p". */
	efile = sampled_escape_string(file);
	ebaseptr = sample_basename(efile);

	/* %f/%s just call these once and save the processing. */
	efilelen = strlen(efile);
	ebaselen = strlen(ebaseptr);

	/* %l/%L symbolic link to file. */
	if(!(sym = (char *)malloc(SAM_BUFSIZE_LARGE + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for creating link path.");
	memset(sym, 0, SAM_BUFSIZE_LARGE + 1);
	if(readlink(file, sym, SAM_BUFSIZE_LARGE) > 0) {
		symlen = strlen(sym);	
		esym = sampled_escape_string(sym);
		esymlen = strlen(esym);
	}

	/* no link (of any size)? default to the original file. (same as %f/%F) */
	else {
		sym = file;
		symlen = filelen;
		esym = efile;
		esymlen = efilelen;
	}

	/* %u/%U/%g/%G/%m/%M/%n/%i veriables. */
	uid = sampled_get_file_attr(file, SAM_GETATTR_UID);
	struid = sampled_get_file_attr(file, SAM_GETATTR_STRUID);
	gid = sampled_get_file_attr(file, SAM_GETATTR_GID);
	strgid = sampled_get_file_attr(file, SAM_GETATTR_STRGID);
	mod = sampled_get_file_attr(file, SAM_GETATTR_MOD);
	strmod = sampled_get_file_attr(file, SAM_GETATTR_STRMOD);
	size = sampled_get_file_attr(file, SAM_GETATTR_SIZE);
	inode = sampled_get_file_attr(file, SAM_GETATTR_INODE);

	/* keep the lengths too, save re-call time. */
	uidlen = strlen(uid);
	struidlen = strlen(struid);
	gidlen = strlen(gid);
	strgidlen = strlen(strgid);
	modlen = strlen(mod);
	strmodlen = strlen(strmod);
	sizelen = strlen(size);
	inodelen = strlen(inode);

	/* calculate the buffer size for the final string. */
	ptr = str;
	len = 0;
	while(*ptr) {
		if(*ptr == '%' && *(ptr + 1) && strchr("%sSfFpPlLuUgGmMni0123456789", *(ptr + 1))) {
			ptr++;

			/* "%%" = "%", no length to account for. */

			/* the follow (%s/%f/%p/%l) lowercase are shell-escaped, uppercase are unescaped. */
			/* "%s/%S" = full file path. (-1 for replacing the current character) */
			if(*ptr == 's') len += efilelen - 1;
			else if(*ptr == 'S') len += filelen - 1;

			/* "%f/%F" = just the filename. (-1 for replacing the current character) */
			else if(*ptr == 'f') len += ebaselen - 1;
			else if(*ptr == 'F') len += baselen - 1;

			/* "%p/%P" = just the path. (-1 for replacing the current character) */
			else if(*ptr == 'p' && efilelen - ebaselen > 0) len += efilelen - ebaselen - 1;
			else if(*ptr == 'P' && filelen - baselen > 0) len += filelen - baselen - 1;

			/* "%l/%L" = symbolic link path. (-1 for replacing the current character) */
			else if(*ptr == 'l') len += esymlen - 1;
			else if(*ptr == 'L') len += symlen - 1;

			/* "%u/%U" = uid integer/name (-1 for replacing the current character) */
			else if(*ptr == 'u') len += uidlen - 1;
			else if(*ptr == 'U') len += struidlen - 1;

			/* "%g/%G" = gid integer/name (-1 for replacing the current character) */
			else if(*ptr == 'g') len += gidlen - 1;
			else if(*ptr == 'G') len += strgidlen - 1;

			/* "%m/%M" = mode integer/string (-1 for replacing the current character) */
			else if(*ptr == 'm') len += modlen - 1;
			else if(*ptr == 'M') len += strmodlen - 1;

			/* "%n" = size of file in bytes (-1 for replacing the current character) */
			else if(*ptr == 'n') len += sizelen - 1;

			/* "%i" = inode of file. */
			else if(*ptr == 'i') len += inodelen - 1;

			/* "%0-9" = pull apart the directory tree into arguments. (-1 for replacing the current character) */
			else {
				if((aptr = sampled_system_string_format_numeric(file, *ptr - 0x30))) {
					len += strlen(aptr) - 1;
					free(aptr);
				}
			}
		}
		ptr++;
		len++;
	}
	
	/* now make the buffer, and write to it. */
	if(!(buf = (char *)malloc(len + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for parsing sampled time.");
	memset(buf, 0, len + 1);
	bptr = buf;
	ptr = str;
	while(*ptr) {
		if(*ptr == '%' && *(ptr + 1) && strchr("%sSfFpPlLuUgGmMni0123456789", *(ptr + 1))) {
			ptr++;
			if(*ptr == '%') *bptr++ = '%';
			else if(*ptr == 's') {
				memcpy(bptr, efile, efilelen);
				bptr += efilelen;
			}
			else if(*ptr == 'S') {
				memcpy(bptr, file, filelen);
				bptr += filelen;
			}
			else if(*ptr == 'f') {
				memcpy(bptr, ebaseptr, ebaselen);
				bptr += ebaselen;
			}
			else if(*ptr == 'F') {
				memcpy(bptr, baseptr, baselen);
				bptr += baselen;
			}
			else if(*ptr == 'p' && efilelen - ebaselen > 0) {
				memcpy(bptr, efile, efilelen - ebaselen);
				bptr += efilelen - ebaselen;
			}
			else if(*ptr == 'P' && filelen - baselen > 0) {
				memcpy(bptr, file, filelen - baselen);
				bptr += filelen - baselen;
			}
			else if(*ptr == 'l') {
				memcpy(bptr, esym, esymlen);
				bptr += esymlen;
			}
			else if(*ptr == 'L') {
				memcpy(bptr, sym, symlen);
				bptr += symlen;
			}
			else if(*ptr == 'u') {
				memcpy(bptr, uid, uidlen);
				bptr += uidlen;
			}
			else if(*ptr == 'U') {
				memcpy(bptr, struid, struidlen);
				bptr += struidlen;
			}
			else if(*ptr == 'g') {
				memcpy(bptr, gid, gidlen);
				bptr += gidlen;
			}
			else if(*ptr == 'G') {
				memcpy(bptr, strgid, strgidlen);
				bptr += strgidlen;
			}
			else if(*ptr == 'm') {
				memcpy(bptr, mod, modlen);
				bptr += modlen;
			}
			else if(*ptr == 'M') {
				memcpy(bptr, strmod, strmodlen);
				bptr += strmodlen;
			}
			else if(*ptr == 'n') {
				memcpy(bptr, size, sizelen);
				bptr += sizelen;
			}
			else if(*ptr == 'i') {
				memcpy(bptr, inode, inodelen);
				bptr += inodelen;
			}
			else {
				if((aptr = sampled_system_string_format_numeric(file, *ptr - 0x30))) {
					memcpy(bptr, aptr, strlen(aptr));
					bptr += strlen(aptr);
					free(aptr);
				}
				/* if nothing exists it will fill nothing where the "%0-9" was. */
			}
		}
		else *bptr++ = *ptr;
		ptr++;
	}
	*bptr = 0;

	/* clean up shop. */
	free(efile);
	free(uid);
	free(struid);
	free(gid);
	free(strgid);
	free(mod);
	free(strmod);
	free(size);
	free(inode);

	/* sym could just be pointing to file (not allocated), would cause a double-free. */
	if(sym != file) {
		free(sym);
		free(esym);
	}

	return(buf);
}

/* returns a shell-safe version of a string. */
char *sampled_escape_string(char *str) {
	unsigned int i;
	char *esc, *ptr, *eptr;

	ptr = str;

	/* add up the extra space needed for the escaped version. */
	for(i = 0, ptr = str; *ptr; i++, ptr++) {
		if(strchr(SAM_ESCAPE_CHARS, *ptr)) i++;
	}

	if(!(esc = (char *)malloc(i + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for shell-escaped string.");
	memset(esc, 0, i + 1);

	/* now write it out. */
	for(ptr = str, eptr = esc; *ptr; ptr++) {
		if(strchr(SAM_ESCAPE_CHARS, *ptr)) *eptr++ = '\\';
		*eptr++ = *ptr;
	}

	return(esc);
}
