
#include <stdio.h>
#include <string.h>

#include "cftools.h"

void c_fcerr(char *errmsg)
{
	fprintf(stderr, "%s\n", errmsg);
}

static char taskname[40];

void c_ptaskn(char *name)
{
	if (name) {
		strncpy(taskname, name, sizeof(taskname) - 1);
		taskname[sizeof(taskname) - 1] = 0;
	}
}
