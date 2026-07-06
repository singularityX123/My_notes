#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
 
#define MY_SOCK_PATH "/tmp/my_sock_file"
 
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
 
int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_un peer_addr;
    char buf[BUFSIZ] = {"Hello World!"};
 
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
        handle_error("socket");
 
    memset(&peer_addr, 0, sizeof(struct sockaddr_un));
    peer_addr.sun_family = AF_UNIX;
    strncpy(peer_addr.sun_path, MY_SOCK_PATH,
            sizeof(peer_addr.sun_path) - 1);
 
    if (connect(fd, (struct sockaddr *) &peer_addr,
                sizeof(struct sockaddr_un)) == -1)
        handle_error("connect");
 
    printf("%s\n",buf);
    send(fd, buf, strlen(buf), 0);
 
    close(fd);
    return 0;
}