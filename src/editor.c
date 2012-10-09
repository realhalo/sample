/* [sample] editor.c :: editor functions for editing ~/.sample files.
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

/* defacto editors to try and use if -e/$EDITOR isn't set. */
static char *sample_editors[] = {
	"vi",
	"vim",
	"nano",
	"pico",
	"elvis",
	"joe",
	"jed",
	"ed",
	NULL
};

/* executes the defined editor and waits for it to exit. */
void sample_run_editor(char *sample_file, char *editor) {
	pid_t pid;
	switch((pid = fork())) {
		case -1:
			sample_error(SAM_WRN, "Editor fork() failed.");
			return;
			break;
		case 0:

			/* run it. */
			execl(editor, sample_basename(editor), sample_file, (char *)0);

			/* shouldn't make it here, bad shell. */
			exit(SAM_EXIT_ERROR);
			break;
		default:
			/* wait for a return, if it errors let it go. */
			while(
#ifdef HAVE_WAITPID
			!waitpid(pid, NULL, WNOHANG)
#else
			!wait4(pid, NULL, WNOHANG, NULL)
#endif
			)
#ifdef HAVE_USLEEP
			usleep(100000);
#else
			sleep(1);
#endif
			break;
	}
	return;
}

/* finds an editor in existence from the static list. (hopefully) */
/* if *explicit is not null, it will use it as a filename instead of *sample_editors[] */
char *sample_get_editor(char *path, char *explicit) {
	unsigned int i, j, k;
	char **paths, *editor, *ptr, *lptr;

	ptr = path;
	i = 1;

	/* how many paths are there? */
	while(*ptr) {
		if(*ptr == ':' && *(ptr + 1) && *(ptr + 1) != ':') i++;
		ptr++;
	}
	if(!(paths = (char **)malloc(sizeof(char *) * i + 2)))
		sample_error(SAM_ERR, "Sample failed to allocate memory for creating paths.");

	lptr = ptr = path;
	i = 0;

	/* make all the paths into an array, to be easily accessed. */
	while(SAM_TRUE) {
		if(!*ptr || *ptr == ':') {
			if(lptr == ptr) lptr = ptr + 1;
			else {
				if(!(paths[i] = (char *)malloc(ptr - lptr + 1)))
					sample_error(SAM_ERR, "Sample failed to allocate memory for creating paths.");
				memset(paths[i], 0 , ptr - lptr + 1);
				strncpy(paths[i++], lptr, ptr - lptr);
				lptr = ptr + 1;
			}
			if(!*ptr) break;
		}
		ptr++;
	}

	editor = 0;

	/* explicit filename of editor supplied. */
	if(explicit) {
		for(j = 0; j < i; j++) {
			if(!(editor = (char *)malloc(strlen(paths[j]) + strlen(explicit) + 2)))
				sample_error(SAM_ERR, "Sample failed to allocate memory for creating editor path.");
			memset(editor, 0, strlen(paths[j]) + strlen(explicit) + 2);
			sprintf(editor, "%s/%s", paths[j], explicit);

			/* found it. */
			if(!access(editor, X_OK)) break;
			free(editor);
			editor = 0;
		}
	}

	/* try to find generic editor. */
	else {
		k = 0;

		/* find our editor. */
		while(!editor && sample_editors[k]) {
			for(j = 0; j < i; j++) {
				if(!(editor = (char *)malloc(strlen(paths[j]) + strlen(sample_editors[k]) + 2)))
					sample_error(SAM_ERR, "Sample failed to allocate memory for creating editor path.");
				memset(editor, 0, strlen(paths[j]) + strlen(sample_editors[k]) + 2);
				sprintf(editor, "%s/%s", paths[j], sample_editors[k]);

				/* found it. */
				if(!access(editor, X_OK)) break;
				free(editor);
				editor = 0;
			}
			k++;
		}
	}

	/* cleanup. */
	for(j = 0; j < i; j++) free(paths[j]);
	free(paths);

	/* can be null. */
	return(editor);
}
