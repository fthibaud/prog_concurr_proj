#include "BE.h"

void *Th1(void *arg)
{
    /*char message[5];*/
    aboMsg(1);
    sleep(10);
    sendMsg(1,2,"Bonjour, je m'appelle 1");
    /*recvMsg(1, message, "RECEV");*/
    pthread_exit(NULL);
};

void *Th2(void *arg)
{
    char message[5];
    aboMsg(2);
    sleep(5);
    recvMsg(2, message, "RECEV");
    sleep(5);
    /*sendMsg(2,1,"Bonjour 1 je suis 2 j'ai bien reçu votre message");*/
    recvMsg(2, message, "COUNT");

    pthread_exit(NULL);
};


/*int main()
{
    initMsg(10);
    sleep(1);
    pthread_t idTh1;
    pthread_t idTh2;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&idTh1, &attr, Th1, NULL)!=0)
    {
        printf("Erreur lors de la création de la tâche gestionnaire\n");
        exit(1);
    };
    if (pthread_create(&idTh2, &attr, Th2, NULL)!=0)
    {
        printf("Erreur lors de la création de la tâche gestionnaire\n");
        exit(1);
    };
    pthread_join(idTh1, NULL);
    pthread_join(idTh2, NULL);
    while (1)
    {
        sleep(1);
    }
    finMsg("FORCE");
    return 0;
};*/

int main()
{
    printf("Init...\n");
    initMsg(50);
    sleep(1);
    aboMsg(10);
    aboMsg(20);
    char message[5];
    sendMsg(10,20,"Bonjour, je m'appelle 1");
    recvMsg(20, message, "RECEV");
    sleep(2);
    recvMsg(20, message, "COUNT");
    pthread_join(idThGest, NULL);
    return 0;
};

