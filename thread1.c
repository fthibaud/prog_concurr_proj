#include <BE.h>


void *Th1(void *arg)
{
    aboMsg(1);
    sendMsg(1,2,"Bonjour, je m'appelle 1");
    pthread_exit(NULL);
};void

Th2(void *arg)
{
    char message[5]
    aboMsg(2);
    sleep(5);
    recvMsg("COUNT", &message)
    recvMsg("RECEV", &message)
    sendMsg(2,1,"Bonjour 1 je suis 2 j'ai bien reçu votre message")
    pthread_exit(NULL);
};





void main()
{
    initMsg();
    pthread_t idTh1;
    pthread_t idTh2
    if (pthread_create(&idTh1, NULL, Th1, NULL)!=0)
    {
        printf("Erreur lors de la création de la tâche gestionnaire\n");
        exit(1);
    };
    if (pthread_create(&idTh2, NULL, Th2, NULL)!=0)
    {
        printf("Erreur lors de la création de la tâche gestionnaire\n");
        exit(1);
    };
    pthread_join(idTh1, NULL);
    pthread_join(idTh2, NULL);
}
