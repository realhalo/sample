/* [sampled] wpid.c :: pid monitoring functions for sampled.
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
struct wpid_struct wpids;


/* this will add/modify a user's last child pid, to be used to monitor what is running. */
void sampled_wpid_entry(char *user, pid_t pid) {
	unsigned int i, slot;
	slot = 0;

	/* must be the first user, allocate the whole structure. */
	if(!wpids.size) {
		if(!(wpids.entries = (struct wpid_entry **)malloc(sizeof(struct wpid_entry *) * 2)))
			sample_error(SAM_ERR, "Failed to allocate memory for watchpid array.");
		slot = 0;
	}

	/* just another user, first see if the user exists, then re-allocate.  */
	else {

		/* see if the user already exists. */
		for(i = 0; i < wpids.size; i++) {

			/* already exists, replace this pid in it's spot and return. */
			if(!strcmp(user, wpids.entries[i]->user)) {
				wpids.entries[i]->pid = pid;
				return;
			}
		}

		/* new user, re-allocate. */
		if(!slot) {
			if(!(wpids.entries = (struct wpid_entry **)realloc(wpids.entries, sizeof(struct wpid_entry *) * (wpids.size + 2))))
				sample_error(SAM_ERR, "Failed to re-allocate memory for watchpid array.");
			slot = wpids.size;
		}
	}

	/* allocate a new user/pid entry pair. */
	if(!(wpids.entries[slot] = (struct wpid_entry *)malloc(sizeof(struct wpid_entry) + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for watchpid element.");

	/* allocate the user buffer. */
	if(!(wpids.entries[slot]->user = (char *)malloc(strlen(user) + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for watchpid username element.");

	/* write the values. */
	strcpy(wpids.entries[slot]->user, user);
	wpids.entries[slot]->pid = pid;

	wpids.size++;

	return;
}

/* gets the child pid for the user. */
pid_t sampled_wpid_get(char *user) {
	unsigned int i;

	/* see if there is a user record for the person. */
	for(i = 0; i < wpids.size; i++) {

		/* found the user, return their last pid. */
		if(!strcmp(user, wpids.entries[i]->user)) {
			return(wpids.entries[i]->pid);
		}
	}

	/* nothing found, assume all is well. */
	return(0);
}
