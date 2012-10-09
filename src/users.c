/* [sampled] users.c :: functions to handle (dis)allowed users in sampled.
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
char **sampled_users = 0;
unsigned int sampled_users_size = 0;
unsigned char sampled_usermode = SAM_USERMODE_NONE;

/* check to see if a user is allowed to have an executed ~/.sample file.  */
unsigned char sampled_users_allowed(char *user) {
	unsigned int i;

	/* user (dis)allow must not have been set, then all users are good. */
	if(sampled_usermode == SAM_USERMODE_NONE) return(SAM_TRUE);

	/* we have nothing, default to all users then.  otherwise sampled is running for nothing. */
	if(!sampled_users_size) return(SAM_TRUE);

	for(i = 0; i < sampled_users_size; i++) {
		if(!strcmp(sampled_users[i], user))

			/* if it's an allowed user return true, if a disallowed return false. */
			return((sampled_usermode == SAM_USERMODE_ALLOW ? SAM_TRUE : SAM_FALSE));
	}

	/* if it's an allowed user, we didnt find it, return false.  vice-versa for disallowed. */
	return((sampled_usermode == SAM_USERMODE_ALLOW ? SAM_FALSE : SAM_TRUE));
}

/* takes a comma-separated list of users and makes the list to compare to. */
void sampled_users_makelist(char *users, unsigned char mode) {
	unsigned int i;
	char *ptr, *lptr;

	/* the list must have already been made, no dupes. */
	if(sampled_usermode != SAM_USERMODE_NONE) return;

	i = 1;

	ptr = users;

	/* how many users are there? */
	while(*ptr) {
		if(*ptr == ':' && *(ptr + 1) && *(ptr + 1) != ':') i++;
		ptr++;
	}

	/* allocate it. */
	if(!(sampled_users = (char **)malloc(sizeof(char *) * i + 2)))
		sample_error(SAM_ERR, "Sample failed to allocate memory for creating user list.");

	lptr = ptr = users;
	i = 0;

	/* breaks itself on a null-byte, puts the users in the array. */
	while(SAM_TRUE) {
		if(!*ptr || *ptr == ':') {
			if(lptr == ptr) lptr = ptr + 1;
			else {
				if(!(sampled_users[i] = (char *)malloc(ptr - lptr + 1)))
					sample_error(SAM_ERR, "Sample failed to allocate memory for creating a user list entry.");
				memset(sampled_users[i], 0 , ptr - lptr + 1);
				strncpy(sampled_users[i++], lptr, ptr - lptr);
				lptr = ptr + 1;
			}
			if(!*ptr) break;
		}
		ptr++;
	}

	/* total number of users. */
	sampled_users_size = i;

	/* set the mode the userlist is. */
	sampled_usermode = mode;

	return;
}
