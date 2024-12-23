#ifndef readfile_h
#define readfile_h

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

char* readfile(char* name);

#endif

#if __INCLUDE_LEVEL__ == 0

char* readfile(char* name)
{
    FILE* f      = fopen(name, "rb");
    char* string = NULL;

    if (f != NULL)
    {

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET); /* same as rewind(f); */

	string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;
    }

    return string;
}

#endif
