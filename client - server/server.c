#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <netinet/tcp.h>

#include "utils.h"

#define MAX_CLIENTS 512

struct s_topic_msg {
	char *message;

	struct s_topic_msg *next;
};

struct s_topic {
	char *name;
	unsigned char sf;

	struct s_topic *next;
};

struct s_client {
	char id[IDLEN];
	int sockfd;
	struct s_topic *topics;

	struct s_topic_msg *msg_buffer;

	struct s_client *next;
};

int get_port(int argc, char **argv, int *port)
{
	/* Verificare numar de argumente */
	if (argc != 2)
		return -1;
	
	/* Conversie port la int */
	*port = atoi(argv[1]);

	/* Verificare atoi */
	if (*port == 0)
		return -1;

	/* Succes */
	return 0;
}

struct s_client *add_client(struct s_client *clients, char *id, int sockfd)
{
	struct s_client *p  = clients;
	struct s_topic_msg *m, *del;

	/* Verific daca clientul exista deja */
	while (p != NULL) {
		if (strcmp(p->id, id) == 0) {
			/* Se actualizeaza sockfd */
			p->sockfd = sockfd;

			/* Se trimit toate mesajele din msg_buffer */
			m = p->msg_buffer;

			while (m != NULL) {
				send(sockfd, m->message, strlen(m->message), 0);
				del = m;
				m = m->next;

				free(del->message);
				free(del);
			}

			p->msg_buffer = NULL;

			return clients;
		}

		p = p->next;
	}

	/* Se adauga un nou client */
	struct s_client *client = malloc(sizeof(struct s_client));

	strcpy(client->id, id);
	client->sockfd = sockfd;
	client->topics = NULL;
	client->msg_buffer = NULL;

	/* Se adauga primul in lista */
	client->next = clients;

	/* Se intoarce lista */
	return client;
}

char *get_id(struct s_client *clients, int sockfd)
{
	struct s_client *p  = clients;

	/* Cauta id-ul clientului cu fd = sockfd */
	while (p != NULL) {
		if (p->sockfd == sockfd)
			return p->id;

		p = p->next;
	}

	/* Tratare sockfd invalid */
	error_handler(p == NULL, "Internal error.");
	
	return "";
}

void disconnect(struct s_client *clients, int sockfd)
{
	struct s_client *p  = clients;

	/* Cauta id-ul clientului cu fd = sockfd */
	while (p != NULL) {
		if (p->sockfd == sockfd) {
			p->sockfd = -1;
			return;
		}

		p = p->next;
	}

	/* Tratare sockfd invalid */
	error_handler(p == NULL, "Internal error.");
}

void clean_topics(struct s_topic *topics)
{
	struct s_topic *p = topics;
	struct s_topic *del;

	while (p != NULL) {
		del = p;
		p = p->next;

		free(del->name);
		free(del);
	}
}

void clean_topic_msg(struct s_topic_msg *msg)
{
	struct s_topic_msg *p = msg;
	struct s_topic_msg *del;

	while (p != NULL) {
		del = p;
		p = p->next;

		free(del->message);
		free(del);
	}
}

void clean_clients(struct s_client *clients)
{
	struct s_client *p  = clients;
	struct s_client *del;

	while (p != NULL) {
		del = p;
		p = p->next;

		clean_topics(del->topics);
		clean_topic_msg(del->msg_buffer);

		free(del);
	}
}

struct s_cmd *create_command(char *buffer)
{
	struct s_cmd *cmd = malloc(sizeof(struct s_cmd));
	char *token = NULL;

	cmd->cmd = get_command_type(buffer);

	token = strtok(buffer, DELIMITERS);
	token = strtok(NULL, DELIMITERS);

	cmd->topic = malloc(strlen(token) + 1);

	strcpy(cmd->topic, token);

	if (cmd->cmd == CMD_SUB) {
		token = strtok(NULL, DELIMITERS);
		cmd->sf = atoi(token);
	}

	return cmd;
}

void clean_command(struct s_cmd *cmd)
{
	free(cmd->topic);
	free(cmd);
}

void subscribe(int sockfd, struct s_cmd *cmd, struct s_client *clients)
{
	struct s_client *client  = clients;
	struct s_topic *topic;

	/* Cauta clientul cu fd = sockfd */
	while (client != NULL) {
		if (client->sockfd == sockfd)
			break;

		client = client->next;
	}

	/* Clientul nu exista */
	error_handler(client == NULL, "Internal error.");

	/* Verific daca topicul exista deja */
	topic = client->topics;

	while (topic != NULL) {
		if (strcmp(topic->name, cmd->topic) == 0)
			return;
		topic = topic->next;
	}

	/* Creez un nou topic */
	topic = malloc(sizeof(struct s_topic));
	topic->name = malloc(strlen(cmd->topic) + 1);
	strcpy(topic->name, cmd->topic);
	topic->sf = cmd->sf;

	/* Adaug topicul in lista de topicuri ale clientului */
	topic->next = client->topics;
	client->topics = topic;
}

void unsubscribe(int sockfd, struct s_cmd *cmd, struct s_client *clients)
{
	struct s_client *client  = clients;
	struct s_topic *topic, *del;

	/* Cauta clientul cu fd = sockfd */
	while (client != NULL) {
		if (client->sockfd == sockfd)
			break;

		client = client->next;
	}

	/* Clientul nu exista */
	error_handler(client == NULL, "Internal error.");

	/* Verific daca topicul este primul */
	topic = client->topics;

	/* Verific daca exista cel putin un topic */
	if (topic == NULL)
		return;

	/* Verific daca este primul */
	if (strcmp(topic->name, cmd->topic) == 0) {
		client->topics = topic->next;

		free(topic->name);
		free(topic);
		return;
	}

	/* Caut topicul in lista */
	while (topic->next != NULL) {
		if (strcmp(topic->next->name, cmd->topic) == 0) {
			del = topic->next;

			topic->next = del->next;
			free(del->name);
			free(del);
			return;
		}

		topic = topic->next;
	}
}

void send_update(struct s_client *clients, char *buffer, char *ip, int port)
{
	char *topic = buffer;
	unsigned char type = buffer[50];
	void *content = buffer + 51;
	char *message = malloc(BUFLEN);
	struct s_client *client = clients;
	int sign;
	uint32_t number;
	uint16_t number_f;
	uint8_t number_pow;
	
	switch (type) {
	case 0:
		sign = (* (uint8_t *) (content)) == 0 ? 1 : -1;
		number = ntohl(* (uint32_t *) (content + 1));
		sprintf(message, "%s:%d - %s - INT - %d",
			ip,
			port,
			topic,
			sign * number);
		break;
	case 1:
		number_f = ntohs((* (uint16_t *) content));
		sprintf(message, "%s:%d - %s - SHORT_REAL - %.2f",
			ip,
			port,
			topic,
			((float) number_f) / 100);
		break;
	case 2:
		sign = (* (uint8_t *) (content)) == 0 ? 1 : -1;
		number = ntohl(* (uint32_t *) (content + 1));
		number_pow = * (uint8_t *) (content + 5);
		sprintf(message, "%s:%d - %s - FLOAT - %f",
			ip,
			port,
			topic,
			(float) sign * number / pow(10, number_pow));
		break;
	case 3:
		sprintf(message, "%s:%d - %s - STRING - %s",
			ip,
			port,
			topic,
			(char *) content);
		break;
	}

	while (client != NULL) {
		struct s_topic *t= client->topics;

		while (t != NULL) {
			if (strcmp(t->name, topic) == 0) {
				if (client->sockfd < 0 && t->sf == 1) {
					/* Adauga in coada de mesje */
					struct s_topic_msg *p = client->msg_buffer;
					struct s_topic_msg *msg = malloc(sizeof(struct s_topic_msg));

					msg->message = malloc(strlen(message) + 1);
					strcpy(msg->message, message);
					msg->next = NULL;

					if (p == NULL) {
						client->msg_buffer = msg;
					} else {
						while (p->next != NULL)
							p = p->next;

						p->next = msg;
					}
				} else {
					send(client->sockfd, message, strlen(message), 0);
				}
			}

			t = t->next;
		}

		client = client->next;
	}

	/* Eliberare spatiu auxiliar */
	free(message);
}

int main(int argc, char **argv)
{
	int rc;
	int sockfd_tcp, newsockfd, portno, sockfd_udp;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	fd_set tcp_fds;
	fd_set tmp_fds;
	int fdmax;
	struct s_client *clients = NULL;
	struct s_cmd *cmd;
	int flags = 1;

	FD_ZERO(&tcp_fds);
	FD_ZERO(&tmp_fds);

	/* Conversie port la int */
	rc = get_port(argc, argv, &portno);
	error_handler(rc != 0, "Usage: ./server PORT");

	/* Setare socket TCP primire conexiuni */
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	error_handler(sockfd_tcp < 0, "Function socket failed! (TCP)");

	/* Setare socket UDP */
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	error_handler(sockfd_udp < 0, "Function socket failed! (UDP)");

	/* Dezactivarea algoritmului Nagle */
	rc = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(int));
	error_handler(rc < 0, "Function setsockopt failed!");

	/* Setare serv_addr */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	/* Bind socket TCP */
	ret = bind(sockfd_tcp,
		(struct sockaddr *) &serv_addr,
		sizeof(struct sockaddr));
	error_handler(ret < 0, "Function bind failed! (TCP)");

	/* Bind socket UDP */
	ret = bind(sockfd_udp,
		(struct sockaddr *) &serv_addr,
		sizeof(struct sockaddr));
	error_handler(ret < 0, "Function bind failed! (UDP)");

	/* Asculta conexiuni TCP */
	ret = listen(sockfd_tcp, MAX_CLIENTS);
	error_handler(ret < 0, "Function listen failed!");

	/* Se adauga fd care asculta pentru conexiuni noi in multime */
	FD_SET(sockfd_tcp, &tcp_fds);

	/* Se adauga fd care asculta UDP */
	FD_SET(sockfd_udp, &tcp_fds);

	/* Se adauga fd pentru stdin */
	FD_SET(STDIN_FILENO, &tcp_fds);

	/* Se seteaza fdmax */
	fdmax = MAX(sockfd_tcp, sockfd_udp);

	while (1) {
		tmp_fds = tcp_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		error_handler(ret < 0, "Function select failed!");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp) {
					/* Cerere pentru o conexiune noua TCP */

					/* Se accepta cererea */
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_tcp,
						(struct sockaddr *) &cli_addr,
						&clilen);
					error_handler(newsockfd < 0, "Function accept failed!");

					/* Dezactivarea algoritmului Nagle */
					rc = setsockopt(newsockfd,
						IPPROTO_TCP,
						TCP_NODELAY,
						&flags,
						sizeof(int));
					error_handler(rc < 0, "Function setsockopt failed!");

					/* Se adauga noul socket (client TCP) */
					FD_SET(newsockfd, &tcp_fds);

					/* Actualizare fdmax */
					fdmax = MAX(fdmax, newsockfd);

					/* Se asteapta un mesaj de la client cu ID-ul */
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					error_handler(n < 0, "Function recv failed!");

					/* Se adauga clientul in lista de clienti */
					clients = add_client(clients, buffer, newsockfd);

					/* Log conectare client */
					printf("New client %s connected from %s:%d\n",
						buffer,
						inet_ntoa(cli_addr.sin_addr),
						ntohs(cli_addr.sin_port));

				} else if (i == sockfd_udp) {
					/* Mesaj UDP */

					/* Primest mesajul */
					memset(buffer, 0, BUFLEN);
					n = recvfrom(i, buffer, sizeof(buffer), 0,
						(struct sockaddr *) &cli_addr, &clilen);
					error_handler(n < 0, "Function recv failed! (UDP)");

					/* Prelucrez mesajul si il trimit */
					send_update(clients, buffer,
						inet_ntoa(cli_addr.sin_addr),
						ntohs(cli_addr.sin_port));
				} else if (i == STDIN_FILENO) {
					/* Mesaj de la stdin */

					/* Citeste linia de la tastatura */
					fgets(buffer, BUFLEN, stdin);

					/* Trateaza comanda exit */
					if (strncmp(buffer, EXIT_COMMAND, strlen(EXIT_COMMAND)) == 0)
						goto stop_server;
				} else {
					/* Se primeste un mesaj de un client TCP */

					/* Se primeste mesajul */
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					error_handler(n < 0, "Function recv failed!");

					if (n == 0) {
						/* Un client TCP isi incheie conexiunea */

						/* Extrag id-ul clientului */
						char *id = get_id(clients, i);

						/* Log deconectare client */
						printf("Client %s disconnected.\n", id);

						/* Se inchide sockfd */
						close(i);

						/* se scoate din multimea de citire socketul inchis */
						FD_CLR(i, &tcp_fds);

						/* Se marcheaza in lista de clienti ca fiind inchis */
						disconnect(clients, i);
					} else {
						/* Se primeste un mesaj de la un client TCP */
						cmd = create_command(buffer);

						/* Tratare comanda */
						switch (cmd->cmd) {
						case CMD_SUB:
							subscribe(i, cmd, clients);
							break;
						case CMD_USUB:
							unsubscribe(i, cmd, clients);
							break;
						}

						/* Eliberare memorie */
						clean_command(cmd);
					}
				}
			}
		}
	}

stop_server:
	/* Elibereaza memoria alocata pentru lista de clienti */
	clean_clients(clients);

	/* Inchide sockfd pentru primire conexiuni TCP */
	close(sockfd_tcp);

	/* Inchide sockfd pentru UDP */
	close(sockfd_udp);

	/* Executia se incheie cu succes */
	return 0;
}
