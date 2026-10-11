#include <stdio.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <string.h>

struct msgbuf {
               long mtype;       /* message type, must be > 0 */
               int mtext[1];    /* message data */
           };

enum send_t {
    NONE,
    NUM,
    STICK
};

void Judge(int amount_of_players, int id);

int main()
{
    //создаем общую очередь сообщений
    int id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0700);
    int amount_of_players = 5;

    //make process for judge
    int p = fork();

    if (p == 0)
    {
        Judge(amount_of_players, id);
        exit(0);
    }

    for (int i = 0; i < amount_of_players; i++)
    {
        //make process for player
        int p = fork();

        if (p == 0)
        {
            Player(i + 1, id);
            exit(0);
        }
    }
    wait(0);
    msgctl(id, IPC_RMID, 0);
    return 0;
}

void Judge(int amount_of_players, int id)
{
    //start reg
    printf("J:\tstart reg\n");
    for (int i = 0; i < amount_of_players; i++)
    {
        struct msgbuf buf;
        ssize_t bytes_read = msgrcv(id, &buf, sizeof(buf.mtext), NUM, 0);

        if (bytes_read < 0) 
        {
            perror("msgrcv");
        } 
        else 
        {
            printf("J:\tplayer %d is there\n", buf.mtext[0]);
        }
    }
    printf("J:\tall runners are there\n");
    //start run
    //send stick to the first player
    struct msgbuf buf;
    buf.mtype = STICK + 1; 
    buf.mtext[0] = 1;

    printf("J:\tstart running\n");

    if (msgsnd(id, &buf, sizeof(buf.mtext), 0) < 0) 
    {
        perror("msgsnd");
    }
    //get stick from the last player
    ssize_t bytes_read = msgrcv(id, &buf, sizeof(buf.mtext), STICK + amount_of_players + 1, 0);
    printf("J:\tfinish\n");
}

void Player(int i, int id)
{
    struct msgbuf buf;
    buf.mtype = NUM; 
    buf.mtext[0] = i;
    
    //send reg-message
    printf("P(%d):\tI'm coming\n", i);
    if (msgsnd(id, &buf, sizeof(buf.mtext), 0) < 0) 
    {
        perror("msgsnd");
    }
    //get stick with num == i
    ssize_t bytes_read = msgrcv(id, &buf, sizeof(buf.mtext), STICK + i, 0);
    //runner starts running
    printf("P(%d):\tI'm running\n", i);
    //send stick to the next
    buf.mtype = STICK + i + 1;

    if (msgsnd(id, &buf, sizeof(buf.mtext), 0) < 0) 
    {
        perror("msgsnd");
    }
    printf("P(%d):\tI'm sending stick to %d\n", i, i + 1);

}