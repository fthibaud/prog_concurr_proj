//#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#define nb_limite_abo 100
#define nb_max_msg_bal 10
#define message_len 80


struct bal
{
    char ident;
	int used;
    int pointeur1;
    int pointeur2;
    unsigned int nb_msg;
    sem_t nb_Libr;
    sem_t nb_Full;
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

char _none;
int _lance = 0;
Requete _requete;
//int _nb_abo_courant = 0;
sem_t _nb_max_abo;
struct bal _liste_bal[nb_limite_abo];



pthread_cond_t _cabo = PTHREAD_COND_INITIALIZER;
pthread_cond_t _cmsg = PTHREAD_COND_INITIALIZER;
pthread_cond_t _crcp = PTHREAD_COND_INITIALIZER;
pthread_cond_t _cdesabo = PTHREAD_COND_INITIALIZER;
pthread_cond_t _cfin = PTHREAD_COND_INITIALIZER;
pthread_cond_t _crcpb = PTHREAD_COND_INITIALIZER;

pthread_mutex_t _mutex_cabo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cmsg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_crcp = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_crcpb = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cdesabo= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_cfin= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_bal= PTHREAD_MUTEX_INITIALIZER;

//initialisation des mutexs de ressource

pthread_mutex_t _mutex_requete = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_bal_list = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mutex_ecran = PTHREAD_MUTEX_INITIALIZER;

//fin

//initialisation des variables conditionnelles et de leurs mutexs associées

pthread_cond_t _c_requete_depot = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_c_requete_depot = PTHREAD_MUTEX_INITIALIZER;
int _c_r_d = 0;

pthread_cond_t _c_requete_libre = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_c_requete_libre = PTHREAD_MUTEX_INITIALIZER;
int _c_r_l = 0;

pthread_cond_t _c_reponse_abo = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_c_reponse_abo = PTHREAD_MUTEX_INITIALIZER;
int _c_r_a = 0;

pthread_cond_t _c_reponse_send = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_c_reponse_send = PTHREAD_MUTEX_INITIALIZER;
int _c_r_s = 0;

pthread_cond_t _c_reponse_recv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_c_reponse_recv = PTHREAD_MUTEX_INITIALIZER;
int _c_r_r = 0;

pthread_cond_t _c_reponse_count = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_c_reponse_count = PTHREAD_MUTEX_INITIALIZER;
int _c_r_c = 0;

//fin

pthread_t idThGest;


void signal(pthread_cond_t *cond, pthread_mutex_t *mutex, int *condition)
{
    pthread_mutex_lock(mutex);
    (*condition) = 1;
    pthread_mutex_unlock(mutex);
    pthread_cond_signal(cond);
};


void wait(pthread_cond_t *cond, pthread_mutex_t *mutex, int *condition)
{
    pthread_mutex_lock(mutex);
    while (!*condition)
    	pthread_cond_wait(cond, mutex);
    (*condition) = 0;
    pthread_mutex_unlock(mutex);
};


int dans(char element, struct bal *liste)
{
    int i = 0;
    while (liste[i].used)
	{
        if (element == (liste[i].ident))
        {
            return 1;
        }
		i++;
	}    
	return 0;
};

int indice(char idente, struct bal *liste)
{
    int i = 0;
    while (liste[i].used)
	{
        if (idente == (liste[i].ident))
        {
            return i;
        }
		i++;
	}
    return -1;
}

int indice_libre(struct bal *liste)
{
    int i = 0;
    while (liste[i].used)
	{
		i++;
	}
    return i;
}

struct bal* addbal (char element, struct bal *liste)
{
    struct bal* retour;
    int i = 0;
    while (liste[i].used)
	{
        if (element == (liste[i].ident))
        {
            retour = &(liste[i]);
        }
		i++;
	}
    return retour;
};


void echange(int reqtype, struct reponse * reponse, char idente, char identd, char message[message_len], pthread_cond_t * c_reponse, pthread_mutex_t * mutex_c_reponse, int * c_r_rep)
{
		pthread_mutex_lock(&_mutex_requete); // mutex pour vérouiller l'accès concurrent à requête

		_requete.type = reqtype; // dépot de la requête
    	_requete.idente = idente;
    	_requete.identd = identd;
    	_requete.reponse = reponse;
    	strcpy(_requete.msg, message);

		signal(&_c_requete_depot, &_mutex_c_requete_depot, &_c_r_d); // annonce au gestionnaire que la requête est déposée
		wait(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l); // attend que la requête soit recopiée localement par le thread de traitement pour libérer la requête
		
		pthread_mutex_unlock(&_mutex_requete); // libère le mutex

		printf("Le thread client attend la réponse à la requête...\n");
		wait(c_reponse, mutex_c_reponse, c_r_rep); // attend que le thread de traitement écrive dans la réponse
};


void *ThAbo(void *arg)
{
	Requete requete = *(&_requete);
	signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);

	pthread_mutex_lock(&_mutex_bal_list); // mutex pour l'accès en écriture à la liste des boîtes aux lettres
	int indice = indice_libre(_liste_bal);
    _liste_bal[indice].ident = requete.idente;
   	_liste_bal[indice].pointeur1 = 0;
   	_liste_bal[indice].pointeur2 = 0;
	_liste_bal[indice].used = 1;
   	sem_init(& _liste_bal[indice].nb_Libr, 0, nb_max_msg_bal);
   	sem_init(& _liste_bal[indice].nb_Full, 0, 0);
	pthread_mutex_unlock(&_mutex_bal_list); // fin mutex
    
	requete.reponse -> code_ret = 0;
    strcpy((*requete.reponse).msg, "Thread abonné avec succès\n");
    signal(&_c_reponse_abo, &_mutex_c_reponse_abo, &_c_r_a);

	pthread_exit(NULL);
};


void *ThMsg(void *arg)
{
    Requete requete = *(&_requete);
    signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);

    struct bal* bal;
	bal = addbal(requete.identd, _liste_bal); // on récupère l'adresse de la boite aux lettre qui correspond à l'identifiant de l'emetteur de la requete

    sem_wait(&((*bal).nb_Libr));

	pthread_mutex_lock(&_mutex_bal_list); // mutex pour l'accès en écriture à la liste des boîtes aux lettres
    sem_post(&((*bal).nb_Full));
    (*bal).pointeur2 = ((*bal).pointeur2 + 1) % 10;
    strcpy( (*bal).msgs[(*bal).pointeur1] , requete.msg );
    (*bal).nb_msg ++;
	pthread_mutex_unlock(&_mutex_bal_list); // fin mutex

    requete.reponse -> code_ret = 0;
    strcpy( (*requete.reponse).msg, "Message correctement envoyé");
    signal(&_c_reponse_send, &_mutex_c_reponse_send, &_c_r_s);

    pthread_exit(NULL);
};


void *ThRcp(void *arg)
{
    Requete requete = *(&_requete);
    signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);

    struct bal* bal;
	bal = addbal(requete.idente, _liste_bal); // on récupère l'adresse de la boite aux lettre qui correspond à l'identifiant de l'emetteur de la requete
    
	sem_wait((&(*bal).nb_Full));
	
	pthread_mutex_lock(&_mutex_bal_list); // mutex pour l'accès à la liste des boîtes aux lettres
    sem_post(&((*bal).nb_Libr));
    strcpy( (*requete.reponse).msg, (*bal).msgs[(*bal).pointeur1]);
    (*bal).pointeur1 = ((*bal).pointeur1 + 1) % nb_max_msg_bal;
    (*bal).nb_msg --;
	pthread_mutex_unlock(&_mutex_bal_list); // fin mutex

    requete.reponse -> code_ret = 0;
	signal(&_c_reponse_recv, &_mutex_c_reponse_recv, &_c_r_r);

    pthread_exit(NULL);
};


void *ThRcp2(void *arg)
{
    Requete requete = *(&_requete);
    signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);

	pthread_mutex_lock(&_mutex_bal_list); // mutex pour l'accès à la liste des boîtes aux lettres
    struct bal* bal;
    bal = addbal(requete.idente, _liste_bal); // on récupère l'adresse de la boite aux lettre qui correspond à l'identifiant de l'emetteur de la requete
    sprintf((*requete.reponse).msg, "%d", (*bal).nb_msg); 
	pthread_mutex_unlock(&_mutex_bal_list); // fin mutex

    requete.reponse -> code_ret = 0;
	signal(&_c_reponse_count, &_mutex_c_reponse_count, &_c_r_c);

    pthread_exit(NULL);
};

/*
void *ThDesabo(void *arg)
{
    Requete requete = *(&_requete);
    signal(&_mutex_cr, &_crequete, &_ecri_requete, 0);
    int i;
    for (i = indice(requete.idente, _liste_bal); i < _nb_abo_courant; i++)
    {
        _liste_bal[i] = _liste_bal[i+1];
    }
    _nb_abo_courant --;
    requete.reponse -> code_ret = 0;
    strcpy( (*requete.reponse).msg, "Thread désabonné avec succès\n");
    signal(&_mutex_cdesabo, &_cdesabo, &_ecri_cdesabo, 1);
    pthread_exit(NULL);
};*/


void *ThGest()
{
    printf("Messagerie active\n");
    while(1)
    {
        wait(&_c_requete_depot, &_mutex_c_requete_depot, &_c_r_d);
	printf("\n\nLe thread gestionnaire a bien reçu la requête.\n");
        if (_requete.type == 1)
        {
            if (dans(_requete.idente, _liste_bal))
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Identifiant déjà utilisé\n");
                signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
				signal(&_c_reponse_abo, &_mutex_c_reponse_abo, &_c_r_a);
            }
            else
            {
                pthread_t idThAbo;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if ( pthread_create(&idThAbo, &attr, ThAbo, NULL)!=0 )
                {
                    (_requete.reponse) -> code_ret = -1;
                	strcpy( (*_requete.reponse).msg, "Erreur lors de la création de la tâche abonnement\n");
                	signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
					signal(&_c_reponse_abo, &_mutex_c_reponse_abo, &_c_r_a);
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
                if ( pthread_create(&idThMsg, &attr, ThMsg, NULL)!=0 )
                {
					(_requete.reponse) -> code_ret = -1;
                	strcpy( (*_requete.reponse).msg, "Erreur lors de la création de la tâche envoie message\n");
                    signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
					signal(&_c_reponse_send, &_mutex_c_reponse_send, &_c_r_s);
                }
            }
            else
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "L'un des deux threads n'est pas abonnné\n");
                signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
				signal(&_c_reponse_send, &_mutex_c_reponse_send, &_c_r_s);
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
                if ( pthread_create(&idThRcp, &attr, ThRcp, NULL)!=0 )
                {
					(_requete.reponse) -> code_ret = -1;
                	strcpy( (*_requete.reponse).msg, "Erreur lors de la création de la tâche reception\n");
                    signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
					signal(&_c_reponse_recv, &_mutex_c_reponse_recv, &_c_r_r);
                }
            }
            else
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Cette identifiant n'a pas de boite aux lettres\n");
                signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
				signal(&_c_reponse_recv, &_mutex_c_reponse_recv, &_c_r_r);
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
                if ( pthread_create(&idThRcp2, &attr, ThRcp2, NULL)!=0 )
                {
                    (_requete.reponse) -> code_ret = -1;
                	strcpy( (*_requete.reponse).msg, "Erreur lors de la création de la tâche reception\n");
                	signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
					signal(&_c_reponse_count, &_mutex_c_reponse_count, &_c_r_c);
                }
            }
            else
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Cette identifiant n'a pas de boite aux lettres\n");
                signal(&_c_requete_libre, &_mutex_c_requete_libre, &_c_r_l);
				signal(&_c_reponse_count, &_mutex_c_reponse_count, &_c_r_c);
            }
        }
/*
        if (_requete.type == 5)
        {
            if (dans(_requete.idente, _liste_bal) == 0)
            {
                (_requete.reponse) -> code_ret = -1;
                strcpy( (*_requete.reponse).msg, "Identifiant inexistant\n");
                signal(&_mutex_cabo, &_cabo, &_ecri_cabo, 1);
                signal(&_mutex_cr, &_crequete, &_ecri_requete, 0);
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
                    signal(&_mutex_cr, &_crequete, &_ecri_requete, 0);
                }
            }
        }

        if (_requete.type == 6)
        {
            _lance = 0;
            (_requete.reponse) -> code_ret = 0;
            strcpy( (*_requete.reponse).msg, "La terminaison du thread gestionnaire à été effectué\n");
            signal(&_mutex_cfin, &_cfin, &_ecri_cfin, 1);
            signal(&_mutex_cr, &_crequete, &_ecri_requete, 0);
            pthread_exit(NULL);
        }*/
        _requete.type = 0;
    }
};


int initMsg(unsigned int nb_max_abo)
{
	int retour = -1;
    if (nb_max_abo <= 0 || nb_max_abo > nb_limite_abo)
    {
        printf("Nombre d'abonné incorrect\n");
    }
    else
    {
        if (_lance == 1)
        {
            printf("Service déjà lancé\n");
        }
        else
        {
			sem_init(&_nb_max_abo, 0, nb_max_abo);
            if (pthread_create(&idThGest, NULL, ThGest, NULL)!=0)
            {
                printf("Erreur lors de la création de la tâche gestionnaire\n");
            }
            else
            {
                _lance = 1;
		retour = 0;
            }
        }
    }
    return retour;
};


int aboMsg(int identifiant)
{
    int retour = -1;
    if (_lance == 0)
    {
        printf("Le serveur n'est pas lancé\n");
    }
    else
    {
		if (sem_trywait(&_nb_max_abo)==-1)
        {
            printf("Nombre d'abonné maximum atteint\n");
        }
        else
        {
		struct reponse repo_abo; // création de l'espace pour la réponse

		echange(1, &repo_abo, identifiant, _none, "", &_c_reponse_abo, &_mutex_c_reponse_abo, &_c_r_a);

        printf("Message : %s \n", repo_abo.msg);
		retour = repo_abo.code_ret; // récupère le code retour
        }
    }
    return retour;
};


int sendMsg(int identifiantexp, int identifiantdes, char message[message_len])
{
    int retour = -1;
    if (_lance == 0)
    {
        printf("Le serveur n'est pas lancé\n");
    }
    else
    {
        struct reponse repo_msg; // création de l'espace pour la réponse			
		
		echange(2, &repo_msg, identifiantexp, identifiantdes, message, &_c_reponse_send, &_mutex_c_reponse_send, &_c_r_s);

        printf("Code retour : %d, Message : %s \n", repo_msg.code_ret, repo_msg.msg);
		retour = repo_msg.code_ret; // récupère le code retour
    }
    return retour;
};


int recvMsg(int identifiant, char message[message_len], char flag[5])
{
	int retour = -1;
    if (_lance == 0)
    {
        printf("Le serveur n'est pas lancé\n");
    }
    else
    {
        if (strcmp(flag,"RECEV") == 0)
        {
            struct reponse repo_rcp;

			echange(3, &repo_rcp, identifiant, _none, "", &_c_reponse_recv, &_mutex_c_reponse_recv, &_c_r_r);

            printf("Code retour : %d, Message : %s \n", repo_rcp.code_ret, repo_rcp.msg);
			retour = repo_rcp.code_ret; // récupère le code retour
            strcpy(message, repo_rcp.msg);
        }
        if (strcmp(flag,"COUNT") == 0)
        {
            struct reponse repo_rcp;

			echange(4, &repo_rcp, identifiant, _none, "", &_c_reponse_count, &_mutex_c_reponse_count, &_c_r_c);

            printf("Code retour : %d, Message : %s \n", repo_rcp.code_ret, repo_rcp.msg);
			retour = repo_rcp.code_ret; // récupère le code retour
            strcpy(message, repo_rcp.msg);
        }
        if ((strcmp(flag,"COUNT") != 0) && (strcmp(flag,"RECEV") != 0))
        {
            printf("Mauvais flag de requete\n");
        }
    }
    return retour;
};


/*
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
            wait(&_mutex_cr, &_crequete, &_ecri_requete, 1, -1);
            struct reponse repo_desabo;
            sendreq(5, &_requete, &repo_desabo, identifiant, _none, "");
            signal(&_mutex_cr, &_crequete, &_ecri_requete, 1);
            printf("Attend la reponse de desabo...\n");
            wait(&_mutex_cdesabo, &_cdesabo, &_ecri_cdesabo, 0, 0);
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
        void finrep()
        {
            wait(&_mutex_cr, &_crequete, &_ecri_requete, 1, -1);
            struct reponse repo_fin;
            sendreq(6, &_requete, &repo_fin, _none, _none, "");
            signal(&_mutex_cr, &_crequete, &_ecri_requete, 1);
            printf("Attend la reponse de terminaison...\n");
            wait(&_mutex_cfin, &_cfin, &_ecri_cfin, 0, 0);
            printf("code retour : %d, Message : %s \n", repo_fin.code_ret, repo_fin.msg);
        }
        if (strcmp(flag,"CLEAN") == 0)
        {
            if (_nb_abo_courant > 0)
            {
                printf("Il y a encore des threads abonnés\n");
                return -1;
            }
            else
            {
                finrep();
            }
        }

        if (strcmp(flag,"FORCE") == 0)
        {
            finrep();
        }
        if ((strcmp(flag,"CLEAN") != 0) && (strcmp(flag,"FORCE") != 0))
        {
            printf("Mauvais flag de requete\n");
        }
    }
    return 0;
};*/

