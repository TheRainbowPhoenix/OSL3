#include <stdio.h>
#include <stdlib.h>

/*
 * Global usage functions
 *
 * Dunno why it is here :o
 */

void trace(char src[], char err[]) {
  printf("%s : %s\n",src, err);
  exit(EXIT_FAILURE);
}
