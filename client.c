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

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN 512  	// Buffer length


void* readThreadFunc();
void* sendThreadFunc();
void signal_catcher(int signo);
int n, bytes_to_read, sd;
char sbuf[BUFLEN], rbuf[BUFLEN], *bp;

int main (int argc, char **argv)
{
	signal(SIGINT, signal_catcher);

	int port;
	struct hostent	*hp;
	struct sockaddr_in server;
	char  *host,   **pptr;
	char str[16];
  	pthread_t readThread, sendThread;
	char fileBuffer[2048];
	int index = 0;

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
	int rbufIndex = 0;
	bp = rbuf;
	bytes_to_read = BUFLEN;
	while (1) {
		n = 0;
		while ((n = recv (sd, bp, bytes_to_read, 0)) < BUFLEN)
		{
			bp += n;
			bytes_to_read -= n;
		}
	
		printf ("%s", rbuf);
		fflush(stdout);
	}

}

void* sendThreadFunc(){
	while (1)
	{
		fgets (sbuf, BUFLEN, stdin);

		// Transmit data through the socket
		send (sd, sbuf, BUFLEN, 0);
	}

	return NULL;
}

void signal_catcher(int signo){
	if(signo == SIGINT){
		char eof[2];
		eof[0] = EOF;
		send(sd, eof, BUFLEN, 0);
		kill(getpid(), SIGTERM);
	}
}
