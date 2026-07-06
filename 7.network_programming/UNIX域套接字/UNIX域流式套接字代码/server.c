#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
 
#define MY_SOCK_PATH "/tmp/my_sock_file"
#define LISTEN_BACKLOG 50
 
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
 
int main(int argc, char *argv[])
{
    int sfd, cfd;
    struct sockaddr_un my_addr, peer_addr;
    socklen_t peer_addr_size;
    char buf[BUFSIZ] = {};
 
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
        handle_error("socket");
 
    memset(&my_addr, 0, sizeof(struct sockaddr_un));
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, MY_SOCK_PATH,
            sizeof(my_addr.sun_path) - 1);
 
    if (bind(sfd, (struct sockaddr *) &my_addr,
                sizeof(struct sockaddr_un)) == -1)
        handle_error("bind");
    if (listen(sfd, LISTEN_BACKLOG) == -1)
        handle_error("listen");
 
    peer_addr_size = sizeof(struct sockaddr_un);
    cfd = accept(sfd, (struct sockaddr *) &peer_addr,
            &peer_addr_size);
    if (cfd == -1)
        handle_error("accept");
     
    recv(cfd, buf, BUFSIZ, 0);
    printf("%s\n", buf);
 
    close(cfd);
    close(sfd);
    remove(MY_SOCK_PATH);
    return 0;
}