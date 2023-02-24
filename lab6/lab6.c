#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct server_data
{
    char *username;
    int client;
};

void *server_handler(void *clientsv)
{
    struct server_data client_data = *(struct server_data *)clientsv;
    char buffer[1024];

    printf("Thread started for: %s", client_data.username);

    while (1)
    {
        char *temp;

        recv(client_data.client, buffer, 1024, 0);

        if (strcmp(buffer, "LIST\n") == 0)
        {
            memset(&buffer, 0, sizeof(buffer));
            strcpy(buffer, "CURR ");

            // READ SEMAPHORE
            FILE *fp = fopen("user.txt", "r");
            size_t len = 0;
            while (getline(&temp, &len, fp) != -1)
            {
                temp[strcspn(temp, "\n")] = ' ';
                strcat(buffer, temp);
            }
            strcat(buffer, "\n");
            fclose(fp);
        }
        else if (strcmp(buffer, "EXIT\n") == 0)
        {
            char file_temp[2048];
            // READ SEMAPHORE
            FILE *fp = fopen("user.txt", "r");
            size_t len = 0;
            while (getline(&temp, &len, fp) != -1)
            {
                if (strcmp(client_data.username, temp) == 0)
                {
                    continue;
                }
                strcat(file_temp, temp);
            }
            fclose(fp);

            fp = fopen("user.txt", "w");
            fwrite(file_temp, 1, (strlen(file_temp) * sizeof(char)), fp);
            fclose(fp);

            break;
        }
        else if (strchr(buffer, '@'))
        {
            // READ SEMAPHORE
            int flag = 0;
            FILE *fp = fopen("user.txt", "r");
            size_t len = 0;
            while ((getline(&temp, &len, fp)) != -1)
            {
                if (strcmp(buffer, temp) == 0)
                {
                    // printf("MATCH found\n");
                    flag = 1;
                    break;
                }
            }
            fclose(fp);
            memset(&buffer, 0, sizeof(buffer));

            if (flag == 1)
            {
                strcpy(buffer, "201 Available\n");
            }
            else
            {
                strcpy(buffer, "404 Not Found\n");
            }
        }
        else
        {
            memset(&buffer, 0, sizeof(buffer));
            strcpy(buffer, "INVALID INPUT\n");
        }
        send(client_data.client, buffer, 1024, 0);
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

    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    socklen_t cliaddr_len;

    pthread_t thread, cthread;

    if (type == 's')
    {
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
        len = sizeof(cliaddr);
        // Accept a connection
        FILE *fp = fopen("user.txt", "w");
        fclose(fp);
        while (1)
        {
            if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
            {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            recv(connfd, buffer, 1024, 0);
            printf("%s", buffer);
            // strcpy(buffer, "MESG what is your name?\n");
            send(connfd, "MESG what is your name?\n", 1024, 0);
            recv(connfd, buffer, 1024, 0);
            printf("%s\n", buffer);

            char temp_buffer[1024];
            sprintf(temp_buffer, "%s@%s\n", &buffer[5], inet_ntoa(cliaddr.sin_addr));

            // ADD SEMAPHORE
            FILE *user = fopen("user.txt", "a");
            // fseek(user, 0, SEEK_END);
            fwrite(temp_buffer, 1, (strlen(temp_buffer) * sizeof(char)), user);
            fclose(user);

            struct server_data thread_data;
            thread_data.username = malloc(strlen(temp_buffer) * sizeof(char));
            strcpy(thread_data.username, temp_buffer);
            thread_data.client = connfd;

            if (pthread_create(&thread, NULL, server_handler, &thread_data) != 0)
            {
                perror("thread creation failed");
                exit(EXIT_FAILURE);
            }
            pthread_detach(thread);
        }

        close(sockfd);
    }

    // CLIENT
    else if (type == 'c')
    {

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

        memset(&buffer, 0, sizeof(buffer));

        char temp_buffer[1024];
        fgets(buffer, 1024, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        char usrname[1024];
        sprintf(usrname, "%s@%s", buffer, cstr);
        sprintf(temp_buffer, "MESG %s", buffer);
        send(sockid, temp_buffer, 1024, 0);

        // if (pthread_create(&cthread, NULL, client_listener, &sockid) != 0)
        // {
        //     perror("thread creation failed");
        //     exit(EXIT_FAILURE);
        // }

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
            send(sockid, buffer, 1024, 0);
            if (strcmp(buffer, "EXIT\n") == 0)
            {
                printf("exiting...");
                break;
            }
            recv(sockid, buffer, 1024, 0);
            if (strncmp(buffer, "CURR", 4) == 0)
            {
                char temp[1024];
                strcpy(temp, buffer);
                char *token;

                memset(&buffer, 0, sizeof(buffer));
                token = strtok(temp, " ");
                token = strtok(NULL, " ");

                while (token != NULL)
                {
                    if (strcmp(token, usrname) == 0)
                    {
                        token = strtok(NULL, " ");
                        continue;
                    }
                    strcat(buffer, token);
                    strcat(buffer, " ");
                    token = strtok(NULL, " ");
                }
            }
            printf("%s\n", buffer);
        }
        // pthread_join(cthread, NULL);
        close(sockid);
        printf("client closed\n");
    }

    else
    {
        perror("Wrong Input\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
