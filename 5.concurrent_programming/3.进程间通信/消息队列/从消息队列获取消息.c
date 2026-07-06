#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>

struct msgbuf {
    long mtype;
    char mtext[100];
};

int main() {
    int msqid;
    key_t key = ftok(".", 'a');
    msqid = msgget(key, 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }

    struct msgbuf msg;
    if (msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }

    printf("Message received: %s\n", msg.mtext);

    msgctl(msqid, IPC_RMID, NULL);

    return 0;
}