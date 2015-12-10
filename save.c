int aboMsg(identifiant)
{
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
            if (in(identifiant, abos, nb_abo))
                {
                    printf("Identifiant déjà utilisé");
                }
            else
             {
              return 0;
             }
        }
    }
}

int in(char element, char *liste, int taille_liste)
{
    int i;
    for (i = 0; i < taille_liste; i++)
        if (element == liste[i])
        {
            return 1;
        }
    return 0;
}
