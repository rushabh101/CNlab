#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void sendfile(char *query) {

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

	int sockfd, connfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;
	socklen_t cliaddr_len;

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
		if (listen(sockfd, 0) < 0)
		{
			perror("listen failed");
			exit(EXIT_FAILURE);
		}
		printf("Server is listening...\n");
		len = sizeof(cliaddr);
		// Accept a connection

		while (1)
		{
			if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
			{
				perror("accept failed");
				exit(EXIT_FAILURE);
			}

			printf("Connected to client\n");

			// Read data from the client
			while (1)
			{
				int n = recv(connfd, buffer, 1024, 0);
				if (n == -1)
				{
					printf("%s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				buffer[n] = '\0';
				printf("Client sent: %s\n", buffer);

				if (strcmp(buffer, "EXIT\n") == 0)
				{
					printf("exiting...\n");
					break;
				}
                else if(strncmp(buffer, "GET", 3) == 0) {
                    char *sfilename;
					char tempbuffer[1024];
					strcpy(tempbuffer, buffer);
                    strtok(tempbuffer," ");
                    sfilename = strtok (NULL," ");

					char *temp;
                    temp = strtok (NULL," ");
                    int bytecount = atoi(temp);

                    printf("%s %d\n", sfilename, bytecount);

                    FILE *file = fopen(sfilename, "r");
					if(!file) {
						perror("couldnt open owo\n");
						exit(1);
					}
					fseek(file, 0L, SEEK_END);
					int res = ftell(file);
					fseek(file, 0L, SEEK_SET);
					bytecount = bytecount > res ? res : bytecount;

                    printf("bytes read: %d\n", bytecount);

                    memset(buffer, 0, 1024);
                    int ss = fread(buffer, bytecount, 1, file);
                    fclose(file);
                }

				// Send a response to the client
				if (send(connfd, buffer, 1024, 0) == -1)
				{
					perror("sending failed");
					exit(EXIT_FAILURE);
				}
				printf("sent data: %s\n", buffer);
			}

			// Close the connection
			close(connfd);
		}

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

		// Communicating with server
		while (1)
		{
			printf("Input data to be sent:\n");
			fgets(buffer, 1024, stdin);
			if (strcspn(buffer, "\n") == 1023)
			{
				printf("Input too long\n");
				continue;
			}

			char *filename;
			send(sockid, buffer, 1024, 0);
			if (strcmp(buffer, "EXIT\n") == 0)
			{
				printf("exiting...");
				break;
			}
            else if(strncmp(buffer, "GET", 3) == 0) {
				char tempbuffer[1024];
				strcpy(tempbuffer, buffer);
                strtok (tempbuffer," ");
                filename = strtok (NULL," ");
            }
			val_rec = recv(sockid, buffer, 1024, 0);
			FILE *file = fopen(filename, "w");

			printf("%s received from server\n", buffer);

			int packet_size = strcspn(buffer, "\n");
            fwrite(buffer, packet_size, 1, file);
            fclose(file);
		}

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
