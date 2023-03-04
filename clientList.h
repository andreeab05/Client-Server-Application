// Structura de date ce reprezinta un nod din coada de mesaje
// Se retine in nod mesajul, lungimea sa (nr de bytes)
typedef struct msgQueue{
    message m;
    int mess_len;
    struct msgQueue *next;
}msgQueue;

// Structura pentru modelarea unei cozi
typedef struct queue{
    msgQueue *head;
    msgQueue *tail;
}queue;

// Structura de date (lista dublu inlantuita) ce reprezinta lista 
// de clienti tinuta de server. Se retine id-ul clientului, socketul
// si se retine valoarea 1 in campul connection_status daca clientul
// este conectat, respectiv 0 daca este deconectat
typedef struct clientList {
    char id_client[10];
    int socket;
    int connection_status;
    queue* unsent_msg;
    struct clientList* next;
    struct clientList* prev;
}clientList;

// Functie pentru adaugarea unui mesaj in coada
void enQ(queue** unsent_msgs, message *m, int mess_len) {
    msgQueue *newMsg = malloc(sizeof(msgQueue));
    memcpy(&newMsg->m, m, sizeof(message));
    newMsg->next = NULL;
    newMsg->mess_len = mess_len;

    if((*unsent_msgs)->head == NULL && (*unsent_msgs)->tail == NULL){
        (*unsent_msgs)->head = newMsg;
        (*unsent_msgs)->tail = newMsg;
        return;
    }

    (*unsent_msgs)->tail->next = newMsg;
    (*unsent_msgs)->tail = newMsg;
}

// Functie pentru eliminarea unui mesaj din coada. Mesajul este
// returnat
msgQueue* deQ(queue ** unsent_msgs) {
    msgQueue* tmp = (*unsent_msgs)->head;
    (*unsent_msgs)->head = (*unsent_msgs)->head->next;

    if((*unsent_msgs)->head == NULL)
        (*unsent_msgs)->tail = NULL;

    return tmp;
}

// Functie ce elibereaza memoria alocata dinamic pentru coada
void freeQueue(queue *msgQ) {
    while(msgQ->head){
        msgQueue* tmp = msgQ->head;
        free(tmp);
        msgQ->head = msgQ->head->next;
    }
    if(msgQ->head == NULL){
        msgQ->tail = NULL;
    }
}

// Functie ce adauga in lista de clienti un client nou
void push(clientList** head, int socket, char* id_client)
{
    clientList* new_node = malloc(sizeof(clientList));

    strcpy(new_node->id_client, id_client);
    new_node->socket = socket;
    new_node->connection_status = 1;
    new_node->unsent_msg = malloc(sizeof(queue));
    new_node->unsent_msg->head = NULL;
    new_node->unsent_msg->tail = NULL;
    new_node->next = (*head);
    new_node->prev = NULL;
 
    if ((*head) != NULL)
        (*head)->prev = new_node;

    (*head) = new_node;
}

// Functie ce cauta un client in lista de clienti a serverului si returneaza:
// 0 -> daca lista e vida sau daca clientul este nou
// 1 -> daca clientul nou are acelasi ID cu un client deja conectat
// 2 -> daca clientul se reconecteaza
int search_client(clientList *clients, char* id, int socket){
    if(clients == NULL){
        return 0;
    }
    clientList *tmp = clients;
    while(tmp){
        // Se verifica daca clientul curent este conectat sau nu
        if(tmp->connection_status == 1){
            // Daca este conectat se compara ID urile
            if(strcmp(tmp->id_client, id) == 0){
                return 1;
            }
        }
        // Daca nu este conectat si id-urile sunt identice => reconectare
        else{
            if(strcmp(tmp->id_client, id) == 0){
                // Actualizare camp de connection_status si socket
                tmp->connection_status = 1;
                tmp->socket = socket;
                return 2;
            }
        }
        tmp = tmp->next;
    }
    return 0;
}

// Functie ce primeste ca parametru un client si lista de clienti tinuta de server.
// Clientul este cautat in lista si daca este gasit si este conectat, un pointer 
//catre client este returnat de functie, altfel functia intoarce NULL
clientList* checkConnectivity(clientList *clients, char* id) {
    clientList* tmp = clients;
    while(tmp){
        if(strcmp(tmp->id_client, id) == 0){
            if(tmp->connection_status == 1)
            {
                return tmp;
            }
        }
        tmp = tmp->next;
    }
    return NULL;
}

// Functie ce cauta in lista de clienti un client cu ID ul primit ca parametru
// si returneaza fie un pointer catre clientul respectiv, fie NULL
clientList* findClient(clientList* clients, char* id) {
    clientList* tmp = clients;
    while(tmp) {
        if(strcmp(tmp->id_client, id) == 0)
            return tmp;

        tmp = tmp->next;
    }
    return NULL;
}