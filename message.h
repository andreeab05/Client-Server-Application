// Structura ce modeleaza mesajele trimise de server clientilor
// TCP. Pe langa datele primite de la clientii UDP (topic, type
// si payload), se mai retin si adresa IP si portul clientului
// UDP ce a trimis mesajul 
typedef struct message{
	struct in_addr ip_udp;
	in_port_t port_udp;
	char topic[50];
	uint8_t type;
	uint8_t payload[1500];
}message;