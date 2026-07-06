#ifndef _UDP_H_
#define _UDP_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

#define ErrExit(msg) printf("[%s:%d]%s:%s", __FUNCTION__, __LINE__,msg, strerror(errno)), exit(EXIT_FAILURE)

#endif


