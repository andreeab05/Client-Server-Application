#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include "message.h"
#define UDPMAX 1551

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
    char id_client[10];

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // Deschidere socket pentru comunicarea TCP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Dezactivare algoritm Nagle
    int flag = 1;
    int res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(res < 0, "bad Nagle");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));

    // Identificare id client
    strcpy(id_client, argv[1]);

    // Indentificarea adresei IP a serverului cu care clientul TCP va comunica  
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

    // Trimiterea id-ului clientului la server
    ret = send(sockfd, id_client, sizeof(id_client), 0);
    DIE(ret < 0, "can't send id");

    fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    // Se adauga socketul pentru comunicarea cu serverul in multimea de citire read_fds
	FD_SET(sockfd, &read_fds);
    // Se adauga noul file descriptor (socketul pe care clientul primeste mesaje de la tastatura) in multimea read_fds
    FD_SET(STDIN_FILENO, &read_fds);
	fdmax = sockfd;
	
	while (1) {
        tmp_fds = read_fds; 

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

        // Daca clientul primeste mesaj de la stdin
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            char command[BUFLEN], topic[BUFLEN];
            int SF;
		    memset(buffer, 0, sizeof(buffer));

		    n = read(0, buffer, sizeof(buffer) - 1);
		    DIE(n < 0, "failed read");

            int len_buffer = strlen(buffer);

            // Se primeste comanda de unsubscribe si se trimite mesaj catre server
            if (strstr(buffer, "unsubscribe") != NULL) {
                sscanf(buffer, "%s %s", command, topic);
                n = send(sockfd, &len_buffer, sizeof(len_buffer), 0);
                n = send(sockfd, buffer, len_buffer, 0);
                while(len_buffer - n > 0){
                    n += send(sockfd, buffer, len_buffer - n, 0);
                }
		        DIE(n < 0, "can't send subscribe message to server :(");
                if(n >= 0)
                    printf("Unsubscribed to topic\n");
            }
            // Se primeste comanda de subscribe si se trimite mesaj catre server
            else if (strstr(buffer, "subscribe") != NULL) {
                sscanf(buffer, "%s %s %d", command, topic, &SF);
                if (SF != 0 && SF != 1) {
                    printf("Wrong command! :( \n");
                    continue;
                }
                n = send(sockfd, &len_buffer, sizeof(len_buffer), 0);
                n = send(sockfd, buffer, len_buffer, 0);
                while(len_buffer - n > 0){
                    n += send(sockfd, buffer, len_buffer - n, 0);
                }
		        DIE(n < 0, "can't send unsubscribe message to server :(");
                if(n >= 0)
                    printf("Subscribed to topic.\n");
            }
            // Se primeste comanda de exit si clientul se inchide
            else if (strstr(buffer, "exit") != NULL) {
                break;
            }
            // Altfel, comanda primita este invalida
            else printf("Unknown command. Try again!\n");
        }
        else{
            // Clientul primeste mesaj de la server
            // Se primeste prima data numarul de bytes ocupati de mesajul ce va veni
            // pentru a se lua fix atatia octeti cat ocupa mesajul in caz de concatenare
            int bytes_received;
            int recv1 = recv(sockfd, &bytes_received, sizeof(bytes_received), 0);
            if(recv1 == 0)
                break;

            message new_message;
            int rec = recv(sockfd, &new_message, bytes_received, 0);
            DIE(rec < 0, "bad recv");

            // Daca nu s-au primit din prima toti octetii necesari (trunchierea mesajului),
            // se primesc in continuare bytes pana la intregirea mesajului
            while(bytes_received - rec > 0){
                rec += recv(sockfd, &new_message + rec, bytes_received - rec, 0);
            }

            // Afisarea mesajului primit de la server in functie de tipul datelor din payload-ului mesajului
            if(new_message.type == 0){
                printf("%s:%d - %s - %s - %d\n", inet_ntoa(new_message.ip_udp), ntohs(new_message.port_udp), new_message.topic, "INT", *(int *)new_message.payload); 
            }
            else if(new_message.type == 1){
                printf("%s:%d - %s - %s - %.2lf\n", inet_ntoa(new_message.ip_udp), ntohs(new_message.port_udp), new_message.topic, "SHORT_REAL", *(double*)new_message.payload);
            }
            else if(new_message.type == 2){
                printf("%s:%d - %s - %s - %.4f\n", inet_ntoa(new_message.ip_udp), ntohs(new_message.port_udp), new_message.topic, "FLOAT", *(double*)new_message.payload);
            }
            else if(new_message.type == 3){
                printf("%s:%d - %s - %s - %s\n", inet_ntoa(new_message.ip_udp), ntohs(new_message.port_udp), new_message.topic, "STRING",(char *)new_message.payload);
            }
        }
	}
	close(sockfd);
	return 0;
}
