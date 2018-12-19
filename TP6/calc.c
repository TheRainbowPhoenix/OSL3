#include <stdio.h>

int main(int argc, char const *argv[]) {
  int a, b;
  char c;

  int run = 1;

  while (run) {
    scanf("%d %c %d", &a, &c, &b );
    switch (c) {
      case '+': printf("%d\n", (a+b)); break;
      case '-': printf("%d\n", (a-b)); break;
      case '*': printf("%d\n", (a*b)); break;
      case '/': printf("%d\n", (a/b)); break;
      default: printf("invalid\n"); run = 0; break;
    }
  }
  return 0;
}
