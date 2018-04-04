/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	server.c -   A simple multiplexed echo server using TCP,
--								 implemented in the chat room application.
--
--	PROGRAM:		server.exe
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			April 5, 2018
--
--	DESIGNERS:		Based on Richard Stevens Example, p165-166
--					Modified & redesigned:
--						Vafa Dehghan Saei & Luke Lee: April, 2018
--
--
--	PROGRAMMER:		Vafa Dehghan Saei
--					Luke Lee
--
--	NOTES:
--	The program will accept TCP connections from multiple client machines.
-- 	The program will read data from each client socket and echo it back to
--  all other clients except the one who sent the message, with the sender's
--  ip address.
---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	512		//Buffer length
#define TRUE	1
#define LISTENQ	5
#define MAXLINE 4096

// Client structure
struct ClientInfo
{
	int fileDesc;
	char ipAddr[BUFLEN];
};

// Global variables
struct ClientInfo clntInfo[FD_SETSIZE];

// Function headers
void* updateThreadFunc();
void displayClientList();

/*----------------------------------------------------------------------
-- FUNCTION:	main
--
-- DATE:		April 5, 2018
--
-- DESIGNER:	Vafa Dehghan Saei
--				Luke Lee
--
-- PROGRAMMER:	Vafa Dehghan Saei
--				Luke Lee
--
-- INTERFACE:	int main(int argc, char **argv)
--
-- ARGUMENT:	int argc		- an int that counts the number of
--								  arguments from cmd line
--				char **argv		- an array of char* that holds the
--								  values of each argument		
--
-- RETURNS:		int				- returns 0 if no error occurs
--
-- NOTES:
-- This is the entry point of the Linux chat console application for
-- server. The server address struct is initialized here and starts to
-- listen for incoming client connections.
----------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	int i, maxi, nready, bytes_to_read, arg;
	int listen_sd, new_sd, sockfd, port, maxfd;
	uint client_len;
	struct sockaddr_in server, client_addr;
	char *bp, buf[BUFLEN], newbuf[BUFLEN], tempIP[BUFLEN];
   	ssize_t n;
   	fd_set rset, allset;

	pthread_t updateThread;

	switch(argc)
	{
		case 1:
		port = SERVER_TCP_PORT;	// Use the default port
		break;
		case 2:
		port = atoi(argv[1]);	// Get user specified port
		break;
		default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	// Create a stream socket
	if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot Create Socket!");
	}

	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
	arg = 1;
	if (setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
	{
		perror("setsockopt");
	}

	// Bind an address to the socket
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

	if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		perror("bind error");
	}

	// printing starting message
	fprintf(stdout, "/================================\\\n");
   	fprintf(stdout, "| Welcome to the chat room!      |\n");
   	fprintf(stdout, "| Server started on port %d.   |\n", port);
   	fprintf(stdout, "| Type -d to show client list    |\n");
   	fprintf(stdout, "\\================================/\n");
	fprintf(stdout, "[Waiting for client connections...]\n");

	// Listen for connections
	// queue up to LISTENQ connect requests
	listen(listen_sd, LISTENQ);

	maxfd = listen_sd;	// initialize
   	maxi = -1;		// index into client[] array

	for (i = 0; i < FD_SETSIZE; i++)
	{
		clntInfo[i].fileDesc = -1;  // -1 indicates available entry
	}
 	FD_ZERO(&allset);
   	FD_SET(listen_sd, &allset);

   	pthread_create(&updateThread, NULL, updateThreadFunc, NULL);

	while (TRUE)
	{
		rset = allset; // structure assignment
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listen_sd, &rset)) // new client connection
		{
			client_len = sizeof(client_addr);
			if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, &client_len)) == -1)
			{
				perror("accept error");
			}
			printf("Client connected! Remote Address: %s\n", inet_ntoa(client_addr.sin_addr));


			for (i = 0; i < FD_SETSIZE; i++)
			{
				if (clntInfo[i].fileDesc < 0)
				{
					clntInfo[i].fileDesc = new_sd;	// save descriptor.
					strcpy(clntInfo[i].ipAddr, inet_ntoa(client_addr.sin_addr));
					//printf("Client added: %d, %d, %s\n", i, new_sd, clntInfo[i].ipAddr);
					break;
				}
			}

			if (i == FD_SETSIZE)
			{
				printf ("Too many clients\n");
				exit(1);
			}

			FD_SET (new_sd, &allset);     // add new descriptor to set
			if (new_sd > maxfd)
			{
				maxfd = new_sd;	// for select
			}
			if (i > maxi)
			{
				maxi = i;	// new max index in client[] array
			}

			if (--nready <= 0)
			{
				continue;	// no more readable descriptors
			}
		}
		for (i = 0; i <= maxi; i++)	// check all clients for data
		{
			if ((sockfd = clntInfo[i].fileDesc) < 0)
			{
				continue;
			}
			if (FD_ISSET(sockfd, &rset))
			{
				bp = buf;
				bytes_to_read = BUFLEN;
				while ((n = read(sockfd, bp, bytes_to_read)) > 0)
				{
					bp += n;
					bytes_to_read -= n;
				}
				for (int j = 0; j <= maxi; j++)	// send to all other clients
				{
					if (buf[0] == EOF) // connection closed by client
					{
						//printf("Remote Address:  %s closed connection\n", inet_ntoa(client_addr.sin_addr));
						printf("Remote Address:  %s closed connection\n", clntInfo[i].ipAddr);
						FD_CLR(sockfd, &allset);
						clntInfo[i].fileDesc = -1;
						break;
					}
					if ((sockfd = clntInfo[j].fileDesc) < 0 || j == i)
					{
						fprintf(stdout, "[%s] says: %s", clntInfo[i].ipAddr, buf);
						continue;
					}

					strcpy(tempIP, clntInfo[i].ipAddr);
					sprintf(newbuf, "%s: %s", tempIP, buf);
					//fprintf(stdout, "[%s] says: %s", clntInfo[i].ipAddr, buf);
					write(clntInfo[j].fileDesc, newbuf, BUFLEN);   // echo to client
				}
				if (--nready <= 0)
				{
					break;        // no more readable descriptors
				}
			}
		}
	}

	pthread_join(updateThread, NULL);

	return(0);
}

/*----------------------------------------------------------------------
-- FUNCTION:	updateThreadFunc
--
-- DATE:		April 5, 2018
--
-- DESIGNER:	Vafa Dehghan Saei
--				Luke Lee
--
-- PROGRAMMER:	Luke Lee
--
-- INTERFACE:	void* updateThreadFunc()
--
-- ARGUMENT:	void	
--
-- RETURNS:		void*			- returns a function pointer to the
--								  pthread_create() argument
--
-- NOTES:
-- This function is running on a new thread, and is responsible for
-- listening to user input. If user types -d from stdin, it displays
-- a list of currently connected clients.
----------------------------------------------------------------------*/
void* updateThreadFunc()
{
	char buff[BUFLEN];
	while(1)
	{
		fgets(buff, BUFLEN, stdin);
		if (strcmp(buff, "-d\n") == 0)
		{
			displayClientList();
		}
		else
		{
			printf("[Enter -d to show client list]\n");
		}
	}
}

/*----------------------------------------------------------------------
-- FUNCTION:	displayClientList
--
-- DATE:		April 5, 2018
--
-- DESIGNER:	Vafa Dehghan Saei
--				Luke Lee
--
-- PROGRAMMER:	Luke Lee
--
-- INTERFACE:	void displayClientList()
--
-- ARGUMENT:	void	
--
-- RETURNS:		void
--
-- NOTES:
-- This function is called when user types -d from stdin. It
-- displays a list of currently connected clients.
----------------------------------------------------------------------*/
void displayClientList()
{
	int i;
	fprintf(stdout, "Connected clients:\n");
	for(i = 0; i < FD_SETSIZE; i++)
	{
		if (clntInfo[i].fileDesc >= 0)
		{
			fprintf(stdout, "%d. IP Address: %s\n", i + 1, clntInfo[i].ipAddr);
		}
	}
	fprintf(stdout, "End of client list.\n");
}