#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct msgbuf {
    long mtype;
    char mtext[100];
};

int main() {
    int msqid;
    key_t key = ftok(".", 'a');
    msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }

    struct msgbuf msg;
    msg.mtype = 1;
    strcpy(msg.mtext, "Hello, world!");

    if (msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    printf("Message sent: %s\n", msg.mtext);

    return 0;
}