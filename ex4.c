#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

int	ft_strcmp(char *s1, char *s2)
{
	int i;

	i = 0;
	while ((s1[i] != '\0' || s2[i] != '\0') && s1[i] == s2[i])
		i++;
	return ((char)s1[i] - (char)s2[i]);
}

void handler(int sig) {
	if(sig == SIGINT) {
		char in[40];
		char * code = "exit";

		printf("Password : ");

		scanf("%s",in);
		
		while (strcmp(in, code) != 0) {
			printf("  [!] BAD PASSWORD\n");
			printf("Password : ");
			scanf("%s",in);
		}


		printf("\n Bye !\n");
		exit(0);
	}
}

int main(void) {
	int run = 1;

	if (signal(SIGINT, handler) == SIG_ERR) {
		printf("Failed !\n");
	}

	printf(" Welcome to myApp !\n");
	printf(" ==================\n");	
	printf("\n  Use ctrl+c to quit\n\n");

	while(run) {
		sleep(1);
	}

	return 0;
}
