/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		tcp_clnt.c - A simple TCP client program.
--
--	PROGRAM:		tclnt.exe
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			February 2, 2008
--
--	REVISIONS:		(Date and Description)
--				January 2005
--				Modified the read loop to use fgets.
--				While loop is based on the buffer length
--
--
--	DESIGNERS:		Aman Abdulla
--
--	PROGRAMMERS:		Aman Abdulla
--
--	NOTES:
--	The program will establish a TCP connection to a user specifed server.
-- The server can be specified using a fully qualified domain name or and
--	IP address. After the connection has been established the user will be
-- prompted for date. The date string is then sent to the server and the
-- response (echo) back from the server is displayed.
---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN 512  	// Buffer length


void* readThreadFunc();
void* sendThreadFunc();
void signal_catcher(int signo);
void printFunc();
int n, bytes_to_read, sd;
char sbuf[BUFLEN], rbuf[BUFLEN], *bp;
char* printBuffer[2048];
int fileIndex = 0;

int main (int argc, char **argv)
{
	signal(SIGINT, signal_catcher);

	int port;
	struct hostent	*hp;
	struct sockaddr_in server;
	char  *host,   **pptr;
	char str[16];
  pthread_t readThread, sendThread;

	switch(argc)
	{
		case 2:
		host =	argv[1];	// Host name
		port =	SERVER_TCP_PORT;
		break;
		case 3:
		host =	argv[1];
		port =	atoi(argv[2]);	// User specified port
		break;
		default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	// Create the socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyname(host)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

	// Connecting to the server
	if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}
	printf("Connected:    Server Name: %s\n", hp->h_name);
	pptr = hp->h_addr_list;
	printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));
	printf("Transmit:\n");
	//gets(sbuf); // get user's text


	pthread_create (&readThread, NULL, readThreadFunc, NULL);
	pthread_create (&sendThread, NULL, sendThreadFunc, NULL);

	pthread_join(readThread, NULL);
	pthread_join(sendThread, NULL);


	close (sd);
	return (0);
}


void* readThreadFunc(){
	bp = rbuf;
	bytes_to_read = BUFLEN;
	while (1) {
		n = 0;
		while ((n = recv (sd, bp, bytes_to_read, 0)) < BUFLEN)
		{
			bp += n;
			bytes_to_read -= n;
		}
		printBuffer[fileIndex] = rbuf;
		fileIndex++;

		printf ("%s", rbuf);
		// printf("%d\n", fileIndex);

		fflush(stdout);
	}

}
void printFunc(){
	FILE* fp;
	int tempIndex = 0;
	remove("output.txt");
	fp = fopen("output.txt", "w");
	while (printBuffer[tempIndex] != NULL) {
		fputs(printBuffer[tempIndex], fp);
		printf("%s", printBuffer[tempIndex]);
		tempIndex++;
	}

	fclose(fp);
}

void* sendThreadFunc(){
	while (1)
	{
		fgets (sbuf, BUFLEN, stdin);
		// if(!strcmp(sbuf, "-p\n")){
		// 	printFunc();
		// 	continue;
		// }

		// char meString[6] = "Me: ";
		// strcat(meString, sbuf);
		printBuffer[fileIndex] = sbuf;
		fileIndex++;

		// Transmit data through the socket
		send (sd, sbuf, BUFLEN, 0);
	}

	return NULL;
}

void signal_catcher(int signo){
	if(signo == SIGINT){
		printFunc();
		char eof[2];
		eof[0] = EOF;
		send(sd, eof, BUFLEN, 0);
		kill(getpid(), SIGTERM);
	}
}
