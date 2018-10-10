#include <stdio.h>

int	ft_atoi(char *str)
{
	int rslt;
	int sgn;

	rslt = 0;
	sgn = 1;
	while (*str == ' ' || (*str >= 9 && *str <= 13))
		str++;
	if (*str == '-')
		sgn = -1;
	if (*str == '-' || *str == '+')
		str++;
	while (*str >= '0' && *str <= '9')
	{
		rslt = rslt * 10 + *str - '0';
		str++;
	}
	return (sgn * rslt);
}

int main(int argc, char *argv[]) {
	if(argc<=1) {
		printf("Invalid argument count : got %d expected 1 or more\n", argc-1);
		return 1;
	}
	int i = 0;
	while(*argv != '\0') {
		i+= ft_atoi(*argv++);
	}
	printf("%d\n",i); 
	
	return 0;
}
