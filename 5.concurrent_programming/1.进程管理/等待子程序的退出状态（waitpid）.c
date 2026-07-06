#include <sys/wait.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	pid_t cpid = fork();
	if (cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (cpid == 0) {
		sleep(1);
		exit(EXIT_SUCCESS);
	} else {
		printf("cpid = %d\n", cpid);
		pid_t wpid = waitpid(cpid, NULL, 0);
		printf("wpid = %d\n", wpid);
		exit(EXIT_SUCCESS);
	}
	return 0;
}