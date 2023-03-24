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

// Handling multiple clients
// Mainting list of clients through txt file

sem_t sem;

struct server_data {
    char *ip;
    int client;
};

char usrname[1024];
char *client_target;
int client_sock = -1;

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
    sprintf(temp_buffer, "%s@%s %d\n", &buffer[5], client_data.ip, connfd);
    char *username = malloc(strlen(&buffer[5]) * sizeof(char));
    strcpy(username, &buffer[5]);
    // WRITE SEMAPHORE
    sem_wait(&sem);
    FILE *user = fopen("user.txt", "a");
    // fseek(user, 0, SEEK_END);
    fwrite(temp_buffer, 1, (strlen(temp_buffer) * sizeof(char)), user);
    fclose(user);
    sem_post(&sem);

    while (1) {
        char *temp;

        recv(connfd, buffer, 1024, 0);
        printf("%s\n", buffer);
        if (strcmp(buffer, "LIST\n") == 0) {
            memset(&buffer, 0, sizeof(buffer));
            strcpy(buffer, "CURR ");

            // READ SEMAPHORE
            sem_wait(&sem);
            FILE *fp = fopen("user.txt", "r");
            size_t len = 0;
            while (getline(&temp, &len, fp) != -1) {
                temp[strcspn(temp, "\n")] = ',';
                strcat(buffer, temp);
            }
            strcat(buffer, "\n");
            fclose(fp);
            sem_post(&sem);
        } else if (strcmp(buffer, "EXIT\n") == 0) {
            char file_temp[2048];
            // WRITE SEMAPHORE
            sem_wait(&sem);
            FILE *fp = fopen("user.txt", "r");
            size_t len = 0;
            while (getline(&temp, &len, fp) != -1) {
                if (strncmp(temp_buffer, temp, strcspn(temp, " ")) == 0) {
                    continue;
                }
                strcat(file_temp, temp);
            }
            fclose(fp);

            fp = fopen("user.txt", "w");
            fwrite(file_temp, 1, (strlen(file_temp) * sizeof(char)), fp);
            fclose(fp);
            sem_post(&sem);
            break;
        } else if (strchr(buffer, '@')) {
            // READ SEMAPHORE
            int flag = 0;

            sem_wait(&sem);
            char *username;
            FILE *fp = fopen("user.txt", "r");
            size_t len = 0;
            while ((getline(&temp, &len, fp)) != -1) {
                if (strncmp(temp_buffer, temp, strcspn(temp, " ")) == 0) {
                    // printf("MATCH found\n");
                    username = malloc(strcspn(buffer, "@") * sizeof(char));
                    strncpy(username, buffer, strcspn(buffer, "@"));
                    flag = 1;
                    break;
                }
            }
            fclose(fp);
            sem_post(&sem);

            memset(&buffer, 0, sizeof(buffer));

            if (flag == 1) {
                sprintf(buffer, "201 Available%s %d\n", username, connfd);
            } else {
                strcpy(buffer, "404 Not Found\n");
            }
        } else if (strncmp(buffer, "CURR", 4) == 0) {
            strcpy(buffer, "boop");
        } else {
            memset(&buffer, 0, sizeof(buffer));
            strcpy(buffer, "INVALID INPUT\n");
        }
        send(client_data.client, buffer, 1024, 0);
    }
    pthread_exit(NULL);  // Exits from thread
    // exit(1); // Exits from process
}

void *client_listener(void *client_socket) {
    int sockid = *(int *)client_socket;
    char buffer[1024];

    while (1) {
        if(recv(sockid, buffer, 1024, 0) == -1) pthead_exit(0);
        if (strncmp(buffer, "EXIT\n", 5) == 0) {
            // printf("listener exiting...\n");
            // send(sockid, buffer, 1024, 0);
            // pthread_exit(NULL);
        } else if (strncmp(buffer, "CURR", 4) == 0) {
            char temp[1024];
            strcpy(temp, buffer);
            char *token;

            memset(&buffer, 0, sizeof(buffer));
            token = strtok(temp, " ");
            token = strtok(NULL, ",");

            while (token != NULL) {
                if (strncmp(token, usrname, strcspn(token, " ")) == 0) {
                    token = strtok(NULL, ",");
                    continue;
                }
                strncat(buffer, token, strcspn(token, " "));
                strcat(buffer, "\n");
                token = strtok(NULL, ",");
            }
        } else if (strncmp(buffer, "201 Available", 13) == 0) {
            client_target = malloc(strlen(&buffer[13]) * sizeof(char));
            strncpy(client_target, &buffer[13], strcspn(&buffer[13], " "));
            client_sock = atoi(&buffer[strcspn(&buffer[13], " ") + 13]);
        }
        printf("%s\n", buffer);
        printf("Message data to be sent:\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Use 3 cli arguments\n");
        return -1;
    }
    char type = argv[3][0];
    int port = atoi(argv[2]);

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

        // Creating empty file
        FILE *fp = fopen("user.txt", "w");
        fclose(fp);

        // Accepting client connections
        while (1) {
            if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            struct server_data thread_data;
            thread_data.ip = malloc(strlen(inet_ntoa(cliaddr.sin_addr)) * sizeof(char));
            strcpy(thread_data.ip, inet_ntoa(cliaddr.sin_addr));
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

        sprintf(usrname, "%s@%s", buffer, cstr);
        sprintf(temp_buffer, "MESG %s", buffer);
        send(sockid, temp_buffer, 1024, 0);

        if (pthread_create(&cthread, NULL, client_listener, &sockid) != 0) {
            perror("thread creation failed");
            exit(EXIT_FAILURE);
        }

        // Communicating with server
        while (1) {
            // printf("Message data to be sent:\n");
            memset(&buffer, 0, sizeof(buffer));
            fgets(buffer, 1024, stdin);
            if (strcspn(buffer, "\n") == 1023) {
                printf("Input too long\n");
                continue;
            }

            if (strcmp(buffer, "EXIT\n") == 0) {
                printf("exiting...");
                break;
            } else if (client_sock != -1) {
                char *temp_b = malloc(strlen(buffer) * sizeof(char));
                strcpy(temp_b, buffer);
                sprintf(buffer, "MESG %s:%d %s", client_target, client_sock, temp_b);
            }
            send(sockid, buffer, 1024, 0);
        }
        // pthread_join(cthread, NULL);
        close(sockid);
        printf("client closed\n");
    }

    else {
        perror("Wrong Input\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}