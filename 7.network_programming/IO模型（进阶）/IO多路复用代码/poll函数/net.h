#ifndef _NET_H_
#define _NET_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <poll.h>

typedef struct sockaddr Addr;
typedef struct sockaddr_in Addr_in;
#define BACKLOG 5
#define ErrExit(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
#define MAX_SOCK_FD 1024

void Argment(int argc, char *argv[]);
int CreateSocket(char *argv[]);
int DataHandle(int fd, struct sockaddr_in *client_addr);


#endif