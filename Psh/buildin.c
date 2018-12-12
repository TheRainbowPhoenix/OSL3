/*
 * PSH BUILTIN
 *
 * even if we don't need thoses, we got it
 */
#include "structs.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

extern char* builtin[];

int isBuiltin(process *p) {
  for (size_t i = 0; builtin[i]; i++) {
    if(strcmp(builtin[i], p->argv[0]) == 0) return 1;
  }
  return 0;
}

/*
 * THE FUNCTIONS
 */

 int cd(char **args) {
 	char	*home;
 	if(!args[1]) {
 		changeDir(_getENV("HOME"));
 		return 0;
 	}
  if(args[1][0] == '~') {
    changeDir(_getENV("HOME"));
    return 0;
  }
 	if(checkPath(args[1])) {
 		return changeDir(args[1]);
 	}
 	return 1;
 }

int builtin_main(int argc, char *argv[]) {
  printf("[47m[90m BUILTIN MAIN TEST [37m[49m\n");
  char cwd[256];
  char** cd0 = (char *[]){"cd",".."};
  char** cd1 = (char *[]){"cd","Psh"};
  cd(cd0);
  getcwd(cwd, sizeof(cwd));
  printf("%s\n", cwd);
  cd(cd1);
  getcwd(cwd, sizeof(cwd));
  printf("%s\n", cwd);
  return 0;
}
