//#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define nb_limite_abo 100
#define nb_max_msg_bal 10
#define message_len 80

struct bal
{
    char ident;
    int pointeur1;
    int pointeur2;
    int nb_msg;
    char msgs[nb_max_msg_bal][message_len];
};

struct reponse
{
    int code_ret;
    char msg[message_len];
};

typedef struct
{
    int type; // 0=pas de requête, 1=abonnement, 2=envoi de message, 3=lecture des messages, 4=nombre de messages courant, 5 = désabonnement
    char idente;
    char identd;
    char msg[message_len];
    struct reponse* reponse;
} Requete;

int _lance = 0;
Requete _requete;
int _nb_abo_courant = 0;
int _nb_max_abo = 0;
struct bal _liste_bal[nb_limite_abo];

pthread_cond_t _crequete = PTHREAD_COND_INITIALIZER;
int _ecri_requete = 0;
pthread_cond_t _cabo = PTHREAD_COND_INITIALIZER;
int _ecri_cabo = 0;
pthread_cond_t _cmsg = PTHREAD_COND_INITIALIZER;
int _ecri_cmsg = 0;
pthread_cond_t _crcp = PTHREAD_COND_INITIALIZER;
int _ecri_crcp = 0;
pthread_cond_t _cdesabo = PTHREAD_COND_INITIALIZER;
int _ecri_cdesabo = 0;
pthread_cond_t _cfin = PTHREAD_COND_INITIALIZER;
int _ecri_cfin = 0;
pthread_cond_t _crcpb = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_cr = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cabo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cmsg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_crcp = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_crcpb = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cdesabo= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cfin= PTHREAD_MUTEX_INITIALIZER;
pthread_t idThGest;


int dans(char element, struct bal *liste)
{
    int i;
    for (i = 0; i < _nb_abo_courant; i++)
        if (element == (liste[i].ident))
        {
            return 1;
        }
    return 0;
};

int indice(char idente, struct bal *liste)
{
    int i;
    for (i = 0; i < _nb_abo_courant; i++)
        if (idente == (liste[i].ident))
        {
            return i;
        }
    return -1;
}

struct bal* addbal (char element, struct bal *liste)
{
    int i;
    struct bal* retour;
    for (i = 0; i < _nb_abo_courant; i++)
        if (element == (liste[i].ident))
        {
            retour = &(liste[i]);
        }
    return retour;
};

void *ThMsg(void *arg)
{
    struct bal* bal;
    bal = addbal(_requete.identd, _liste_bal);
    if ((*bal).nb_msg == 10)
    {
        _requete.reponse -> code_ret = -1;
        strcpy( (*_requete.reponse).msg, "Boite au lettre pleine\n");
    }
    else
    {
        (*bal).pointeur2 = ((*bal).pointeur2 + 1) % 10;
        (*bal).nb_msg ++;
        _requete.reponse -> code_ret = 0;
        strcpy( (*bal).msgs[(*bal).pointeur1], _requete.msg);
        strcpy( (*_requete.reponse).msg, "Message correctement envoyé");
    }

    pthread_mutex_lock(&_mutex_cmsg);
    _ecri_cmsg = 1;
    pthread_cond_signal(&_cmsg);
    pthread_mutex_unlock(&_mutex_cmsg);
    pthread_mutex_lock(&_mutex_crcpb);
    pthread_cond_signal(&_crcpb);
    pthread_mutex_unlock(&_mutex_crcpb);
    pthread_exit(NULL);
};


void *ThRcp(void *arg)
{
    struct bal* bal;
    bal = addbal(_requete.idente, _liste_bal);
    pthread_mutex_lock(&_mutex_crcpb);
    while ((*bal).nb_msg == 0)
    {
        pthread_cond_wait(&_crcpb, &_mutex_crcpb) ;
    }
    pthread_mutex_unlock(&_mutex_crcpb);
    strcpy( (*_requete.reponse).msg, (*bal).msgs[(*bal).pointeur1]);
    (*bal).pointeur1 = ((*bal).pointeur1 + 1) % 10;
    (*bal).nb_msg --;
    _requete.reponse -> code_ret = 0;
    pthread_mutex_lock(&_mutex_crcp);
    _ecri_crcp = 1;
    pthread_cond_signal(&_crcp);
    pthread_mutex_unlock(&_mutex_crcp);
    pthread_exit(NULL);
};


void *ThRcp2(void *arg)
{
    struct bal* bal;
    bal = addbal(_requete.idente, _liste_bal);
    sprintf((*_requete.reponse).msg, "%d", (*bal).nb_msg);
    _requete.reponse -> code_ret = 0;
    pthread_mutex_lock(&_mutex_crcp);
    _ecri_crcp = 1;
    pthread_cond_signal(&_crcp);
    pthread_mutex_unlock(&_mutex_crcp);
    pthread_exit(NULL);
};

void *ThAbo(void *arg)
{
    _liste_bal[_nb_abo_courant].ident = _requete.idente;
    _liste_bal[_nb_abo_courant].pointeur1 = 0;
    _liste_bal[_nb_abo_courant].pointeur2 = 0;
    _liste_bal[_nb_abo_courant].nb_msg = 0;
    _nb_abo_courant++;
    _requete.reponse -> code_ret = 0;
    strcpy( (*_requete.reponse).msg, "Thread abonné avec succès\n");
    pthread_mutex_lock(&_mutex_cabo);
    _ecri_cabo = 1;
    pthread_cond_signal(&_cabo);
    pthread_mutex_unlock(&_mutex_cabo);
    pthread_exit(NULL);
};


void *ThDesabo(void *arg)
{
    int i;
    for (i = indice(_requete.idente, _liste_bal); i < _nb_abo_courant; i++)
    {
        _liste_bal[i] = _liste_bal[i+1];
    }
    _nb_abo_courant --;
    _requete.reponse -> code_ret = 0;
    strcpy( (*_requete.reponse).msg, "Thread désabonné avec succès\n");
    pthread_mutex_lock(&_mutex_cdesabo);
    _ecri_cdesabo = 1;
    pthread_cond_signal(&_cdesabo);
    pthread_mutex_unlock(&_mutex_cdesabo);
    pthread_exit(NULL);
};

void *ThGest()
{
    while(1)
    {
        pthread_mutex_lock(&_mutex_cr);
        while (_ecri_requete == 0)
        {
            pthread_cond_wait(&_crequete, &_mutex_cr) ;
        }
        _ecri_requete = 0;
        pthread_mutex_unlock(&_mutex_cr);
        if (_requete.type == 1)
        {
            if (dans(_requete.idente, _liste_bal))
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Identifiant déjà utilisé\n");
                pthread_mutex_lock(&_mutex_cabo);
                _ecri_cabo = 1;
                pthread_cond_signal(&_cabo);
                pthread_mutex_unlock(&_mutex_cabo);


            }
            else
            {
                pthread_t idThAbo;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if ( pthread_create(&idThAbo, &attr, ThAbo, NULL)!=0 ) //PTHREAD_CREATE_DETACHED à rajouter !!
                {
                    printf("Erreur lors de la création de la tâche abonnement\n");
                }
            }
        }

        if (_requete.type == 2)
        {
            if ( dans(_requete.idente, _liste_bal) && dans(_requete.identd, _liste_bal) )
            {
                pthread_t idThMsg;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if ( pthread_create(&idThMsg, &attr, ThMsg, NULL)!=0 ) //PTHREAD_CREATE_DETACHED à rajouter !!
                {
                    printf("Erreur lors de la création de la tâche envoie message\n");
                }
            }



            else
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "L'un des deux threads n'est pas abonnné\n");
                pthread_mutex_lock(&_mutex_cmsg);
                _ecri_cmsg = 1;
                pthread_cond_signal(&_cmsg);
                pthread_mutex_unlock(&_mutex_cmsg);


            }
        }

        if (_requete.type == 3)
        {
            if ( dans(_requete.idente, _liste_bal))
            {
                pthread_t idThRcp;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if ( pthread_create(&idThRcp, &attr, ThRcp, NULL)!=0 ) //PTHREAD_CREATE_DETACHED à rajouter !!
                {
                    printf("Erreur lors de la création de la tâche reception\n");
                }
            }



            else
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Cette identifiant n'a pas de boite aux lettres\n");
                pthread_mutex_lock(&_mutex_crcp);
                _ecri_crcp = 1;
                pthread_cond_signal(&_crcp);
                pthread_mutex_unlock(&_mutex_crcp);


            }
        }

        if (_requete.type == 4)
        {
            if ( dans(_requete.idente, _liste_bal))
            {
                pthread_t idThRcp2;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if ( pthread_create(&idThRcp2, &attr, ThRcp2, NULL)!=0 ) //PTHREAD_CREATE_DETACHED à rajouter !!
                {
                    printf("Erreur lors de la création de la tâche reception\n");
                }
            }



            else
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Cette identifiant n'a pas de boite aux lettres\n");
                pthread_mutex_lock(&_mutex_crcp);
                _ecri_crcp = 1;
                pthread_cond_signal(&_crcp);
                pthread_mutex_unlock(&_mutex_crcp);


            }
        }

        if (_requete.type == 5)
        {
            if (dans(_requete.idente, _liste_bal) == 0)
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Identifiant inexistant\n");
                pthread_mutex_lock(&_mutex_cabo);
                _ecri_cabo = 1;
                pthread_cond_signal(&_cabo);
                pthread_mutex_unlock(&_mutex_cabo);


            }
            else
            {
                pthread_t idThDesabo;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if ( pthread_create(&idThDesabo, &attr, ThDesabo, NULL)!=0 ) //PTHREAD_CREATE_DETACHED à rajouter !!
                {
                    printf("Erreur lors de la création de la tâche abonnement\n");
                }
            }
        }

        if (_requete.type == 6)
        {
            _lance = 0;
            (_requete.reponse) -> code_ret = 0;
            strcpy( (*_requete.reponse).msg, "La terminaison du thread gestionnaire à été effectué\n");
            pthread_mutex_lock(&_mutex_cfin);
            _ecri_cfin = 1;
            pthread_cond_signal(&_cfin);
            pthread_mutex_unlock(&_mutex_cfin);
            pthread_exit(NULL);
        }

    }
};



int initMsg(int nb_max_abo)
{
    if (nb_max_abo <= 0 || nb_max_abo > nb_limite_abo)
    {
        printf("Nombre d'abonné incorrect\n");
    }
    else
    {
        _nb_max_abo = nb_max_abo;
        if (_lance == 1)
        {
            printf("Service déjà lancé\n");
        }
        else
        {
            if (pthread_create(&idThGest, NULL, ThGest, NULL)!=0)
            {
                printf("Erreur lors de la création de la tâche gestionnaire\n");
                exit(1);
            }
            else
            {
                _lance = 1;
            }
        }
    }
    return 0;
};

int sendMsg(int identifiantexp, int identifiantdes, char message[message_len])
{
    if (_lance == 0)
    {
        printf("Le serveur n'est pas lancé\n");
        return -1;
    }
    else
    {
        struct reponse repo_msg;
        _requete.type = 2;
        _requete.idente = identifiantexp;
        _requete.identd = identifiantdes;
        strcpy(_requete.msg, message);
        _requete.reponse = &repo_msg;

        pthread_mutex_lock(&_mutex_cr);
        _ecri_requete = 1;
        pthread_cond_signal(&_crequete);
        pthread_mutex_unlock(&_mutex_cr);

        printf("Attend la reponse de msg...\n");

        pthread_mutex_lock(&_mutex_cmsg);
        _ecri_cmsg = 0;
        while (_ecri_cmsg == 0)
        {
            pthread_cond_wait(&_cmsg, &_mutex_cmsg);
        }
        pthread_mutex_unlock(&_mutex_cmsg);
        printf("code retour : %d, Message : %s \n", repo_msg.code_ret, repo_msg.msg);
    }
    return 0;
};


int recvMsg(int identifiant, char message[message_len], char flag[5])
{
    if (_lance == 0)
    {
        printf("Le serveur n'est pas lancé\n");
        return -1;
    }
    else
    {
        if (strcmp(flag,"RECEV") == 0)
        {
            struct reponse repo_rcp;
            _requete.type = 3;
            _requete.idente = identifiant;
            _requete.reponse = &repo_rcp;

            pthread_mutex_lock(&_mutex_cr);
            _ecri_requete = 1;
            pthread_cond_signal(&_crequete);
            pthread_mutex_unlock(&_mutex_cr);

            printf("Attend la reponse de reception du message...\n");

            pthread_mutex_lock(&_mutex_crcp);
            _ecri_crcp = 0;
            while (_ecri_crcp == 0)
            {
                pthread_cond_wait(&_crcp, &_mutex_crcp);
            }
            pthread_mutex_unlock(&_mutex_crcp);
            printf("Code retour : %d, Message : %s \n", repo_rcp.code_ret, repo_rcp.msg);
            strcpy(message, repo_rcp.msg);
        }
        if (strcmp(flag,"COUNT") == 0)
        {
            struct reponse repo_rcp;
            _requete.type = 4;
            _requete.idente = identifiant;
            _requete.reponse = &repo_rcp;

            pthread_mutex_lock(&_mutex_cr);
            _ecri_requete = 1;
            pthread_cond_signal(&_crequete);
            pthread_mutex_unlock(&_mutex_cr);

            printf("Attend la reponse de reception du nombre de messages...\n");

            pthread_mutex_lock(&_mutex_crcp);
            _ecri_crcp = 0;
            while (_ecri_crcp == 0)
            {
                pthread_cond_wait(&_crcp, &_mutex_crcp);
            }
            pthread_mutex_unlock(&_mutex_crcp);
            printf("Code retour : %d, Nombre de message : %s \n", repo_rcp.code_ret, repo_rcp.msg);
            strcpy(message, repo_rcp.msg);
        }
        if ((strcmp(flag,"COUNT") != 0) && (strcmp(flag,"RECEV") != 0))
        {
            printf("Mauvais flag de requete\n");
        }

    }
    return 0;
};


int aboMsg(int identifiant)
{
    if (_lance == 0)
    {
        printf("Le serveur n'est pas lancé\n");
        return -1;
    }
    else
    {
        if (_nb_abo_courant == _nb_max_abo)
        {
            printf("Nombre d'abonné maximum atteint\n");
            return -1;
        }
        else
        {
            struct reponse repo_abo;
            _requete.type = 1;
            _requete.idente = identifiant;
            _requete.reponse = &repo_abo;

            pthread_mutex_lock(&_mutex_cr);
            _ecri_requete = 1;
            pthread_cond_signal(&_crequete);
            pthread_mutex_unlock(&_mutex_cr);

            printf("Attend la reponse de abo...\n");

            pthread_mutex_lock(&_mutex_cabo);
            _ecri_cabo = 0;
            while (_ecri_cabo == 0)
            {
                pthread_cond_wait(&_cabo, &_mutex_cabo);
            }
            pthread_mutex_unlock(&_mutex_cabo);
            printf("code retour : %d, Message : %s \n", repo_abo.code_ret, repo_abo.msg);
        }
    }
    return 0;
};


int desaboMsg(int identifiant)
{
    if (_lance == 0)
    {
        printf("Le serveur n'est pas lancé\n");
        return -1;
    }
    else
    {
        if (_nb_abo_courant == 0)
        {
            printf("Aucun abonné au service\n");
            return -1;
        }
        else
        {
            struct reponse repo_desabo;
            _requete.type = 5;
            _requete.idente = identifiant;
            _requete.reponse = &repo_desabo;

            pthread_mutex_lock(&_mutex_cr);
            _ecri_requete = 1;
            pthread_cond_signal(&_crequete);
            pthread_mutex_unlock(&_mutex_cr);

            printf("Attend la reponse de desabo...\n");

            pthread_mutex_lock(&_mutex_cdesabo);
            _ecri_cdesabo = 0;
            while (_ecri_cdesabo == 0)
            {
                pthread_cond_wait(&_cdesabo, &_mutex_cdesabo);
            }
            pthread_mutex_unlock(&_mutex_cdesabo);
            printf("code retour : %d, Message : %s \n", repo_desabo.code_ret, repo_desabo.msg);
        }
    }
    return 0;
};


int finMsg(char flag[5]) //CLEAN FORCED
{
    if (_lance == 0)
    {
        printf("Service déjà terminé\n");
    }
    else
    {
        if (strcmp(flag,"CLEAN") == 0)
        {
            if (_nb_abo_courant > 0)
            {
                printf("Il y a encore des threads abonnés\n");
                return -1;
            }
            else
            {
                struct reponse repo_fin;
                _requete.type = 6;
                _requete.reponse = &repo_fin;

                pthread_mutex_lock(&_mutex_cr);
                _ecri_requete = 1;
                pthread_cond_signal(&_crequete);
                pthread_mutex_unlock(&_mutex_cr);

                printf("Attend la reponse de terminaison...\n");

                pthread_mutex_lock(&_mutex_cfin);
                _ecri_cfin = 0;
                while (_ecri_cfin == 0)
                {
                    pthread_cond_wait(&_cfin, &_mutex_cfin);
                }
                pthread_mutex_unlock(&_mutex_cfin);
                printf("code retour : %d, Message : %s \n", repo_fin.code_ret, repo_fin.msg);
            }
        }

        if (strcmp(flag,"FORCE") == 0)
        {
            struct reponse repo_fin;
            _requete.type = 6;
            _requete.reponse = &repo_fin;

            pthread_mutex_lock(&_mutex_cr);
            _ecri_requete = 1;
            pthread_cond_signal(&_crequete);
            pthread_mutex_unlock(&_mutex_cr);

            printf("Attend la reponse de terminaison...\n");

            pthread_mutex_lock(&_mutex_cfin);
            _ecri_cfin = 0;
            while (_ecri_cfin == 0)
            {
                pthread_cond_wait(&_cfin, &_mutex_cfin);
            }
            pthread_mutex_unlock(&_mutex_cfin);
            printf("code retour : %d, Message : %s \n", repo_fin.code_ret, repo_fin.msg);
        }
        if ((strcmp(flag,"CLEAN") != 0) && (strcmp(flag,"FORCE") != 0))
        {
            printf("Mauvais flag de requete\n");
        }
    }
    return 0;
};


int main()
{
    aboMsg(10);
    printf("Init...\n");
    initMsg(50);
    printf("abo 10\n");
    aboMsg(10);
    printf("abo 20\n");
    aboMsg(20);
    printf("desabo20et10\n");
    desaboMsg(20);
    desaboMsg(10);
    printf("finclean\n");
    finMsg("CLEAN");
    printf("finforce\n");
    finMsg("FORCE");
    pthread_join(idThGest, NULL);
    return 0;
};

