-README - TEMA 2 PCOM : Aplicatie client-server TCP si UDP- 
            -pentru gestionarea mesajelor-

Student: Borbei Andreea
Grupa: 323CC

Protocol de nivel aplicatie:
    In vederea primirii corecte a mesajelor, am considerat urmatoarele:
-> retinerea unei liste de topicuri in server, iar pentru fiecare topic se 
retine o lista de abonati. Astfel pentru abonare/dezabonare de la un topic,
se va cauta topicul, apoi abonatul si se va efectua operatia respectiva. 
Aceasta ordonare a datelor usureaza si trimiterea mesajelor, deoarece toti
abonatii unui topic sunt tinuti la un loc.
-> pentru implementarea store-and-forward, pentru fiecare client s-a implementat
o coada de mesaje ce permite stocarea mesajelor legate de un topic la care clientul
s-a abonat cu optiunea SF = 1 si trimiterea acestora in ordine cronologica la 
reconectare.
-> am trimis mesaje de dimensiune variabila pentru a facilita trimiterea mesajelor
(trimiterea unui mesaj de dimensiune fixa de aproape, 1600 bytes, ar reduce eficienta).
In acest sens, la parsarea mesajelor primite de la clientii UDP, am retinut 
dimensiunea payload-ului si am trimis prin apelul functiei send numarul de bytes ocuopat
de mesaj.
-> pentru a rezolva concatenarea/trunchierea mesajelor realizata de TCP, am trimis catre
destinatar (fie el client TCP sau server) prima data numarul de bytes pe care trebuie sa
il asteapte, apoi am trimis mesajul in sine. Astfel, la concatenare, se va lua din buffer
exact numarul de bytes ocupat de mesaj. Daca send returneaza un numar mai mic decat
numarul de bytes care trebuie trimis (trunchierea mesajului), se apeleaza succesiv 
send pana la trimiterea in intregime a mesajului. Destinatarul, similar, va 
primi numarul de bytes si va apela succesiv functia recv pana la primirea 
intregului mesaj in functie de ceea ce returneaza apelul lui recv.
-> pentru reprezentarea mesajelor trimise de server am implementat structura message
ce va retine adresa IP si portul clientului UDP ce a trimis mesajul, topicul mesajului,
tipul datelor din payload si payload-ul.

Implementare server:
    Pentru a tine evidenta clientilor TCP carora trebuie sa le trimita
mesaje, serverul tine o lista cu clienti. Intr-un nod de lista se retine
ID-ul clientului, socketul pe care s-au primit date de la acesta, statusul
de conectare al acestuia si o coada de mesaje in care se vor retine mesajele
din perioada in care acesta a fost deconectat de la server.
De asemenea, serverul tine si o lista de topicuri. Pentru fiecare topic se 
retine, pe langa numele topicului, o lista de subscriberi.
    Se parcurg file descriptorii de la 0 pana la fdmax, iar pentru fiecare
se verifica daca se afla in multimea tmp_fds (care initial este identica cu
read_fds, dar se foloseste pentru a nu se modifica de catre apelul functiei
select multimea read_fds). Daca un file descriptor se afla in multime, se 
verifica daca este un socket pe care serverul fie asteapta cereri de conectare
de la clientii TCP, fie socket pe care primeste mesaje de la clientii UDP, fie
primeste date de la stdin, fie primeste date de la clientii TCP. 
    In cazul in care serverul primeste o cerere de conectare de la un client
de pe socketul pe care asculta, serverul mai primeste de la client ID-ul sau.
Acesta cauta in lista de clienti (cu ajutorul functiei search_client) ID-ul
clientului nou. Daca exista deja un client conectat cu acelasi ID 
(search_client intoarce 1), atunci este afisat un mesaj aferent si socketul
este inchis si scos din multimea read_fds. Daca ID-ul clientului este acelasi
cu ID-ul unui client deconectat, inseamna ca acesta se reconecteaza 
(search_client intoarce valoarea 2) si sunt trimise mesajele primite de client
in timpul in care acesta a fost deconectat (fiecare client are o coada de mesaje
in care sunt memorate mesajele din perioada de inactivitate). Altfel, clientul
este nou, deci este adaugat in lista de clienti si se afiseaza un mesaj
corespunzator.
    Singura comanda pe care o poate primi serverul de la stdin este cea de exit.
Se citeste de la stdin si daca comanda primita este diferita de exit, se afiseaza
un mesaj de eroare. Daca comanda este exit, lista de clienti este parcursa si 
pentru fiecare client se inchide socketul respectiv si se elibereaza memoria 
alocata dinamic pentru acesta (atat in lista de clienti, cat si pentru coada de 
mesaje aferenta fiecarui client). De asemenea, se elibereaza memoria si pentru
lista de topicuri.
    Daca serverul primeste mesaj de la un client UDP, se copiaza in campurile
ip_udp si port_udp ale variabilei de tip message adresa IP si portul clientului
UDP, iar apoi buffer-ul in care s-au scris octetii cu recvfrom este parsat. 
Primii 50 de bytes sunt interpretati drept topic. Astfel se copiaza intr-un 
string auxiliar topic primii 50 de bytes din buffer si se verifica daca in
string exista terminatorul de sir. Daca acesta nu exista, este adaugat, iar
sirul este copiat in campul topic al variabilei de tip message. Urmatorul byte
reprezinta tipul datelor din payload-ul mesajului, care este si el copiat in
campul type al variabilei de tip message. Urmatorii bytes sunt interpretati in
functie de campul type. Pentru INT (type = 0) urmatorul byte din buffer este
copiat intr-o variabila, el reprezentand semnul numarului. Apoi se copiaza 
urmatorii 4 bytes din buffer, se aplica functia ntohl pentru a-i transforma
din network byte order in host byte order. In functie de valoarea byte-ului de
semn, numarul rezultat este negativ (sign_byte = 1) sau pozitiv (sign_byte  = 0).
Apoi numarul rezultat este copiat la adresa de memorie a campului payload.
Pentru SHORT_REAL se copiaza din buffer urmatorii 2 bytes, se transforma din 
network byte order in host byte order si numarul obtinut se imparte la 100.
Numarul rezultat se copiaza la adresa campului payload din mesaj.
Pentru FLOAT, similar cu INT, se extrage byte-ul ce indica semnul numarului,
modulul si puterea negativa a lui 10 cu care trebuie inmultit modulul. Pentru
a calcula puterea lui 10 am folosit functia pow din biblioteca math.host
Pentru STRING, se copiaza la adresa campului payload din mesaj urmatorii 1500 bytes
din buffer.
    Pentru a trimite mesaje de dimensiune variata, se retine in variabila 
size_payload numarul de bytes ocupat de payload (pentru INT 4 bytes, pentru 
SHORT_REAL 8 bytes, pentru FLOAT 8 bytes, iar pentru STRING strlen(payload) + 1
bytes).
    Dupa parsarea mesajului si incadrarea lui in structura de date definita 
(message), topicul mesajului este cautat in lista de topicuri existente tinuta
de server. Daca este gasit, este parcursa lista de subscriberi a topicului.
Pentru fiecare subscriber se verifica daca acesta este conectat (caz in care
se trimite mesajul), iar daca nu este conectat, se verifica daca acesta s-a
abonat la topic cu SF = 1, caz in care mesajul este adaugat in coada de mesaje
a clientului respectiv pentru a fi trimis la reconectare. Daca topicul nu exista
in lista, acesta este adaugat, iar mesajul se pierde.
    Daca seerverul primeste date pe unul din socketii de client TCP exista 2
cazuri:
    Clientul s-a deconectat, iar la recv se primesc 0 bytes, caz in care pentru
clientul respectiv se actualizeaza connection_status la 0 (deconectat), se 
inchide socketul corespunzator si se afiseaza un mesaj sugestiv. Socketul este de 
asmenea eliminat din multimea read_fds. Daca numarul de bytes primit la primul
apel de recv este diferit de 0, atunci se mai apeleaza o data recv pentru a se 
primi mesajul efectiv (de subscribe sau unsubscribe). Mesajul primit este parsat
si se actioneaza in functie de comanda:
-Daca comanda este de subscribe, se cauta in lista de topicuri, topicul primit
in mesaj. Daca este gasit, clientul este adaugat in lista de subscriberi a 
topicului (daca nu este deja abonat, caz in care se va afisa la stderr un mesaj
corespunzator). Daca nu este gasit in lista, este adaugat si similar se adauga
subscriberul nou.
-Daca comanda este de unsubscribe, se apeleaza functia deleteSubscriber care 
cauta topicul curent in lista de topicuri, iar daca este gasit, cauta in lista
subscriberilor sai, clientul care a trimis comanda si il elimina la gasire.

Implementare client:
    Clientul poate primi de la tastatura comenzi de abonare/dezabonare. Odata
primita comanda, aceasta este parsata pentru verificarea corectitudinii si
trimisa serverului. Pentru comenzile diferite de subscribe, unsubscribe si exit
se afiseaza un mesaj corespunzator la stdout.
    De asemenea, acesta poate primi mesaje de la server. Pentru usurinta parsarii
mesajelor in client, am dat drept al doilea argument al functiei recv() adresa
unei variabile de tip message, astfel elementele mesajului (IP client UDP, port
client UDP, topic, type si payload) pot fi accesate cu usurinta. Dupa receptarea
in intregime a mesajului, acesta este afisat in functie de type.

*Mentionez ca implementarea cu liste nu este cea mai eficienta deoarece parcurgerea
element cu element a unei liste are complexitate mare (O(nr elemente din lista)).
De asemenea, pentru implementarea introducerii/scoaterii unui element din lista si
din coada am folosit cod de la link-urile [1] si [2]. Ca punct de plecare pentru codul
temei am folosit rezolvarile laboratoarelor 6, 7 si 8 de pe OCW.


Link-uri:
[1] https://www.geeksforgeeks.org/doubly-linked-list/
[2] https://www.geeksforgeeks.org/queue-linked-list-implementation/

