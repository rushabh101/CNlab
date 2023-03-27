#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    char *users[10];
    char *token;
    for(int i =0; i < 10; i++) {
        users[i] = malloc(20 * sizeof(char));
    }

    strcpy(users[1], "hello w o");
    token = strtok(users[1], " ");
    char *token2 = strtok(NULL, "\0");
    // for(int i =0; i < 10; i++) {
    //     printf("%d\n", strlen(users[i]));
    // }
    printf("%s\n", token);
}