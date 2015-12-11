#include "BE_v2.h"

void *Th1(void *arg)
{
    char message[20];
    int ident=1;
    aboMsg(ident);
    sleep(1);
    sendMsg(ident,2,"Bonjour, je m'appelle");
    sendMsg(ident,2,"Th1");

    recvMsg(ident, message, "RECEV");
    printf("Le message 3 est : %s \n", message);

    recvMsg(ident, message, "RECEV");
    printf("Le message 4 est : %s \n", message);

    pthread_exit(NULL);
};

void *Th2(void *arg)
{
    char message[20];
    int ident=2;
    aboMsg(ident);

    recvMsg(ident, message, "RECEV");
    printf("Le message 1 est : %s \n", message);

    recvMsg(ident, message, "COUNT");
    printf("Nombre de messages dans la BAL : %s \n", message);

    recvMsg(ident, message, "RECEV");
    printf("Le message 2 est : %s \n", message);

    recvMsg(ident, message, "COUNT");
    printf("Nombre de messages dans la BAL : %s \n", message);

    sendMsg(ident,1,"Et moi je m'appelle");
    sendMsg(ident,1,"Th2");

    desaboMsg(ident);

    pthread_exit(NULL);
};


int main()
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
    //finMsg("FORCE");
    return 0;
};
/*
int main()
{
    printf("Init...\n");
    initMsg(50);
    sleep(1);
    aboMsg(10);
	printf("envoi 2\n");
    aboMsg(20);
    char message[5];
    sendMsg(10,20,"Bonjour, je m'appelle 1");
    recvMsg(20, message, "RECEV");
    sleep(2);
    recvMsg(20, message, "COUNT");
    pthread_join(idThGest, NULL);
    return 0;
};*/

