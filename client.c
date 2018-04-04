/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	client.c - A simple TCP client program for the chat application.
--
--	PROGRAM:		client.exe
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			April 5, 2018
--
--	FUNCTIONS:		
--					int main (int argc, char **argv)
--					void* readThreadFunc()
--					void* sendThreadFunc()
--					void printFunc()
--					void signal_catcher(int signo)
--					
--
--	DESIGNERS:		Vafa Dehghan Saei
--					Luke Lee
--
--	PROGRAMMERS:	Vafa Dehghan Saei
--					Luke Lee
--
--
--	NOTES:
--	The program will establish a TCP connection to a user specifed server.
--  The server can be specified using a fully qualified domain name or and
--	IP address. After the connection has been established the user can type
--  messages to be sent to other connected clients. The client will also
--  display any incoming message sent from other clients through the server.
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

	// printing starting message
	pptr = hp->h_addr_list;
	fprintf(stdout, "/====================================\n");
   	fprintf(stdout, "| Welcome to the chat room!\n");
   	fprintf(stdout, "| Connected to Server: %s\n", hp->h_name);
	fprintf(stdout, "| Server Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));
   	fprintf(stdout, "\\====================================\n");

	pthread_create (&readThread, NULL, readThreadFunc, NULL);
	pthread_create (&sendThread, NULL, sendThreadFunc, NULL);

	pthread_join(readThread, NULL);
	pthread_join(sendThread, NULL);

	close (sd);
	return (0);
}

/*----------------------------------------------------------------------
-- FUNCTION:	readThreadFunc
--
-- DATE:		April 5, 2018
--
-- DESIGNER:	Vafa Dehghan Saei 
--
-- PROGRAMMER:	Vafa Dehghan Saei
--
-- INTERFACE:	void* readThreadFunc()
--
-- ARGUMENT:	void		
--
-- RETURNS:		void*			- returns a function pointer to the
--								  pthread_create() argument
--
-- NOTES:
-- This function gets called after client connects to the server
-- successfully, a new thread is created to receive any incoming message
-- from server.
----------------------------------------------------------------------*/
void* readThreadFunc()
{
	int k = 0;
	char* temp[1024];
	bp = rbuf;
	bytes_to_read = BUFLEN;
	while (1)
	{
		n = 0;
		while ((n = recv (sd, bp, bytes_to_read, 0)) < BUFLEN)
		{
			bp += n;
			bytes_to_read -= n;
		}

		temp[k]= (char *) malloc(strlen(rbuf) + 1);
		memcpy(temp[k], rbuf, strlen(rbuf));

		printBuffer[fileIndex++] = temp[k++];
		printf ("\n%s", rbuf);
		printf ("(Me): "); // printing for send thread
		fflush(stdout);
	}

}

/*----------------------------------------------------------------------
-- FUNCTION:	sendThreadFunc
--
-- DATE:		April 5, 2018
--
-- DESIGNER:	Vafa Dehghan Saei 
--
-- PROGRAMMER:	Vafa Dehghan Saei
--
-- INTERFACE:	void* sendThreadFunc()
--
-- ARGUMENT:	void		
--
-- RETURNS:		void*			- returns a function pointer to the
--								  pthread_create() argument
--
-- NOTES:
-- This function gets called after client connects to the server
-- successfully, a new thread is created to listen for user input from
-- stdin and put it in a send buffer.
----------------------------------------------------------------------*/
void* sendThreadFunc()
{
	int k = 0;
	char* temp[1024];
	while (1)
	{
		fprintf(stdout, "(Me): ");
		fgets (sbuf, BUFLEN, stdin);
		if(!strcmp(sbuf, "-p\n"))
		{
			printf("[Chat saved to output.txt]\n");
			fflush(stdin);
			printFunc();
			continue;
		}


		//
		// char meString[6] = "Me: ";
		// strcat(meString, sbuf);

		temp[k]= (char *) malloc(strlen(sbuf) + 1);
		memcpy(temp[k], sbuf, strlen(sbuf));
		printBuffer[fileIndex++] = temp[k++];


		// Transmit data through the socket
		send (sd, sbuf, BUFLEN, 0);
	}

	return NULL;
}

/*----------------------------------------------------------------------
-- FUNCTION:	printFunc
--
-- DATE:		April 5, 2018
--
-- DESIGNER:	Vafa Dehghan Saei 
--
-- PROGRAMMER:	Vafa Dehghan Saei
--
-- INTERFACE:	void printFunc()
--
-- ARGUMENT:	void		
--
-- RETURNS:		void
--
-- NOTES:
-- This function is called when user turns on the save to file mode.
-- It saves the received messages to a text file.
----------------------------------------------------------------------*/
void printFunc()
{
	FILE* fp;
	int tempIndex = 0;
	remove("output.txt");
	fp = fopen("output.txt", "w");
	while (printBuffer[tempIndex] != NULL)
	{
		fputs(printBuffer[tempIndex++], fp);
	}

	fclose(fp);
}

/*----------------------------------------------------------------------
-- FUNCTION:	signal_catcher
--
-- DATE:		April 5, 2018
--
-- DESIGNER:	Vafa Dehghan Saei 
--
-- PROGRAMMER:	Vafa Dehghan Saei
--
-- INTERFACE:	void signal_catcher(int signo)
--
-- ARGUMENT:	int signo		- an int representing the signal number
--
-- RETURNS:		void
--
-- NOTES:
-- This function sets the Ctrl-C signal to send an EOF character to the
-- connected socket.
----------------------------------------------------------------------*/
void signal_catcher(int signo)
{
	if(signo == SIGINT)
	{
		char eof[2];
		eof[0] = EOF;
		send(sd, eof, BUFLEN, 0);
		kill(getpid(), SIGTERM);
	}
}
