## Aplicatie client-server


### Server

Serverul are un socket TCP pe care accepta conexiuni noi. Pentru fiecare client nou, se mai adauga cate un socket TCP. La incheierea executiei unui client, socketul aferent este inchis.
Pe langa acestea, mai este linkat si STDIN_FILENO, care este "ascultat". Fiecare mesaj pe acest sockfd (0 sau STDIN_FILENO) este tratat si verificat. Comanda care se poate trimite serverului este cea de exit, care duce la oprirea serverului si a tuturor conexiunilor.


Serverul accepta conexiuni UDP pe un scoket. Fiecare mesaj pe UDP este analizat, parsat, iar apoi trimis fiecarui subscriber. Pentru acei subscriberi care sunt offline, mesajele sunt salvate in structura interna pentru fiecare, in campul msg_buffer.

Un client offline este determinat prin verificarea campului sockfd: acesta este -1, atunci cand clientul este offline, sau pozitiv. Cand clientul este online, sockfd este socket-ul cu care serverul comunica cu clientul.
    
Un client este unic determinat in lista prin id-ul acestuia.
La deconectarea unui client, sockfd se seteaza pe -1.

La conectarea unui nou client, se verifica daca acesta exista deja. Daca exista, se va updata sockfd corespunzator, altfel, se va adauga un nou nod in lista.
Daca exista, se verifica daca lista de mesaje care trebuie trimise clientului contine elemente. Daca contine, toate mesajele vor fi trimise acum clientului.

Lista este complet golita la incheierea executiei serverului.

Fiecare client are o lista cu elemente de tipul s_topic. Acestea ofera detalii despre topicurile la care clientul este abonat, si daca au setat sau nu campul SF.

La primirea unui pachet UDP, se cauta toti clientii care au in lista lor topicul mesajului primit. Pentru aceia care au, se verifica daca sunt online. Daca clientul este online, mesajul se va trimite acum. Daca nu este online, se verifica daca campul SF este setat pentru acest topic. Daca este, atunci se va adauga mesajul in lista de mesaje ce trebuie trimise in viitor (pentru acel client). Daca nu este setat SF, se trece la verificarea urmatoruli client.
La autentificarea unui client care a fost offline, se vor trimite si sterge toate mesajele din aceasta coada.


### Client


La pornire, clientul seteaza un socket TCP pentru conexiunea cu serverul.
Pe langa acest socket, similar serverului, clientul asculta tot pe TCP si mesajele primite pe STDIN_FILENO, pentru tratarea comenzilor: subscribe, unsubscribe si exit.
La inceputul comunicatiei, clientul ii trimite serverului ID-ul sau. Astfel, serverului il va putea identifica unic.
Dupa acest mesaj, se vor trimite doar comenzi de subscribe / unsubscribe catre server. Comenzile de exit sunt trimise serverului ca un mesaj de dimensiune 0 (inclusiv la semnal kill).
Mesajele pe care le primeste clientul de la server sunt afisate la stdin.


### Compilare si rulare


Am utilizat Makefile-ul prezent in aceasta arhiva. Pentru a compila sursele se poate rula make build in linie de comanda.

    Pentru a rula serverul: ./server <PORT_DORIT>
    Pentru a rula un client: ./subscriber <ID_Client> <IP_Server> <Port_Server>
    Pentru a sterge executabilele, se poate folosi make clean.


### Precizari implementare


Am dezactivat algoritmul Nagle, setand: TCP_NODELAY.

    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(int));

Pentru a trimite mesje eficient peste TCP, am ales sa le trimit intr-un vector de octeti, intr-un buffer. Pentru a trimite comenzile, elementele sunt separate pritnr-un spatiu. Deoarcere topicul nu contine spatii, atunci dimensiunea buffer-ului este minima si nu se iroseste spatiu.
Pentru trimiterea datelor de la server la client, am ales formatarea datelor pe server si convertirea acestora intr-un sir de caractere, care este trimis apoi la client.

### Precizari suplimentare

Toata memoria este eliberata la incheierea executiei serverului.

Execmplu output Valgrind:

    ==11432== HEAP SUMMARY:
    ==11432==     in use at exit: 0 bytes in 0 blocks
    ==11432==   total heap usage: 471 allocs, 471 frees, 1,724,150 bytes allocated
    ==11432== 
    ==11432== All heap blocks were freed -- no leaks are possible

Am utilizat o functie similara cu DIE din laborator, numita error_handler. In caz de eroare, aceasta afiseaza un mesaj specificat la STDERR si incheie executia cu codul -1.

