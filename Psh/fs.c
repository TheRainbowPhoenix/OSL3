#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"

void putstr(char *s) {
	while (*s) {
		write(1, &*s++, 1);
	}
}
void _putchar(char c) {
	write(1, &c, 1);
}

int startsWith(char * p, char * s) {
	size_t lp = strlen(p);
	return strlen(s) < lp ? 0 : strncmp(p, s, lp) == 0;
}

int checkPath(char * path) {
	return 1; //TODO : Implements me !
}

int changeDir(char * path) {
	char *cwd;
	char buffer[UCHAR_MAX];
	cwd = getcwd(buffer, UCHAR_MAX);
	if (!chdir(path)) {
		setenv("OLDPWD", cwd, 1);
		setenv("PWD", path, 1);
		return 0;
	} else {
		putstr("[47m[90m cd [37m[49m ");
		if(access(path, F_OK) == -1) putstr("No such file or directory");
		else if (access(path, R_OK) == -1) putstr("Permission denied");
		else putstr("Not a directory");
		_putchar('\n');
		return 1;
	}
}
