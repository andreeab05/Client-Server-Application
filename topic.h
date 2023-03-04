// Structura de date (lista dublu inlantuita) pentru retinerea
// subscriberilor abonati la un topic (se retine id-ul subscriberului
// si valoarea pentru store-and-forward SF)
typedef struct subscriber {
    char id_client[10];
    int SF;
    struct subscriber* next;
    struct subscriber* prev; 
}subscriber;

// Structura de date (lista dublu inlantuita) pentru retinerea listei
// cu topicuri. Se retine numele topicului si o lista cu subscriberii abonati
// la topicul respectiv
typedef struct topicList {
    char topic[50];
    subscriber* subscribers;
    struct topicList* next; // Pointer to next node in DLL
    struct topicList* prev; // Pointer to previous node in DLL
}topicList;

// Functie de adaugare a unui topic nou in lista de topicuri
void pushTopic(topicList** head, char* topicName)
{
    // Alocare memorie pentru nodul nou
    topicList* new_node = malloc(sizeof(topicList));
    strcpy(new_node->topic, topicName);
    new_node->subscribers = NULL;
 
    new_node->next = (*head);
    new_node->prev = NULL;
 
    if ((*head) != NULL)
        (*head)->prev = new_node;
 
    (*head) = new_node;
}

// Functie care cauta un topic in lista de topicuri si returneaza un pointer
// catre nodul respectiv din lista daca este gasit, altfel returneaza NULL
topicList* searchTopic(topicList** head, char* topicName){
    if (*head == NULL)
        return NULL;
    
    // Parcurgere lista de topicuri si cautarea topicului dupa nume
    topicList* tmp = *head;
    while(tmp) {
        if(strcmp(tmp->topic, topicName) == 0)
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

// Functie care cauta un subscriber in lista de subscriberi dupa id si
// il elimina din lista
void searchAndRemoveClient(subscriber** head, char* id) {
    if(*head == NULL)
        return;

    subscriber* tmp = *head;
    // Parcurgere lista de subscriberi si eliminarea nodului cu id-ul
    // identic cu cel primit ca parametru, daca exista
    while(tmp) {
        if(strcmp(tmp->id_client, id) == 0){
            if(tmp->next != NULL)
                tmp->next->prev = tmp->prev;
            if(tmp->prev != NULL)
                tmp->prev->next = tmp->next;
            return;
        }
        tmp = tmp->next; 
    }
}

// Functie care adauga in lista de subscriberi a unui topic un subscriber nou
void addSubscriber(topicList** node, clientList* clients, int socket, int SF){
    if(clients == NULL)
        return;
    
    clientList* tmp = clients;
    subscriber* newSubscriber = malloc(sizeof(subscriber));

    // Se parcurge lista de clienti clients tinuta de server si se cauta clientul
    // dupa socketul pe care a trimis mesajul si cand este gasit, este adaugat
    while(tmp) {
        if(tmp->socket == socket) {
            // Adaugare in lista de subscriberi atunci cand aceasta este vida 
            if((*node)->subscribers == NULL){
                strcpy(newSubscriber->id_client,tmp->id_client);
                newSubscriber->SF = SF;
                newSubscriber->next = (*node)->subscribers;
                newSubscriber->prev = NULL;
                (*node)->subscribers = newSubscriber;
                return;
            }
            // Adaugare in lista de subscriberi atunci cand lista nu este vida
            else{
                subscriber* aux = (*node)->subscribers;
                strcpy(newSubscriber->id_client, tmp->id_client);
                newSubscriber->SF = SF;
                while(aux->next) {
                    // Se verifica daca clientul este deja in lista de subscriberi
                    // caz in care nu se mai adauga
                    if(strcmp(aux->id_client, tmp->id_client) == 0){
                        fprintf(stderr, "%s\n", "Already subscribed!");
                        return;
                    }
                    aux = aux->next;
                }
                aux->next = newSubscriber;
                newSubscriber->next = NULL;
                newSubscriber->prev = aux;
                return;
            }
        }
        tmp = tmp->next;
    }
}

// Functie ce parcurge lista de subscriberi a unui topic si scoate din aceasta un subscriber,
// daca acesta se afla in lista
void deleteSubscriber(topicList** head, clientList* clients, char* topic, int socket)
{
    if (*head == NULL)
        return;

    if(clients == NULL)
        return;

    clientList* aux = clients;
    // Se cauta in lista de clienti tinuta de server subscriberul respectiv dupa socket
    while(aux){
        if(aux->socket == socket)
            break;
        aux = aux->next;
    }
 
    topicList *tmp = *head;
    // Se cauta topicul in lista de topicuri si daca este gasit, se cauta si se elimina 
    // subscriberul din lista de subscriberi cu ajutorul functiei searchAndRemoveClient
    while(tmp){
        if(strcmp(tmp->topic, topic) == 0) {
            searchAndRemoveClient(&tmp->subscribers, aux->id_client);
            return;
        }
        tmp = tmp->next;
    }
    return;
}

// Functie ce elibereaza memoria alocata pentru lista de subscriberi
// a unui topic 
void freeSubs(topicList *topic) {
    subscriber* tmp;
    while(topic->subscribers) {
        tmp = topic->subscribers->next;
        free(topic->subscribers);
        topic->subscribers = tmp;
    }
}



