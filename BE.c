#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define nb_limite_abo 100
#define nb_max_msg_bal 10

struct bal
{
    char ident;
    int pointeur1;
    int pointeur2;
    char msgs[nb_max_msg_bal];
};

struct reponse
{
    int code_ret;
    char msg;
};

struct requete
{
    int type; // 0=pas de requête, 1=abonnement, 2=envoi de message, 3=lecture des messages, 4=désabonnement
    char idente;
    char identd;
    struct reponse *reponse;
};


int _lance = 0;
struct requete _requete;
int _nb_abo_courant = 0;
struct bal _liste_bal[nb_limite_abo];

pthread_cond_t _crequete = PTHREAD_COND_INITIALIZER;
pthread_cond_t _cabo = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_cr = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cabo = PTHREAD_MUTEX_INITIALIZER;


int dans(char element, struct bal *liste)
{
    int i;
    for (i = 0; i < _nb_abo_courant; i++)
        if (element == (liste[i].ident))
    {
        return 1;
    }
return 0;
}

void *ThAbo(void *arg)
{
    struct requete requete = (struct requete)arg;
    _liste_bal[_nb_abo_courant].ident = requete.idente;
    _liste_bal[_nb_abo_courant].pointeur1 = 0;
    _liste_bal[_nb_abo_courant].pointeur2 = 0;
    _nb_abo_courant++;
    requete.reponse -> code_ret = 0;
    strcpy((requete.reponse)->msg, "Thread abonné avec succès.");
    pthread_mutex_lock(&_mutex_cabo);
    pthread_cond_signal(&_cabo, &_mutex_cabo);
    pthread_mutex_unlock(&_mutex_cabo);
    exit(1);
}

void *ThGest(void *arg)
{
    int nb_max_abo = (int)arg;
    _lance = 1;
    while(1)
    {
        pthread_mutex_lock(&_mutex_cr);
        pthread_cond_wait(&_crequete, &_mutex_cr);
        pthread_mutex_unlock(&_mutex_cr);
        if (_requete.type == 1)
        {
            if (dans(_requete.idente, liste_bal))
            {
                _requete.reponse -> code_ret = -1;
                _requete.reponse -> msg = "Identifiant déjà utilisé.";
                pthread_mutex_lock(&_mutex_cabo);
                pthread_cond_signal(&_cabo, &_mutex_cabo);
                pthread_mutex_unlock(&_mutex_cabo);
            }
            else
            {
                pthread_t idThAbo;
                if (pthread_create(&idThAbo, PTHREAD_CREATE_DETACHED, ThAbo, (void*)_requete)!=0)
                {
                    printf("Erreur lors de la création de la tâche gestionnaire");
                    exit(1);
                }
            }
        }

    }
}



int initMsg(int nb_max_abo)
{
    if (nb_max_abo <= 0 || nb_max_abo > nb_limite_abo)
    {
        printf("Nombre d'abonné incorrect");
    }
    else
    {
        if (_lance == 1)
        {
            printf("Service déjà lancé");
        }
        else
        {
            pthread_t idThGest;
            if (pthread_create(&idThGest, NULL, ThGest, (void*)nb_max_abo)!=0)
            {
                printf("Erreur lors de la création de la tâche gestionnaire");
                exit(1);
            }
        }
    }
    return 0;
}

int aboMsg(identifiant)
{
    int i = 0;
    if (lance == 0)
    {
        printf("Le serveur n'est pas lancé");
    }
    else
    {
        if (nb_abo_courant == nb_max_abo)
        {
            printf("Nombre d'abonné maximum atteint");
        }
        else
        {
            struct reponse repo_abo;
            _requete.type = 1;
            _requete.idente = identifiant;
            _requete.(*reponse) = &repo_abo;
            pthread_mutex_lock(&_mutex_cr);
            pthread_cond_signal(&_crequete, &_mutex_cr);
            pthread_mutex_unlock(&_mutex_cr);
            pthread_mutex_lock(&_mutex_cabo);
            pthread_cond_wait(&_cabo, &_mutex_cr);
            pthread_mutex_unlock(&_mutex_cabo);

        }
    }
}

initMsg(10);
