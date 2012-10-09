/* [sampled] env.c :: environmental variable handling for sampled.
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
char **sampled_env;
unsigned int sampled_env_size = 0;


/* just builds a buffer and passes it to sampled_putenv(). */
void sampled_setenv(char *entry, char *value) {
	char *var;
	if(!(var = (char *)malloc(strlen(entry) + strlen(value) + 2)))
		sample_error(SAM_ERR, "Failed to allocate memory for parsing sampled time.");
	sprintf(var, "%s=%s", entry, value);
	sampled_putenv(var);
	free(var);
	return;
}

/* doesn't actually put anything in the evironment, just gets it ready for the exec() pass. */
void sampled_putenv(char *var) {
	unsigned int i;
	char *name;

	/* need an '=' and at least 1 character for the variable name. */
	name = strchr(var, '=');
	if(!name || var == name) return;

	/* new environment, start fresh. */
	if(!sampled_env_size) {
		if(!(sampled_env = (char **)malloc(sizeof(char *) * 2)))
			sample_error(SAM_ERR, "Failed to allocate memory for environmental array.");
	}

	/* exists already, reallocate and add to it. */
	else {

		/* is this variable already set? if so overwrite it. */
		for(i = 0; i < sampled_env_size; i++) {
			name = strchr(sampled_env[i], '=');
			if(!strncmp(name, sampled_env[i], (name - sampled_env[i]))) {
				free(sampled_env[i]);
				if(!(sampled_env[i] = (char *)malloc(strlen(var) + 1)))
					sample_error(SAM_ERR, "Failed to re-allocate memory for environmental entry.");
				strcpy(sampled_env[i], var);
				return;
			}
		}

		/*  not set already, so make space for a new entry. */
		if(!(sampled_env = (char **)realloc(sampled_env, sizeof(char *) * (sampled_env_size  + 2))))
			sample_error(SAM_ERR, "Failed to re-allocate memory for environmental array.");
	}

	/* add the variable. */
	if(!(sampled_env[sampled_env_size] = (char *)malloc(strlen(var) + 1)))
		sample_error(SAM_ERR, "Failed to re-allocate memory for environmental array.");

	strcpy(sampled_env[sampled_env_size++], var);
	sampled_env[sampled_env_size] = NULL;

	return;
}
