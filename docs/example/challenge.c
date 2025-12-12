#include <stdio.h>
#include <stdlib.h>

int main()
{
	const char *flag = getenv("FLAG");
	printf("Welcome! The flag is: %s\n", flag ? flag : "FLAG not set");
	return 0;
}