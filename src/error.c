/* [sample/samples] error.c :: error handling for sample/samples. (shared function name)
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


/* record errors, and exit if needed. (different for sampled) */
void sample_error(unsigned char type, char *fmt, ...) {
	char buf[SAM_BUFSIZE_LARGE + 1];
	va_list ap;
	memset(buf, 0, SAM_BUFSIZE_LARGE);
	va_start(ap, fmt);
	vsnprintf(buf, SAM_BUFSIZE_LARGE, fmt, ap);
	va_end(ap);
	if(type == SAM_REG) puts(buf);
	else printf("%s :: %s\n", (type == SAM_ERR ? "ERROR" : "WARNING"),  buf);
	if(type == SAM_ERR) exit(SAM_EXIT_ERROR);
	return;
}

