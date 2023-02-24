#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void *server_handler(void *clientsv)
{
	int *clients = (int *)clientsv;
	char buffer[1024];

	printf("Thread %d started\n", clients[2]);

	int sender = clients[0];
	int rcver = clients[1];

	while (1)
	{
		recv(sender, buffer, 1024, 0);

		if (strncmp(buffer, "EXIT\n", 5) == 0)
		{
			send(rcver, buffer, 1024, 0);
			printf("exiting...\n");
			break;
		}
		else if (strncmp(buffer, "MESG", 4) == 0)
		{
			printf("client%d: %s\n", (int)clients[2], &buffer[5]);
			send(rcver, buffer, 1024, 0);
		}
		else
		{
			printf("Invalid Input\n");
		}
	}
	pthread_exit(NULL);
}

void *client_listener(void *client_socket)
{
	int sockid = *(int *)client_socket;
	char buffer[1024];

	while (1)
	{
		recv(sockid, buffer, 1024, 0);
		if (strncmp(buffer, "EXIT\n", 5) == 0)
		{
			printf("listener exiting...\n");
			send(sockid, buffer, 1024, 0);
			pthread_exit(NULL);
		}
		printf("%s\n", buffer);
		printf("Message data to be sent:\n");
	}
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		printf("Use 3 cli arguments\n");
		return -1;
	}
	char type = argv[3][0];
	int port = atoi(argv[2]);

	int sockid, new_conn, val_rec;
	char buffer[1024];
	struct sockaddr_in addrport, cli;
	int addrlen = sizeof(addrport);

	int sockfd, connfd1, connfd2;
	struct sockaddr_in servaddr, cliaddr1, cliaddr2;
	socklen_t len;
	socklen_t cliaddr_len;

	pthread_t thread1, thread2, cthread;

	switch (type)
	{

	// SERVER
	case 's':
		// Create a socket
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("socket creation failed");
			exit(EXIT_FAILURE);
		}

		// Clear servaddr and set fields
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		// servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(port);

		if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
		{
			perror("Invalid Address\n");
			exit(EXIT_FAILURE);
		}

		// Bind the socket to the address and port
		if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		{
			perror("bind failed");
			exit(EXIT_FAILURE);
		}

		// Listen for incoming connections
		if (listen(sockfd, 2) < 0)
		{
			perror("listen failed");
			exit(EXIT_FAILURE);
		}
		printf("Server is listening...\n");
		len = sizeof(cliaddr1);
		// Accept a connection

		if ((connfd1 = accept(sockfd, (struct sockaddr *)&cliaddr1, &len)) < 0)
		{
			perror("accept failed");
			exit(EXIT_FAILURE);
		}

		printf("Connected to client1\n");

		recv(connfd1, buffer, 1024, 0);
		strcpy(buffer, "200 OK\n");
		if (send(connfd1, buffer, 1024, 0) == -1)
		{
			perror("sending failed");
			exit(EXIT_FAILURE);
		}

		if ((connfd2 = accept(sockfd, (struct sockaddr *)&cliaddr1, &len)) < 0)
		{
			perror("accept failed");
			exit(EXIT_FAILURE);
		}

		printf("Connected to client2\n");

		recv(connfd2, buffer, 1024, 0);
		strcpy(buffer, "200 OK\n");
		if (send(connfd2, buffer, 1024, 0) == -1)
		{
			perror("sending failed");
			exit(EXIT_FAILURE);
		}

		memset(buffer, 0, 1024);
		strcpy(buffer, "CONN client 2\n");
		send(connfd1, buffer, 1024, 0);

		int thread_data1[3] = {connfd1, connfd2, 1};
		if (pthread_create(&thread1, NULL, server_handler, thread_data1) != 0)
		{
			perror("thread creation failed");
			exit(EXIT_FAILURE);
		}

		int thread_data2[3] = {connfd2, connfd1, 2};
		if (pthread_create(&thread2, NULL, server_handler, thread_data2) != 0)
		{
			perror("thread creation failed");
			exit(EXIT_FAILURE);
		}

		printf("Threads made\n");
		pthread_join(thread1, NULL);
		pthread_join(thread2, NULL);

		close(connfd1);
		close(connfd2);
		close(sockfd);

		break;

	// CLIENT
	case 'c':

		cliaddr_len = sizeof(cli);

		sockid = socket(PF_INET, SOCK_STREAM, 0);

		memset(&addrport, 0, sizeof(addrport));
		addrport.sin_family = AF_INET;
		addrport.sin_port = htons(port);
		// addrport.sin_addr.s_addr = htonl(INADDR_ANY);

		if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
		{
			perror("Invalid Address\n");
			exit(EXIT_FAILURE);
		}

		char sstr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addrport.sin_addr, sstr, INET_ADDRSTRLEN);
		if (connect(sockid, (struct sockaddr *)&addrport, addrlen) == -1)
		{
			printf("could not find server on %s:%d\n", sstr, ntohs(addrport.sin_port));
			exit(EXIT_FAILURE);
		}
		printf("connected to server\n");

		// Printing server and client ip and port
		if (getsockname(sockid, (struct sockaddr *)&cli, &cliaddr_len) < 0)
		{
			perror("getsockname failed");
			exit(EXIT_FAILURE);
		}
		char cstr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &cli.sin_addr, cstr, INET_ADDRSTRLEN);

		printf("%s:%d is connected to %s:%d\n", cstr, ntohs(cli.sin_port), sstr, ntohs(addrport.sin_port));

		strcpy(buffer, "HELO\n");
		send(sockid, buffer, 1024, 0);
		recv(sockid, buffer, 1024, 0);
		printf("%s\n", buffer);

		if (pthread_create(&cthread, NULL, client_listener, &sockid) != 0)
		{
			perror("thread creation failed");
			exit(EXIT_FAILURE);
		}

		// Communicating with server
		while (1)
		{
			printf("Message data to be sent:\n");
			fgets(buffer, 1024, stdin);
			if (strcspn(buffer, "\n") == 1023)
			{
				printf("Input too long\n");
				continue;
			}
			if (pthread_tryjoin_np(cthread, NULL) == 0)
			{
				break;
			}

			send(sockid, buffer, 1024, 0);
			if (strcmp(buffer, "EXIT\n") == 0)
			{
				printf("exiting...");
				break;
			}
		}
		// pthread_join(cthread, NULL);
		close(sockid);
		printf("client closed\n");

		break;

	default:
		perror("Wrong Input\n");
		exit(EXIT_FAILURE);
		break;
	}

	return 0;
}
