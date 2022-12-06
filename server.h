#ifndef LIST
#define LIST

typedef struct NoeudClient {
    int data;
    struct NoeudClient* prev;
    struct NoeudClient* link;
    char ip[16];
    char name[31];
} ListeClient;

ListeClient *nouveauNoeud(int sockfd, char* ip) {
    ListeClient *np = (ListeClient *)malloc( sizeof(ListeClient) );
    np->data = sockfd;
    np->prev = NULL;
    np->link = NULL;
    strncpy(np->ip, ip, 16);
    strncpy(np->name, "NULL", 5);
    return np;
}

#endif // LIST