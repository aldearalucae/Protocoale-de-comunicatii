/* Macro pentru maximul dintre doua numere */
#define MAX(x, y) (x > y) ? x : y

#define BUFLEN 4096 /* Lungimea buffer-ului              */
#define IDLEN    11 /* Dimensiunea id-ului unui client   */

#define DELIMITERS " \n" /* Delimitatori pentru tokenizare */

#define EXIT_COMMAND "exit"        /* Identificator comanda exit        */
#define SUB_COMMAND  "subscribe"   /* Identificator comanda subscribe   */
#define USUB_COMMAND "unsubscribe" /* Identificator comanda unsubscribe */

#define CMD_INV  -1 /* Cod pentru o comanda invalida  */
#define CMD_EXIT  0 /* Cod pentru comanda exit        */
#define CMD_SUB   1 /* Cod pentru comanda subscribe   */
#define CMD_USUB  2 /* Cod pentru comanda unsubscribe */

#define MSG_INV       -1 /* Tipul mesajului este invalid                  */
#define MSG_INT        0 /* Tipul mesajului este intreg                   */
#define MSG_SHORT_REAL 1 /* Tipul mesajului este numar real cu 2 zecimale */
#define MSG_FLOAT      2 /* Tipul mesajului este real                     */
#define MSG_STRING     3 /* Tipul mesajului este sir de caractere         */

struct s_cmd {
    int cmd;     /* Id-ul comenzii (ex: CMD_SUB) */
    char *topic; /* Numele topicului             */
    int sf;      /* Store and forward value      */
};

/* Functia pentru tratarea erorilor (asemanator DIE din laborator) */
void error_handler(int condition, char *message)
{
	if (condition) {
		fprintf(stderr, "%s\n", message);
		exit(-1);
	}
}

int get_command_type(char *string)
{
	if (strncmp(string, EXIT_COMMAND, strlen(EXIT_COMMAND)) == 0)
		return CMD_EXIT;

	if (strncmp(string, SUB_COMMAND, strlen(SUB_COMMAND)) == 0)
		return CMD_SUB;
	
	if (strncmp(string, USUB_COMMAND, strlen(USUB_COMMAND)) == 0)
		return CMD_USUB;
	
	return CMD_INV;
}
