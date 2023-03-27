#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define USERS 20
// Handling multiple clients
// Mainting list of clients through txt file
// Sending msgs bw clients not supported

sem_t sem;

char *users[USERS];

char client_usrname[1024];

struct server_data {
    int client;
};

void *server_handler(void *clientsv) {
    struct server_data client_data = *(struct server_data *)clientsv;
    int connfd = client_data.client;
    char buffer[1024];

    recv(connfd, buffer, 1024, 0);
    printf("%s", buffer);
    // strcpy(buffer, "MESG what is your name?\n");
    send(connfd, "MESG what is your name?\n", 1024, 0);
    recv(connfd, buffer, 1024, 0);
    printf("%s\n", buffer);

    char temp_buffer[1024];
    sprintf(temp_buffer, "%s\n", &buffer[5]);
    users[connfd] = malloc(strlen(&buffer[5]) * sizeof(char));
    strcpy(users[connfd], &buffer[5]);

    while (1) {
        char *temp;

        recv(connfd, buffer, 1024, 0);

        if (strcmp(buffer, "LIST\n") == 0) {
            memset(&buffer, 0, sizeof(buffer));
            strcpy(buffer, "CURR ");

            for(int i = 0; i < USERS; i++){
                if(strlen(users[i]) != 0) {
                    strcat(buffer, users[i]);
                    strcat(buffer, " ");
                }
            }

        } else if (strcmp(buffer, "EXIT\n") == 0) {
            for(int i = 0; i < USERS; i++){
                if (strlen(users[i]) != 0) {
                    send(i, "EXIT", 5, 0);
                }
            }
            
            break;
        } else if(strncmp(buffer, "MESG", 4) == 0) {
            char temp[1024];
            strcpy(temp, buffer);
            char *token;

            token = strtok(temp, " ");
            token = strtok(NULL, " ");

            int target = -1;
            for(int i = 0; i < USERS; i++){
                if (strcmp(token, users[i]) == 0) {
                    target = i;
                    break;
                }
            }

            memset(&buffer, 0, sizeof(buffer));
            if(target != -1) {
                char *msg = strtok(NULL, "\0");

                // Save msg to respective file
                char msglog[1024];
                sprintf(msglog, "%s_%s.txt", users[connfd], token);
                FILE *f = fopen(msglog, "a");
                fprintf(f, "%s: %s", users[connfd], msg);
                fclose(f);

                sprintf(buffer, "MESG %s %s", users[connfd], msg);
                send(target, buffer, 1024, 0);
                continue;
            }
            else {
                send(client_data.client, "USER DOES NOT EXIST", 1024, 0);
                continue;
            }


        } 
        else {
            // READ SEMAPHORE
            int flag = 0;

            for(int i = 0; i < USERS; i++){
                if (strcmp(temp_buffer, users[i]) == 0) {
                    flag = 1;
                    break;
                }
            }

            memset(&buffer, 0, sizeof(buffer));

            if (flag == 1) {
                strcpy(buffer, "201 Available\n");
            } else {
                strcpy(buffer, "404 Not Found\n");
            }
        }
        send(client_data.client, buffer, 1024, 0);
    }
    // pthread_exit(NULL);  // Exits from thread
    exit(1); // Exits from process
}

void *client_listener(void *client_socket) {
    int sockid = *(int *)client_socket;
    char buffer[1024];

    while (1) {
        recv(sockid, buffer, 1024, 0);
        if (strncmp(buffer, "CURR", 4) == 0) {
            char temp[1024];
            strcpy(temp, buffer);
            char *token;

            memset(&buffer, 0, sizeof(buffer));
            token = strtok(temp, " ");
            token = strtok(NULL, " ");

            while (token != NULL) {
                if (strcmp(token, client_usrname) == 0) {
                    token = strtok(NULL, " ");
                    continue;
                }
                strcat(buffer, token);
                strcat(buffer, " ");
                token = strtok(NULL, " ");
            }
        }
        else if(strncmp(buffer, "MESG", 4) == 0) {
            time_t current_time;
            struct tm * time_info;
            char timeString[9];  // space for "HH:MM:SS\0"

            time(&current_time);
            time_info = localtime(&current_time);

            strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);

            char temp[1024];
            strcpy(temp, buffer);
            strtok(temp, " ");
            char *sender = strtok(NULL, " ");
            char *msg = strtok(NULL, "\0");

            memset(&buffer, 0, sizeof(buffer));
            sprintf(buffer, "%s %s: %s", timeString, sender, msg);
        }
        else if (strcmp(buffer, "EXIT") == 0) {
            printf("exiting...\n");
            exit(1);
        }
        printf("%s\n", buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Use 3 cli arguments\n");
        return -1;
    }
    char type = argv[3][0];
    int port = atoi(argv[2]);

    for(int i = 0; i < USERS; i++) {
        users[i] = malloc(20 * sizeof(char));
    }
    sem_init(&sem, 0, 1);

    int sockid;
    char buffer[1024];
    struct sockaddr_in addrport, cli;
    int addrlen = sizeof(addrport);

    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    socklen_t cliaddr_len;

    pthread_t thread, cthread;

    if (type == 's') {
        // Create a socket
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Clear servaddr and set fields
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
            perror("Invalid Address\n");
            exit(EXIT_FAILURE);
        }

        // Bind the socket to the address and port
        if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        // Listen for incoming connections
        if (listen(sockfd, 2) < 0) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        printf("Server is listening...\n");
        len = sizeof(cliaddr);

        // Accepting client connections
        while (1) {
            if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            struct server_data thread_data;
            thread_data.client = connfd;

            if (pthread_create(&thread, NULL, server_handler, &thread_data) != 0) {
                perror("thread creation failed");
                exit(EXIT_FAILURE);
            }
            pthread_detach(thread);
        }

        close(sockfd);
    }

    // CLIENT
    else if (type == 'c') {
        cliaddr_len = sizeof(cli);

        sockid = socket(PF_INET, SOCK_STREAM, 0);

        memset(&addrport, 0, sizeof(addrport));
        addrport.sin_family = AF_INET;
        addrport.sin_port = htons(port);
        // addrport.sin_addr.s_addr = htonl(INADDR_ANY);

        if (inet_pton(AF_INET, argv[1], &addrport.sin_addr) <= 0) {
            perror("Invalid Address\n");
            exit(EXIT_FAILURE);
        }

        char sstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addrport.sin_addr, sstr, INET_ADDRSTRLEN);
        if (connect(sockid, (struct sockaddr *)&addrport, addrlen) == -1) {
            printf("could not find server on %s:%d\n", sstr, ntohs(addrport.sin_port));
            exit(EXIT_FAILURE);
        }
        printf("connected to server\n");

        // Printing server and client ip and port
        if (getsockname(sockid, (struct sockaddr *)&cli, &cliaddr_len) < 0) {
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

        sprintf(client_usrname, "%s", buffer);
        sprintf(temp_buffer, "MESG %s", buffer);
        send(sockid, temp_buffer, 1024, 0);

        if (pthread_create(&cthread, NULL, client_listener, &sockid) != 0)
        {
            perror("thread creation failed");
            exit(EXIT_FAILURE);
        }

        // Communicating with server
        while (1) {
            printf("Message data to be sent:\n");
            fgets(buffer, 1024, stdin);
            if (strcspn(buffer, "\n") == 1023) {
                printf("Input too long\n");
                continue;
            }

            send(sockid, buffer, 1024, 0);
        }
    }

    else {
        perror("Wrong Input\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
