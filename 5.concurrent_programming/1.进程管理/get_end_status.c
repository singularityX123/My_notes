#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
	switch( fork() ) {
	case -1:
		perror("fork");
		exit(EXIT_FAILURE);
		break;
	case 0:
		return 1;
		break;
	default:
	}
	int wstatus;
	if(wait(&wstatus) == -1) {
		perror("wait");
		exit(EXIT_FAILURE);
	}
	if(WIFEXITED(wstatus)) {
		printf("%d\n", WEXITSTATUS(wstatus));
	}
}