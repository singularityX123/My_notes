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
    struct sockaddr_un my_addr, peer_addr;
    socklen_t peer_addr_size;
    char buf[BUFSIZ] = {};
 
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1)
        handle_error("socket");
 
    memset(&my_addr, 0, sizeof(struct sockaddr_un));
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, MY_SOCK_PATH,
            sizeof(my_addr.sun_path) - 1);
 
    if (bind(fd, (struct sockaddr *) &my_addr,
                sizeof(struct sockaddr_un)) == -1)
        handle_error("bind");
 
    peer_addr_size = sizeof(struct sockaddr_un);
    recvfrom(fd, buf, BUFSIZ, 0, (struct sockaddr *) &peer_addr,
            &peer_addr_size);
    printf("%s\n",buf);
    close(fd);
    remove(MY_SOCK_PATH);
    return 0;
}