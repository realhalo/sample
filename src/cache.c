/* [sampled] cache.c :: functions for caching sample users. (resource saver)
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
struct sampled_cache_struct **sampled_cache;
unsigned int sampled_cache_size = 0;


/* create / add to sampled user cache. */
void sampled_cache_add(struct passwd *pwptr, struct passwd pw) {
	struct passwd *pwd;

	if(!pwptr) pwd = &pw;
	else pwd = pwptr;

	if(!sampled_cache_size) {
		if(!(sampled_cache = (struct sampled_cache_struct **)malloc(sizeof(struct sampled_cache_struct *) * 2)))
			sample_error(SAM_ERR, "Failed to allocate memory for cache.");
	}
	else {
		if(!(sampled_cache = (struct sampled_cache_struct **)realloc(sampled_cache, sizeof(struct sampled_cache_struct *) * (sampled_cache_size + 2))))
			sample_error(SAM_ERR, "Failed to re-allocate memory for cache.");
	}

	if(!(sampled_cache[sampled_cache_size] = (struct sampled_cache_struct *)malloc(sizeof(struct sampled_cache_struct) + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for pwd entry.");

	/* username. */
	if(!(sampled_cache[sampled_cache_size]->pw_name = (char *)malloc(strlen(pwd->pw_name) + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for cache entry.");
	memset(sampled_cache[sampled_cache_size]->pw_name, 0, strlen(pwd->pw_name) + 1);
	strcpy(sampled_cache[sampled_cache_size]->pw_name, pwd->pw_name);

	/* uid/gid. */
	sampled_cache[sampled_cache_size]->pw_uid = pwd->pw_uid;
	sampled_cache[sampled_cache_size]->pw_gid = pwd->pw_gid;

	/* home dir. */
	if(!(sampled_cache[sampled_cache_size]->pw_dir = (char *)malloc(strlen(pwd->pw_dir) + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for cache entry.");
	memset(sampled_cache[sampled_cache_size]->pw_dir, 0, strlen(pwd->pw_dir) + 1);
	strcpy(sampled_cache[sampled_cache_size]->pw_dir, pwd->pw_dir);

	/* shell. */
	if(!(sampled_cache[sampled_cache_size]->pw_shell = (char *)malloc(strlen(pwd->pw_shell) + 1)))
		sample_error(SAM_ERR, "Failed to allocate memory for cache entry.");
	memset(sampled_cache[sampled_cache_size]->pw_shell, 0, strlen(pwd->pw_shell) + 1);
	strcpy(sampled_cache[sampled_cache_size]->pw_shell, pwd->pw_shell);

	/* "~/.sample" */
	if(!(sampled_cache[sampled_cache_size]->sample_file = (char *)malloc(strlen(SAM_FILENAME) + strlen(pwd->pw_dir) + 2)))
		sample_error(SAM_ERR, "Failed to allocate memory for cache entry.");
	sprintf(sampled_cache[sampled_cache_size]->sample_file, "%s/" SAM_FILENAME, pwd->pw_dir);

	/* "~/.sample_spool" (directory) */
	if(!(sampled_cache[sampled_cache_size]->sample_spooldir = (char *)malloc(strlen(SAM_SPOOLDIR) + strlen(pwd->pw_dir) + 2)))
		sample_error(SAM_ERR, "Failed to allocate memory for cache entry.");
	sprintf(sampled_cache[sampled_cache_size]->sample_spooldir, "%s/" SAM_SPOOLDIR, pwd->pw_dir);

	/* "/etc/sampletab/<username>" */
	if(!(sampled_cache[sampled_cache_size]->sample_tab = (char *)malloc(strlen(SAM_TABDIR) + strlen(pwd->pw_name) + 2)))
		sample_error(SAM_ERR, "Failed to allocate memory for cache entry.");
	sprintf(sampled_cache[sampled_cache_size]->sample_tab, SAM_TABDIR "/%s", pwd->pw_name);

	sampled_cache_size++;

	sampled_cache[sampled_cache_size] = 0;
	return;
}

/* create / add to sampled user cache. (passwd pointer) */
void sampled_cache_add_pwptr(struct passwd *pwptr) {
	struct passwd pw;
	sampled_cache_add(pwptr, pw);
	return;
}

/* create / add to sampled user cache. (passwd) */
void sampled_cache_add_pw(struct passwd pw) {
	sampled_cache_add(NULL, pw);
	return;
}

/* free() cache if it exists. */
void sampled_cache_free() {
	unsigned int i = 0;

	/* free all cached entries. */
	if(sampled_cache_size) {
		for(i = 0; i < sampled_cache_size; i++) {
			free(sampled_cache[i]->pw_name);
			free(sampled_cache[i]->pw_dir);
			free(sampled_cache[i]->pw_shell);
			free(sampled_cache[i]->sample_file);
			free(sampled_cache[i]->sample_spooldir);
			free(sampled_cache[i]->sample_tab);
			free(sampled_cache[i]);
		}
		free(sampled_cache);
	}

	/* free()'d, let it be known. */
	sampled_cache = 0;
	sampled_cache_size = 0;
	return;
}
