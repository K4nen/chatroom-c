#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "proto.h"
typedef struct NoeudClient {
    int data;
    struct NoeudClient* prev;
    struct NoeudClient* link;
    char ip[16];
    char name[31];
} ListeClient;



// Variables globales
int server_sockfd = 0, client_sockfd = 0;
ListeClient
 *root, *now;

ListeClient *nouveauNoeud(int sockfd, char* ip) {
    ListeClient *np = (ListeClient *)malloc( sizeof(ListeClient) );
    np->data = sockfd;
    np->prev = NULL;
    np->link = NULL;
    strncpy(np->ip, ip, 16);
    strncpy(np->name, "NULL", 5);
    return np;
}

void catch_ctrl_c_and_exit(int sig) {
    ListeClient *tmp;
    while (root != NULL) {
        printf("\nFermeture des sockets: %d\n", root->data);
        close(root->data); // fermeture de tous les sockets
        tmp = root;
        root = root->link;
        free(tmp);
    }
    printf("Aurevoir\n");
    exit(EXIT_SUCCESS);
}

void envoi_a_tous_clients(ListeClient *np, char tmp_buffer[]) {
    ListeClient *tmp = root->link;
    while (tmp != NULL) {
        if (np->data != tmp->data) { // tous les clients sauf lui meme
            printf("Envoi au sockets %d: \"%s\" \n", tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

void client_handler(void *p_client) {
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_SEND] = {};
    ListeClient *np = (ListeClient *)p_client;

    // Nommage
    if (recv(np->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("%s didn't input name.\n", np->ip);
        leave_flag = 1;
    } else {
        strncpy(np->name, nickname, LENGTH_NAME);
        printf("%s(%s)(%d) a rejoint le chat.\n", np->name, np->ip, np->data);
        sprintf(send_buffer, "%s(%s)  a rejoint le chat.", np->name, np->ip);
        envoi_a_tous_clients(np, send_buffer);
    }

    // Conversation
    while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(np->data, recv_buffer, LENGTH_MSG, 0);
        if (receive > 0) {
            if (strlen(recv_buffer) == 0) {
                continue;
            }
            sprintf(send_buffer, "%s：%s", np->name, recv_buffer);
        } else if (receive == 0 || strcmp(recv_buffer, "exit") == 0) {
            printf("%s(%s)(%d) a quitté le chat.\n", np->name, np->ip, np->data);
            sprintf(send_buffer, "%s(%s) a quitté le chat.", np->name, np->ip);
            leave_flag = 1;
        } else {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
        envoi_a_tous_clients(np, send_buffer);
    }

    // Enlever noeud
    close(np->data);
    if (np == now) { // Enlever noeud du haut
        now = np->prev;
        now->link = NULL;
    } else { // enlever noeud milieu
        np->prev->link = np->link;
        np->link->prev = np->prev;
    }
    free(np);
}

int main()
{
    signal(SIGINT, catch_ctrl_c_and_exit);

    // Creer socket
    server_sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sockfd == -1) {
        printf("Erreur de création du socket.");
        exit(EXIT_FAILURE);
    }

    // Information du socket
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(8888);

    // Bind et Listen
    bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen);
    listen(server_sockfd, 5);

    // Afficher IP Serveur
    getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Serveur lancé sur: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    // Liste chainée pour clients
    root = nouveauNoeud(server_sockfd, inet_ntoa(server_info.sin_addr));
    now = root;

    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);

        // Afficher adresse IP Client
        getpeername(client_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
        printf("Le client %s:%d est arrivé.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

        // incrémenter liste chainée
        ListeClient *c = nouveauNoeud(client_sockfd, inet_ntoa(client_info.sin_addr));
        c->prev = now;
        now->link = c;
        now = c;

        pthread_t id;
        if (pthread_create(&id, NULL, (void *)client_handler, (void *)c) != 0) {
            perror("Erreur de création du thread\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}