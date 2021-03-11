#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "utils.h"

#define PORT 5000 
#define MAXLINE 1024 
#define MAX_CLIENTS 10

int main(int argc, char **argv) 
{
	int rc;
	int portno;
	int sockfd; 
	char id[IDLEN];
	char address[16];
	char buffer[BUFLEN]; 
	struct sockaddr_in servaddr;
	fd_set tcp_fds;
	fd_set tmp_fds;
	int flags = 1;
  
	FD_ZERO(&tcp_fds);
	FD_ZERO(&tmp_fds);

	/* Verificare numar de argumente */
	error_handler(argc != 4, "Usage: ./subscriber ID IP PORT");

	/* Extrag id client */
	strcpy(id, argv[1]);
	id[10] = '\0';

	/* Extrag adresa serverului */
	strcpy(address, argv[2]);

	/* Extrag port */
	portno = atoi(argv[3]);

	/* Setare socket TCP conexiune cu serverul */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	error_handler(sockfd < 0, "Function socket failed!");

	/* Dezactivarea algoritmului Nagle */
	rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(int));
	error_handler(rc < 0, "Function setsockopt failed!");

	/* Setare servaddr */
	memset((char *) &servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(portno);
	servaddr.sin_addr.s_addr = inet_addr(address);

	rc = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	error_handler(rc < 0, "Function connect failed!");

	/* Se adauga fd care asculta pentru conexiuni noi in multime */
	FD_SET(sockfd, &tcp_fds);

	/* Se adauga fd pentru stdin */
	FD_SET(STDIN_FILENO, &tcp_fds);

	/* Trimit serverului ID-ul meu */
	memset(buffer, 0, sizeof(buffer)); 

	strcpy(buffer, id);
	buffer[strlen(id)] = '\0';
	send(sockfd, buffer, sizeof(buffer), 0);

	while (1) {
		tmp_fds = tcp_fds;
		rc = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		error_handler(rc < 0, "Function select failed!");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			/* Mesajul este de la STDIN */

			/* Se citeste o linie de la stdin */
			memset(buffer, 0, sizeof(buffer)); 
			fgets(buffer, BUFLEN, stdin);

			/* Se verifica comanda */
			switch (get_command_type(buffer)) {
			case CMD_EXIT:
				/* Tratare comanda exit */
				goto exit_client;
			case CMD_SUB:
				/* Trimite mesaj la server */
				send(sockfd, buffer, sizeof(buffer), 0);

				/* Log pentru subscribe */
				printf("Subscribed topic.\n");
				break;
			case CMD_USUB:
				/* Trimite mesaj la server */
				send(sockfd, buffer, sizeof(buffer), 0);

				/* Log pentru unsubscribe */
				printf("Unsubscribed topic.\n");
				break;
			default:
				/* Tratare comanda invalida */
				printf("Invalid command!\n");
			}
		} else if (FD_ISSET(sockfd, &tmp_fds)) {
			/* Mesajul este de la server */
			memset(buffer, 0, sizeof(buffer)); 
			rc = recv(sockfd, buffer, sizeof(buffer), 0); 

			if (rc == 0)
				goto exit_client;

			printf("%s\n", buffer); 
		}
	}

exit_client:
	/* Se inchide socketul deschis pentru conexiunea cu serverul  */
	close(sockfd);

	/* Executia se incheie cu succes */
	return 0;
}
