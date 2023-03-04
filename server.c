#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <math.h>
#include "helpers.h"
#include "message.h"
#include "clientList.h"
#include "topic.h"
#define UDPMAX 1551

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, sock_udp;
	char buffer[BUFLEN];
    char id_client[BUFLEN];
	// Lista de clienti a serverului
    clientList *clients = NULL;
	// Lista de topicuri a serverului
	topicList *topics = NULL;
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	// Se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// Deschidere socketi pentru clientii TCP si UDP
	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "socket");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// Dezactivare algoritm Nagle
    int flag = 1;
    int res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(res < 0, "bad Nagle");

	// Setare port server
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	// Setare informatii despre server
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// Bind pe port pe care o sa asculte serverul 
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	ret = listen(sockfd, 100);
	DIE(ret < 0, "listen");

	// Bind pe socketul pe care asteapta mesaje de la clientii UDP
	ret = bind(sock_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	// Se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	// Se adauga noul file descriptor (socketul pe care serverul primeste mesaje de la clientii UDP) in multimea read_fds
	FD_SET(sock_udp, &read_fds);
	// Se adauga noul file descriptor (socketul pe care serverul primeste mesaje de la tastatura) in multimea read_fds
    FD_SET(STDIN_FILENO, &read_fds);

	if(sockfd > sock_udp)
		fdmax = sockfd;
	else
		fdmax = sock_udp;

	while (1) {
		tmp_fds = read_fds; 
		
		// Apelul functiei select care selecteaza din multimea tmp_fds doar
		// file descriptorii pe care s-au primit date
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			// Se verifica daca socketul i se afla in multimea tmp_fds, deci daca s-au
			// primit date pe el
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// Serverul primeste o cerere de conexiune pe socketul inactiv 
					// (cel cu listen), pe care serverul o accepta, apoi verifica datele
					// clientului nou
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

                    // Primire ID client pentru verificare
                    recv(newsockfd, id_client, sizeof(id_client) - 1, 0);

					// Se apeleaza functia search_client care cauta in lista de clienti
					// a serverului clientul nou cu id_client
					int flagClient = search_client(clients, id_client, newsockfd);
					// Clientul nou incerce sa se conecteze cu id-ul unui client deja conectat
					// deci se inchide conexiunea cu clientul nou
                    if(flagClient == 1){
                        printf("Client %s already connected.\n", id_client);
                        close(newsockfd);
						FD_CLR(newsockfd, &read_fds);
                        continue;
                    }
					// Un client deconectat de la server se reconecteaza
					else if(flagClient == 2) {
						printf("New client %s connected from %s:%d.\n",
							id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
						clientList* reconnected_cli = findClient(clients, id_client);
						// Se parcurge coada de mesaje a clientului reconectat si se
						// transmit mesajele primite de server pentru topicurile la care
						// acesta este abonament in timpul in care clientul nu era conectat
						while(reconnected_cli->unsent_msg->head){
							msgQueue *unsent = deQ(&reconnected_cli->unsent_msg);
							int len_msg = unsent->mess_len;
							// Trimit numarul de bytes ocupat de mesaj
							n = send(newsockfd, &len_msg, sizeof(len_msg), 0);
							DIE(n < 0, "bad send reconnect");
							n = send(newsockfd, &unsent->m, len_msg, 0);
							// Daca nu au frost trimisi toti octetii ocupati de mesaj,
							// se trimit restul pana la trimiterea mesajului complet
							while(len_msg - n > 0){
								n += send(newsockfd, &unsent->m + n, len_msg - n, 0);
							}
							DIE(n < 0, "bad send reconnect");
							free(unsent);
						}

						// Se adauga noul socket intors de apelul functiei accept() in multimea read_fds
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
						continue;
					}

					// Client nou
					// Se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					// Se adauga noul client in lista clientilor serverului
                    push(&clients, newsockfd, id_client);
					printf("New client %s connected from %s:%d.\n",
							clients->id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

				} else if (i == STDIN_FILENO) {
                    // Se scrie de la tastatura comanda pentru inchiderea serverului
                    memset(buffer, 0, sizeof(buffer));
                    n = read(0, buffer, sizeof(buffer) - 1);
		            DIE(n < 0, "failed read");

					// Daca se primeste comanda exit, se inchid conexiunile pentru toti
					// clientii serverului
                    if(strstr(buffer, "exit") != NULL){
                        clientList *tmp = clients;
						// Inchidere clienti si eliberare memorie pentru lista de clienti
                        while(tmp) {
							close(tmp->socket);
							FD_CLR(tmp->socket, &read_fds);
							// free la coada de mesaje
							freeQueue(tmp->unsent_msg);
							// free la nod in sine
							free(tmp);
                            tmp = tmp->next;
                        }

						// Eliberare memorie pentru lista de topicuri
						topicList *tmpTopic;
						while(topics) {
							tmpTopic = topics;
							// Eliberare memorie lista de subscriberi
							freeSubs(tmpTopic);
							free(tmpTopic);
							topics = topics->next;
						}
                        close(sockfd);
                        return 0;
                    }
                    else printf("Invalid command! Try again :( \n");

				// Serverul primeste date pe socketul pentru UDP
                } else if(i == sock_udp){
					void* buffer_udp = malloc(UDPMAX);
					n = recvfrom(i, buffer_udp, UDPMAX, 0, (struct sockaddr*) &from, &fromlen);
					DIE(n < 0, "bad recvfrom");

					message m;

					// In mesajul ce va fi trimis catre clienti, vor exista 2 campuri pentru
					// adresa IP a clientului UDP ce a trimis mesajul si pentru portul respectiv
					memcpy(&m.ip_udp, &from.sin_addr, sizeof(from.sin_addr));
					memcpy(&m.port_udp, &from.sin_port, sizeof(from.sin_port));

					// Parsarea mesajului primit de la clientul UDP
					// Extragerea topicului (primii 50 bytes) si a tipului (urmatorul byte)
					char topic[50];
					memcpy(topic, buffer_udp, 50);
					int size_payload = 0;
					if(strchr(topic, '\0') != NULL){
						strcpy(m.topic, topic);
						memcpy(&m.type, buffer_udp + 50, 1);
					}
					else{
						topic[50] = '\0';
						strcpy(m.topic, topic);
						memcpy(&m.type, buffer_udp + 51, 1);
					}

					// Parsarea payload-ului (urmatorii maxim 1500 bytes)in functie de tip
					// type -> INT
					if(m.type == 0){
						// Extragerea byte-ului de semn
						uint8_t sign_byte;
						memcpy(&sign_byte, buffer_udp + 51, 1);
						uint32_t payload_int;
						memcpy(&payload_int, buffer_udp + 52, 4);
						// Transformarea uint32_t din network byte order in host byte order
						payload_int = ntohl(payload_int);
						if(sign_byte == 1){
							payload_int = -payload_int;
						}
						memcpy(m.payload, &payload_int, 4);
						size_payload = sizeof(payload_int);
					}
					//type -> SHORT_REAL
					else if(m.type == 1) {
						uint16_t payload_sh_rl;
						memcpy(&payload_sh_rl, buffer_udp + 51, 2);
						double payload = ntohs(payload_sh_rl) * 1.0 / 100;
						memcpy(m.payload, &payload, sizeof(payload));
						size_payload = sizeof(payload);
					}
					// type -> FLOAT
					else if(m.type == 2) {
						// Extragere byte de semn
						uint8_t sign_byte;
						memcpy(&sign_byte, buffer_udp + 51, 1);

						// Extragerea modulului numarului obtinut prin alipirea partii
						// intregi de partea zecimala a numarului
						uint32_t module;
						memcpy(&module, buffer_udp + 52, 4);
						// Transformare modul din network byte order in host byte order
						module = (ntohl(module));

						// Extragerea modulului puterii negative a lui 10 cu care trebuie
						// inmultit modulul pentru a obtine numarul original (in modul)
						uint8_t power;
						memcpy(&power, buffer_udp + 56 , 1);
						double div = pow(10.0, (1.0 * power));
						double payload = module / div;

						if(sign_byte == 1)
							payload = -payload;

						memcpy(m.payload, &payload, sizeof(payload));
						size_payload = sizeof(payload);
					}
					// type -> STRING
					else {
						memcpy(m.payload, buffer_udp + 51, 1500);
						size_payload = strlen((char*)m.payload) + 1;
					}

					// Caut in lista de topicuri tinuta de server topicul mesajului abia primit
					// Daca exista, parcurg lista subscriberilor topicului respectiv si in functie
					// de conectivitatea lor la server, trimit mesajul sau il pun in coada de mesaje
					// a fiecarui subscriber cu SF = 1 
					topicList* tmp = searchTopic(&topics, topic);
					if(tmp != NULL){
						subscriber* tmpSub = tmp->subscribers;
						// Parcugere lista de subscribers
						while(tmpSub){
							clientList* client = checkConnectivity(clients, tmpSub->id_client);
							// Daca clientul este conectat trimit mesajul
							int msg_len = sizeof(m) - sizeof(m.payload) + size_payload;
							if(client != NULL){
								// Trimit numarul de bytes ocupat de mesaj
								n = send(client->socket, &msg_len, sizeof(msg_len), 0);
								n = send(client->socket, &m, msg_len, 0);
								// Daca nu au frost trimisi toti octetii ocupati de mesaj,
								// se trimit restul pana la trimiterea mesajului complet
								while(msg_len - n > 0){
									n += send(client->socket, &m + n, msg_len - n, 0);
								}
							// Daca nu este conectat, dar este abonat la topic cu SF = 1, pun mesajul
							// in coada pentru a fi trimis clientului la reconectare
							}else if(tmpSub->SF == 1){
								clientList *deconnected_cli = findClient(clients, tmpSub->id_client);
								if(deconnected_cli != NULL){
									enQ(&deconnected_cli->unsent_msg, &m, msg_len);
								}
							}
							tmpSub = tmpSub->next;
						}
					}
					// Daca topicul nu exista este adaugat in lista de topicuri tinuta de server
					else{
						pushTopic(&topics, m.topic);
					}				
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					int bytes_received;
					n = recv(i, &bytes_received, sizeof(bytes_received), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// Conexiunea s-a inchis
                        clientList *tmp = clients;
                        while(clients) {
							// Se cauta clientul care comunica prin intermediul socketului respectiv
							// si se marcheaza drept neconectat la server
                            if(clients->socket == i){
                                strcpy(id_client, clients->id_client);
								clients->connection_status = 0;
                                break;
                            }
                            clients = clients->next;
                        }
                        clients = tmp;
						printf("Client %s disconnected.\n", id_client);
						close(i);
						// Scot din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else {
						// Serverul primeste mesaj de subscribe / unsubscribe de la client TCP
						char command[15];
						char topic[51];
						int SF;

						// Primire mesaj
						// Se primeste prima data numarul de bytes ocupati de mesajul
						// ce va veni pentru a se lua fix atatia octeti cat ocupa 
						// mesajul in caz de concatenare
						n = recv(i, buffer, bytes_received, 0);
						// Daca nu au frost primiti toti octetii ocupati de mesajul
						// ce trebuie primit, se primesc in continuare octeti pana la
						// intregirea mesajului
						while(bytes_received - n > 0) {
							n += recv(i, buffer + n, bytes_received - n, 0);
						}

						// Separarea mesajului in campurile comanda topic si eventual SF
						sscanf(buffer,"%s %s %d\n", command, topic, &SF);

						if(strcmp(command, "subscribe") == 0){
							// Se primeste mesaj de abonare la un topic
							// Se verifica daca topicul exista in lista de topicuri tinuta de server
							// caz in care doar se adauga clientul in lista de subscriberi a topicului
							topicList* tmp = searchTopic(&topics, topic);
							if(tmp != NULL) {
								addSubscriber(&tmp, clients, i, SF);
							}
							// Daca topicul nu exista, este adaugat in lista si apoi se adauga subscriberul
							else
							{
								pushTopic(&topics, topic);
								tmp = searchTopic(&topics, topic);
								addSubscriber(&tmp, clients, i, SF);
							}
							continue;
						}

						if(strcmp(command, "unsubscribe") == 0){
							// Se primeste comanda de dezabonare de la topic
							deleteSubscriber(&topics, clients, topic, i);
							continue;
						}
					}
				}
			}
		}
	}
	close(sockfd);

	return 0;
}
